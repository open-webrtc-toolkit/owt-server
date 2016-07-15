/**
 * @file tcp.cpp  Transport Control Protocol for Symbian
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <es_sock.h>
#include <in_sock.h>

extern "C" {
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_net.h>
#include <re_tcp.h>


#define DEBUG_MODULE "tcp"
#define DEBUG_LEVEL 5
#include <re_dbg.h>
}


enum {
	TCP_RXSZ_DEFAULT = 8192
};


/**
 * Singleton proxy to RSocketServer with reference counting
 */
class CTcpSocketServer : public CBase {
public:
	CTcpSocketServer();
	~CTcpSocketServer();

public:
	static RSocketServ iSocketServer;  /**< Socket Server */
private:
	static TInt iRefCount;
};


class CTcpSock : public CActive {
public:
	CTcpSock(struct tcp_sock *ts);
	~CTcpSock();
	int  Bind(TInetAddr &ia);
	int  Listen(int backlog);
	void Accepting();
protected:
	void RunL();
	void DoCancel();
public:
	CTcpSocketServer* iSockServer;  /**< Shared Socket Server */
	RSocket iSocket;                /**< Socket object        */
	struct tcp_sock *ts;
};


class CTcpConn : public CActive {
public:
	CTcpConn(struct tcp_sock *ts);
	CTcpConn(struct tcp_conn *tc);
	~CTcpConn();
	int  Open(TInetAddr &ia);
	int  Bind(TInetAddr &ia);
	int  Connect(TInetAddr &ia);
	void StartReceiving();
protected:
	void RunL();
	void DoCancel();
public:
	CTcpSocketServer* iSockServer;   /**< Shared Socket Server */
	RSocket iSocket;                 /**< Socket object        */
	TBuf8<TCP_RXSZ_DEFAULT> iBufRx;  /**< Receive buffer       */
	TSockXfrLength iLen;
	bool connecting;
	bool receiving;
	bool blank;
	struct tcp_sock *ts;
	struct tcp_conn *tc;
};


/** Defines a listening TCP socket */
struct tcp_sock {
public:
	int  blank_socket();
	void tcp_conn_handler();
public:
	CTcpSock *cts;        /**< Listening TCP socket              */
	CTcpConn *ctc;        /**< Pending blank socket (connection) */
	tcp_conn_h *connh;    /**< TCP Connect handler               */
	void *arg;            /**< Handler argument                  */
};


/** Defines a TCP connection */
struct tcp_conn {
public:
	void recv();
	void conn_close(int err);
	int  send(struct mbuf *mb);
public:
	struct list helpers;
	CTcpConn *ctc;        /**< TCP Connection socket             */
	tcp_estab_h *estabh;  /**< Connection established handler    */
	tcp_recv_h *recvh;    /**< Data receive handler              */
	tcp_close_h *closeh;  /**< Connection close handler          */
	void *arg;            /**< Handler argument                  */
	size_t rxsz;          /**< Maximum receive chunk size        */
	bool active;
};

struct tcp_helper {
	struct le le;
	int layer;
	tcp_helper_estab_h *estabh;
	tcp_helper_send_h *sendh;
	tcp_helper_recv_h *recvh;
	void *arg;
};

static RSocketServ *socketServer = NULL;
static RConnection *rconnection = NULL;


/****************************************************************************
 *                      Implementation of CTcpSocketServer                  *
 ****************************************************************************/

RSocketServ CTcpSocketServer::iSocketServer;
TInt CTcpSocketServer::iRefCount = 0;


CTcpSocketServer::CTcpSocketServer()
{
	DEBUG_INFO("CTcpSocketServer(%p) ctor refcount=%d\n", this,
		   iRefCount);

	if (iRefCount == 0) {
		DEBUG_INFO("CTcpSocketServer(%p) ctor Connect\n", this);

		if (socketServer) {
			iSocketServer = *socketServer;
		}
		else {
			iSocketServer.Connect();
		}
	}

	++iRefCount;
}


CTcpSocketServer::~CTcpSocketServer()
{
	DEBUG_INFO("CTcpSocketServer(%p) dtor: refcount=%d\n", this,
		   iRefCount);

	if (--iRefCount == 0) {
		DEBUG_INFO("CTcpSocketServer(%p) dtor Close\n", this);

		if (!socketServer)
			iSocketServer.Close();
	}
}


/****************************************************************************
 *                      Implementation of CTcpSock                          *
 ****************************************************************************/


CTcpSock::CTcpSock(struct tcp_sock *ts)
	:CActive(EPriorityHigh)
	,ts(ts)
{
	DEBUG_INFO("CTcpSock(%p) ctor\n", this);
	CActiveScheduler::Add(this);
	iSockServer = new CTcpSocketServer();
}


CTcpSock::~CTcpSock()
{
	DEBUG_INFO("~CTcpSock(%p) dtor\n", this);

	Cancel();

	iSocket.Close();
	delete iSockServer;
}


int CTcpSock::Bind(TInetAddr &ia)
{
	TInt ret;

	ret = iSocket.Open(ts->cts->iSockServer->iSocketServer, ia.Family(),
			   KSockStream, KProtocolInetTcp);
	if (KErrNone != ret) {
		DEBUG_WARNING("bind: Open (ret=%d)\n", ret);
		goto error;
	}

	ret = iSocket.SetOpt(KSoReuseAddr, KSolInetIp, 1);
	if (KErrNone != ret) {
		DEBUG_WARNING("SetOpt ReuseAddr: ret=%d\n", ret);
	}

	ret = iSocket.Bind(ia);
	if (KErrNone != ret) {
		DEBUG_WARNING("bind: Bind (ret=%d)\n", ret);
		goto error;
	}

 error:
	return kerr2errno(ret);
}


int CTcpSock::Listen(int backlog)
{
	return kerr2errno(iSocket.Listen(backlog));
}


void CTcpSock::Accepting()
{
	/* MUST be done before RemoteName() */
	iSocket.Accept(ts->ctc->iSocket, iStatus);
	SetActive();
}


void CTcpSock::RunL()
{
	DEBUG_INFO("CTcpSock RunL (%p) iStatus=%d ts=%p\n", this,
		   iStatus.Int(), ts);

	/* This happens after calling cancel functions in socket */
	if (iStatus == KErrCancel || iStatus == KErrAbort) {
		iStatus = KErrNone;
		return;
	}
	else if (iStatus != KErrNone) {
		DEBUG_WARNING("RunL received iStatus %d\n", iStatus.Int());
	}

	/* Incoming CONNECT */
	DEBUG_INFO("CTcpSock RunL (%p) Incoming CONNECT\n", this);
	ts->tcp_conn_handler();
}


void CTcpSock::DoCancel()
{
	DEBUG_INFO("CTcpSock(%p) DoCancel\n", this);

	iSocket.CancelAll();
}


/****************************************************************************
 *                      Implementation of CTcpConn                          *
 ****************************************************************************/


CTcpConn::CTcpConn(struct tcp_conn *tc)
	:CActive(EPriorityHigh)
	,ts(NULL)
	,tc(tc)
{
	DEBUG_INFO("CTcpConn(%p) ctor (tc=%p)\n", this, tc);

	CActiveScheduler::Add(this);
	iSockServer = new CTcpSocketServer();
}


CTcpConn::CTcpConn(struct tcp_sock *ts)
	:CActive(EPriorityHigh)
	,ts(ts)
	,tc(NULL)
{
	DEBUG_INFO("CTcpConn(%p) ctor (ts=%p)\n", this, ts);
	CActiveScheduler::Add(this);
	iSockServer = new CTcpSocketServer();
}


CTcpConn::~CTcpConn()
{
	DEBUG_INFO("~CTcpConn(%p) dtor\n", this);

	Cancel();

	/*
	 * This does not work for accepted connections, if the main
	 * socketserver is closed in CTcpSock in advance
	 */
	if (!blank) {
		iSocket.Close();
	}

	delete iSockServer;
}


int CTcpConn::Open(TInetAddr &ia)
{
	TInt ret;
	if (rconnection) {
		DEBUG_INFO("TCP Socket Open: Using RConnection=%p\n",
			   rconnection);
		ret = iSocket.Open(iSockServer->iSocketServer, ia.Family(),
				   KSockStream, KProtocolInetTcp,
				   *rconnection);
	}
	else {
		ret = iSocket.Open(iSockServer->iSocketServer, ia.Family(),
				   KSockStream, KProtocolInetTcp);
	}

	return kerr2errno(ret);
}


int CTcpConn::Bind(TInetAddr &ia)
{
	TInt ret;

	ret = iSocket.SetOpt(KSoReuseAddr, KSolInetIp, 1);
	if (KErrNone != ret) {
		DEBUG_WARNING("SetOpt ReuseAddr: ret=%d\n", ret);
	}

	return kerr2errno(iSocket.Bind(ia));
}


int CTcpConn::Connect(TInetAddr &ia)
{
	DEBUG_INFO("CTcpConn(%p) Connect\n", this);

	connecting = true;

	iSocket.Connect(ia, iStatus);
	SetActive();

	return 0;
}


void CTcpConn::StartReceiving()
{
	DEBUG_INFO("TcpConn(%p) StartReceiving\n", this);

	/* Clear buffer */
	iBufRx.Zero();

	receiving = true;

	iSocket.RecvOneOrMore(iBufRx, 0, iStatus, iLen);
	SetActive();
}


void CTcpConn::RunL()
{
	DEBUG_INFO("CTcpConn RunL (%p) iStatus=%d ts=%p tc=%p\n", this,
		   iStatus.Int(), ts, tc);

	/* This happens after calling cancel functions in socket */
	if (iStatus == KErrCancel || iStatus == KErrAbort) {
		iStatus = KErrNone;
		return;
	}
	else if (iStatus == KErrEof) {
		;
	}
	else if (iStatus != KErrNone) {
		DEBUG_WARNING("RunL received iStatus %d\n", iStatus.Int());
	}

	/* Outgoing CONNECT */
	if (connecting) {
		connecting = false;
		if (iStatus == KErrNone) {
			int err = 0;
			DEBUG_INFO("TCP Conn ESTABLISHED\n");

			struct le *le = list_head(&tc->helpers);
			while (le) {
				struct tcp_helper *th;

				th = (struct tcp_helper *)le->data;
				le = le->next;

				if (th->estabh(&err, tc->active, th->arg)
				    || err) {
					if (err)
						tc->conn_close(err);
					return;
				}
			}

			if (tc->estabh)
				tc->estabh(tc->arg);
			StartReceiving();
		}
		else {
			DEBUG_WARNING("RunL: connecting (%d)\n",
				      iStatus.Int());
			tc->conn_close(ECONNREFUSED);
		}
	}
	/* Incoming DATA */
	else if (receiving) {
		if (iStatus == KErrEof)
			tc->conn_close(0);
		else
			tc->recv();
	}
	else {
		DEBUG_WARNING("CTcpConn RunL: bogus state\n");
	}
}


void CTcpConn::DoCancel()
{
	DEBUG_INFO("CTcpConn(%p) DoCancel\n", this);

	iSocket.CancelAll();
}


/****************************************************************************
 *                    Implementation of struct tcp_sock                     *
 ****************************************************************************/


/**
 * Create new blank socket (pending incoming connection)
 */
int tcp_sock::blank_socket()
{
	int ret;

	DEBUG_INFO("Creating new blank socket\n");

	if (ctc) {
		DEBUG_WARNING("blank socket: already created\n");
		return EALREADY;
	}

	ctc = new CTcpConn(this);
	if (!ctc)
		return ENOMEM;

	ctc->blank = true;

	/* Create a blank socket */
	ret = ctc->iSocket.Open(cts->iSockServer->iSocketServer);
	if (KErrNone != ret) {
		DEBUG_WARNING("create blank socket failed (ret=%d)\n", ret);
		goto error;
	}

	return 0;

 error:
	delete ctc;
	ctc = NULL;

	return kerr2errno(ret);
}


/**
 * Handle incoming connects
 */
void tcp_sock::tcp_conn_handler()
{
	if (!ctc) {
		DEBUG_WARNING("conn handler: no pending socket\n");
	}

	TInetAddr ia;
	struct sa peer;
	ctc->iSocket.RemoteName(ia);
	sa_set_in(&peer, ia.Address(), ia.Port());

	DEBUG_INFO("conn handler: incoming connect from %J\n", &peer);

	ctc->blank = false;

	/*
	 * Application handler might call tcp_accept(), tcp_reject()
	 * or do nothing
	 */
	if (connh)
		connh(&peer, arg);

	if (ctc) {
		DEBUG_INFO("delete ctc\n");
		delete ctc;
		ctc = NULL;
	}

	/* Create blank socket for the next incoming CONNECT */
	blank_socket();

	cts->Accepting();
}


/****************************************************************************
 *                    Implementation of struct tcp_conn                     *
 ****************************************************************************/


int tcp_conn::send(struct mbuf *mb)
{
	if (!ctc) {
		DEBUG_WARNING("send: internal error - no ctc\n");
		return EINVAL;
	}

	DEBUG_INFO("TCP Send %u bytes\n", mbuf_get_left(mb));

	/* call helpers in reverse order */
	int err = 0;
	struct le *le = list_tail(&helpers);
	while (le) {
		struct tcp_helper *th;

		th = (struct tcp_helper *)le->data;
		le = le->prev;

		if (th->sendh(&err, mb, th->arg) || err)
			return err;
	}

	const TPtrC8 buf_tx(mb->buf + mb->pos, mb->end - mb->pos);

	TRequestStatus stat;
	ctc->iSocket.Send(buf_tx, 0, stat);
	User::WaitForRequest(stat); /* TODO blocking */

	return kerr2errno(stat.Int());
}


void tcp_conn::recv()
{
	struct mbuf *mb;
	struct le *le;
	bool hlp_estab = false;
	int err = 0;

	DEBUG_INFO("TCP Receive %u bytes\n", ctc->iBufRx.Length());

	/* If no memory packet will be dropped */
	mb = mbuf_alloc(ctc->iBufRx.Length());
	if (mb) {
		(void)mbuf_write_mem(mb, (uint8_t *)ctc->iBufRx.Ptr(),
				     ctc->iBufRx.Length());
		mb->pos = 0;
	}

	ctc->StartReceiving();

	if (!mb)
		goto out;

	le = list_head(&helpers);
	while (le) {
		struct tcp_helper *th;
		bool hdld;

		th = (struct tcp_helper *)le->data;
		le = le->next;

		if (!hlp_estab)
		        hdld = th->recvh(&err, mb, &hlp_estab, arg);
		else
			hdld = th->estabh(&err, active, arg);

		if (hdld || err) {
			if (err)
				conn_close(err);
			goto out;
		}
	}

	mbuf_trim(mb);

	if (!hlp_estab) {
		if (recvh)
			recvh(mb, arg);
	}
	else {
		if (estabh)
			estabh(arg);
	}

 out:
	mem_deref(mb);
}


void tcp_conn::conn_close(int err)
{
	DEBUG_INFO("tcp conn close (%m)\n", err);

	/* Stop receiving */
	if (ctc->receiving) {
		ctc->iSocket.CancelRead();
		ctc->receiving = false;
	}

	if (closeh)
		closeh(err, arg);
}


/****************************************************************************
 *                            Exported functions                            *
 ****************************************************************************/


static void sock_destructor(void *data)
{
	struct tcp_sock *ts = (struct tcp_sock *)data;

	if (ts->ctc)
		delete ts->ctc;
	if (ts->cts)
		delete ts->cts;
}


static void conn_destructor(void *data)
{
	struct tcp_conn *tc = (struct tcp_conn *)data;

	list_flush(&tc->helpers);

	if (tc->ctc)
		delete tc->ctc;
}


static struct tcp_conn *conn_alloc(CTcpConn *ctc,
				   tcp_estab_h *eh, tcp_recv_h *rh,
				   tcp_close_h *ch, void *arg)
{
	struct tcp_conn *tc;

	tc = (struct tcp_conn *)mem_zalloc(sizeof(*tc), conn_destructor);
	if (!tc)
		return NULL;

	list_init(&tc->helpers);

	if (ctc) {
		DEBUG_INFO("conn_new: using ctc\n");
		tc->ctc = ctc;
	}
	else {
		tc->ctc = new CTcpConn(tc);
		if (!tc->ctc) {
			DEBUG_WARNING("conn_new: new CTcpConn failed\n");
			goto error;
		}
	}

	tc->rxsz   = TCP_RXSZ_DEFAULT;
	tc->estabh = eh;
	tc->recvh  = rh;
	tc->closeh = ch;
	tc->arg    = arg;

	return tc;

 error:
	return (struct tcp_conn *)mem_deref(tc);
}


int tcp_sock_alloc(struct tcp_sock **tsp, const struct sa *local,
		   tcp_conn_h *ch, void *arg)
{
	struct tcp_sock *ts;
	int err = 0;

	/* Local address is unused on Symbian */
	(void)local;

	if (!tsp)
		return EINVAL;

	ts = (struct tcp_sock *)mem_zalloc(sizeof(*ts), sock_destructor);
	if (!ts)
		return ENOMEM;

	ts->cts = new CTcpSock(ts);
	if (!ts->cts) {
		err = ENOMEM;
		goto out;
	}

	ts->connh = ch;
	ts->arg   = arg;

	*tsp = ts;

 out:
	if (err)
		mem_deref(ts);

	return err;
}


int tcp_sock_bind(struct tcp_sock *ts, const struct sa *local)
{
	if (!ts || !ts->cts || !local)
		return EINVAL;

	TInetAddr ia(sa_in(local), sa_port(local));

	return ts->cts->Bind(ia);
}


int tcp_sock_listen(struct tcp_sock *ts, int backlog)
{
	int err;

	if (!ts || !ts->cts)
		return EINVAL;

	err = ts->cts->Listen(backlog);
	if (err)
		goto out;

	err = ts->blank_socket();
	if (err)
		goto out;

	ts->cts->Accepting();

 out:
	return err;
}


int tcp_accept(struct tcp_conn **tcp, struct tcp_sock *ts, tcp_estab_h *eh,
	       tcp_recv_h *rh, tcp_close_h *ch, void *arg)
{
	struct tcp_conn *tc;

	if (!tcp || !ts)
		return EINVAL;

	DEBUG_INFO("tcp_accept() ts=%p\n", ts);

	tc = conn_alloc(ts->ctc, eh, rh, ch, arg);
	if (!tc)
		return ENOMEM;

	/* Transfer ownership to TCP connection */
	ts->ctc->tc = tc;
	ts->ctc = NULL;

	tc->ctc->StartReceiving();

	*tcp = tc;

	return 0;
}


void tcp_reject(struct tcp_sock *ts)
{
	if (!ts)
		return;

	delete ts->ctc;
	ts->ctc = NULL;
}


int tcp_conn_alloc(struct tcp_conn **tcp, const struct sa *peer,
		   tcp_estab_h *eh, tcp_recv_h *rh, tcp_close_h *ch,
		   void *arg)
{
	struct tcp_conn *tc;
	int err;

	if (!tcp || !sa_isset(peer, SA_ALL))
		return EINVAL;

	tc = conn_alloc(NULL, eh, rh, ch, arg);
	if (!tc)
		return ENOMEM;

	TInetAddr ia(sa_in(peer), sa_port(peer));

	err = tc->ctc->Open(ia);
	if (err)
		goto out;

	*tcp = tc;

 out:
	if (err) {
		DEBUG_WARNING("conn_alloc: %J (%m)\n", peer, err);
		mem_deref(tc);
	}
	return err;
}


int tcp_conn_bind(struct tcp_conn *tc, const struct sa *local)
{
	if (!tc || !tc->ctc || !local)
		return EINVAL;

	TInetAddr ia(sa_in(local), sa_port(local));

	return tc->ctc->Bind(ia);
}


int tcp_conn_connect(struct tcp_conn *tc, const struct sa *peer)
{
	if (!tc || !tc->ctc || !sa_isset(peer, SA_ALL))
		return EINVAL;

	tc->active = true;

	TInetAddr ia(sa_in(peer), sa_port(peer));

	return tc->ctc->Connect(ia);
}


int tcp_send(struct tcp_conn *tc, struct mbuf *mb)
{
	if (!tc || !mb)
		return EINVAL;

	return tc->send(mb);
}


int tcp_sock_local_get(const struct tcp_sock *ts, struct sa *local)
{
	if (!ts || !local)
		return EINVAL;

	TInetAddr ia;
	ts->cts->iSocket.LocalName(ia);
	sa_set_in(local, ia.Address(), ia.Port());

	return 0;
}


void tcp_conn_rxsz_set(struct tcp_conn *tc, size_t rxsz)
{
	if (!tc)
		return;

	tc->rxsz = rxsz;
}


int tcp_conn_local_get(const struct tcp_conn *tc, struct sa *local)
{
	if (!tc || !local)
		return EINVAL;

	TInetAddr ia;
	tc->ctc->iSocket.LocalName(ia);
	sa_set_in(local, ia.Address(), ia.Port());

	return 0;
}


int tcp_conn_peer_get(const struct tcp_conn *tc, struct sa *peer)
{
	if (!tc || !peer)
		return EINVAL;

	TInetAddr ia;
	tc->ctc->iSocket.RemoteName(ia);
	sa_set_in(peer, ia.Address(), ia.Port());

	return 0;
}


void tcp_rconn_set(RSocketServ *sockSrv, RConnection *rconn)
{
	socketServer = sockSrv;
	rconnection  = rconn;
}


int tcp_conn_fd(const struct tcp_conn *tc)
{
	(void)tc;
	return -1;
}


/****************************************************************************
 *                            TCP Helpers                                   *
 ****************************************************************************/


static void helper_destructor(void *data)
{
	struct tcp_helper *th = (struct tcp_helper *)data;

	list_unlink(&th->le);
}


static bool helper_estab_handler(int *err, bool active, void *arg)
{
	(void)err;
	(void)active;
	(void)arg;
	return false;
}


static bool helper_send_handler(int *err, struct mbuf *mb, void *arg)
{
	(void)err;
	(void)mb;
	(void)arg;
	return false;
}


static bool helper_recv_handler(int *err, struct mbuf *mb, bool *estab,
				void *arg)
{
	(void)err;
	(void)mb;
	(void)estab;
	(void)arg;
	return false;
}


static bool sort_handler(struct le *le1, struct le *le2, void *arg)
{
	struct tcp_helper *th1 = (struct tcp_helper *)le1->data;
	struct tcp_helper *th2 = (struct tcp_helper *)le2->data;
	(void)arg;

	return th1->layer <= th2->layer;
}


int tcp_register_helper(struct tcp_helper **thp, struct tcp_conn *tc,
			int layer,
			tcp_helper_estab_h *eh, tcp_helper_send_h *sh,
			tcp_helper_recv_h *rh, void *arg)
{
	struct tcp_helper *th;

	if (!tc)
		return EINVAL;

	th = (struct tcp_helper *)mem_zalloc(sizeof(*th), helper_destructor);
	if (!th)
		return ENOMEM;

	list_append(&tc->helpers, &th->le, th);

	th->layer  = layer;
	th->estabh = eh ? eh : helper_estab_handler;
	th->sendh  = sh ? sh : helper_send_handler;
	th->recvh  = rh ? rh : helper_recv_handler;
	th->arg = arg;

	list_sort(&tc->helpers, sort_handler, NULL);

	if (thp)
		*thp = th;

	return 0;
}
