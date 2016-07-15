/**
 * @file udp.cpp  User Datagram Protocol for Symbian
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
#include <re_udp.h>


#define DEBUG_MODULE "udp"
#define DEBUG_LEVEL 5
#include <re_dbg.h>
}


enum {
	UDP_RXSZ_DEFAULT = 8192
};


class CUdpSocket : public CActive {
public:
	CUdpSocket(struct udp_sock *us);
	~CUdpSocket();
	int  Open();
	void StartReceiving();

protected:
	void RunL();
	void DoCancel();

public:
	RSocketServ iSocketServer;      /**< Socket Server     */
	RSocket iSocket;                /**< Socket object     */
	TBuf8<UDP_RXSZ_DEFAULT> iBufRx; /**< Receive buffer    */
	TInetAddr iSource;              /**< Source IP address */
	struct udp_sock *us;            /**< Parent UDP socket */
};


struct udp_sock {
public:
	int  listen(const struct sa *local);
	int  send(const struct sa *dst, struct mbuf *mb);
	void recv();
	int  local_get(struct sa *local) const;

public:
	struct list helpers;
	CUdpSocket *cus;    /**< UDP Socket object           */
	udp_recv_h *rh;     /**< Receive handler             */
	void *arg;          /**< Handler argument            */
	bool conn;          /**< Connected socket flag       */
	size_t rxsz;        /**< Maximum receive chunk size  */
	size_t rx_presz;    /**< Preallocated rx buffer size */
	int cerr;           /**< Cached error code           */
};


struct udp_helper {
	struct le le;
	int layer;
	udp_helper_send_h *sendh;
	udp_helper_recv_h *recvh;
	void *arg;
};


static RSocketServ *socketServer = NULL;
static RConnection *rconnection = NULL;


CUdpSocket::CUdpSocket(struct udp_sock *us)
	:CActive(EPriorityHigh)
	,us(us)
{
	CActiveScheduler::Add(this);
}


CUdpSocket::~CUdpSocket()
{
	Cancel();
	iSocket.Close();

	/* Only close the socket server if we own it */
	if (!socketServer)
		iSocketServer.Close();
}


int CUdpSocket::Open()
{
	TInt err;

	if (socketServer) {
		DEBUG_INFO("UDPsock(%p): Using RSocketServ=%p (handle=%d)\n",
			   us, socketServer, socketServer->Handle());

		iSocketServer = *socketServer;
	}
	else {
		err = iSocketServer.Connect();
		if (KErrNone != err) {
			DEBUG_WARNING("iSocketServer.Connect failed\n");
			return kerr2errno(err);
		}
	}

	if (rconnection) {
		DEBUG_INFO("UDPsock: Using RConnection=%p\n", rconnection);
		err = iSocket.Open(iSocketServer, KAfInet, KSockDatagram,
				   KProtocolInetUdp, *rconnection);
	}
	else {
		err = iSocket.Open(iSocketServer, KAfInet, KSockDatagram,
				   KProtocolInetUdp);
	}
	if (KErrNone != err) {
		DEBUG_WARNING("iSocket.Open failed (err=%d)\n", err);
		return kerr2errno(err);
	}

	return 0;
}


void CUdpSocket::StartReceiving()
{
	/* Clear buffer */
	iBufRx.Zero();

	if (!IsActive())
		SetActive();

	iSocket.RecvFrom(iBufRx, iSource, 0, iStatus);
}


void CUdpSocket::RunL()
{
	DEBUG_INFO("RunL iStatus=%d\n", iStatus.Int());

	/* This happens after calling cancel functions in socket */
	if (iStatus == KErrCancel || iStatus == KErrAbort) {
		iStatus = KErrNone;
	}
	else if (iStatus == KErrGeneral) {
		/* cache error code */
		us->cerr = ECONNREFUSED;
	}
	else if (iStatus != KErrNone) {
		DEBUG_WARNING("RunL received iStatus %d\n", iStatus.Int());
	}
	else if (iBufRx.Length()) {
		us->recv();
	}
}


void CUdpSocket::DoCancel()
{
	iSocket.CancelAll();
}


/****************************************************************************/


int udp_sock::listen(const struct sa *local)
{
	TInetAddr ia(sa_in(local), sa_port(local));

	const TInt ret = cus->iSocket.Bind(ia);
	if (KErrNone != ret)
		return kerr2errno(ret);

	cus->StartReceiving();

	return 0;
}


int udp_sock::send(const struct sa *dst, struct mbuf *mb)
{
	struct sa hdst;
	TRequestStatus stat;
	int err = 0;

	DEBUG_INFO("udp_sock::send %u bytes to %J\n", mbuf_get_left(mb), dst);

	/* Check for error in e.g. connected state */
	if (cerr) {
		err = cerr;
		cerr = 0; /* clear error */
		return err;
	}

	/* call helpers in reverse order */
	struct le *le = list_tail(&helpers);
	while (le) {
		struct udp_helper *uh;

		uh = (struct udp_helper *)le->data;
		le = le->prev;

		if (dst != &hdst) {
			sa_cpy(&hdst, dst);
			dst = &hdst;
		}

		if (uh->sendh(&err, &hdst, mb, uh->arg) || err)
			return err;
	}

	TInetAddr ia(sa_in(dst), sa_port(dst));

	const TPtrC8 buf_tx(mb->buf + mb->pos, mb->end - mb->pos);

	if (conn) {
		cus->iSocket.Connect(ia, stat);
		User::WaitForRequest(stat);

		if (KErrNone != stat.Int()) {
			DEBUG_WARNING("udp_sock::send Connect: kerr=%d\n",
				      stat.Int());
			if (KErrGeneral == stat.Int())
				return ECONNREFUSED;
		}

#if 0
		/* TODO: Cause Access-Violation in WINS emulator! */
		cus->iSocket.Send(buf_tx, 0, stat);
#else
		cus->iSocket.SendTo(buf_tx, ia, 0, stat);
#endif
	}
	else {
		cus->iSocket.SendTo(buf_tx, ia, 0, stat);
	}

	User::WaitForRequest(stat);

	return kerr2errno(stat.Int());
}


void udp_sock::recv()
{
	struct mbuf *mb = mbuf_alloc(rx_presz + cus->iBufRx.Length());
	if (!mb)
		return;

	mb->pos += rx_presz;
	mb->end += rx_presz;

	(void)mbuf_write_mem(mb, (uint8_t *)cus->iBufRx.Ptr(),
			     cus->iBufRx.Length());
	mb->pos = rx_presz;

	struct sa src;
	sa_set_in(&src, cus->iSource.Address(), cus->iSource.Port());

	/* call helpers */
	struct le *le = list_head(&helpers);
	while (le) {
		struct udp_helper *uh;
		bool hdld;

		uh = (struct udp_helper *)le->data;
		le = le->next;

		hdld = uh->recvh(&src, mb, uh->arg);

		if (hdld)
			goto out;
	}

	DEBUG_INFO("udp recv %d bytes from %J\n", cus->iBufRx.Length(), &src);

	cus->StartReceiving();

	rh(&src, mb, arg);

 out:
	mem_deref(mb);
}


int udp_sock::local_get(struct sa *local) const
{
	TInetAddr ia;

	cus->iSocket.LocalName(ia);
	sa_set_in(local, ia.Address(), ia.Port());

	return 0;
}


/****************************************************************************/


static void udp_destructor(void *data)
{
	struct udp_sock *us = (struct udp_sock *)data;

	list_flush(&us->helpers);

	delete us->cus;
}


static void dummy_udp_recv_handler(const struct sa *src,
				   struct mbuf *mb, void *arg)
{
	(void)src;
	(void)mb;
	(void)arg;
}


int udp_listen(struct udp_sock **usp, const struct sa *local,
	       udp_recv_h *rh, void *arg)
{
	struct udp_sock *us = (struct udp_sock *)NULL;
	int err;

	if (!usp)
		return EINVAL;

	us = (struct udp_sock *)mem_zalloc(sizeof(*us), udp_destructor);
	if (!us)
		return ENOMEM;

	us->cus = new CUdpSocket(us);
	if (!us->cus) {
		err = ENOMEM;
		goto out;
	}

	err = us->cus->Open();
	if (err)
		goto out;

	us->rh  = rh ? rh : dummy_udp_recv_handler;
	us->arg = arg;

	err = us->listen(local);
	if (err)
		goto out;

	*usp = us;

 out:
	if (err)
		mem_deref(us);
	return err;
}


void udp_connect(struct udp_sock *us, bool conn)
{
	if (!us)
		return;

	us->conn = conn;
}


int udp_send(struct udp_sock *us, const struct sa *dst, struct mbuf *mb)
{
	if (!us || !dst || !mb)
		return EINVAL;

	DEBUG_INFO("udp_send: US %u bytes to %J\n", mb->end, dst);

	return us->send(dst, mb);
}


int udp_local_get(const struct udp_sock *us, struct sa *local)
{
	if (!us || !local)
		return EINVAL;

	return us->local_get(local);
}


int udp_setsockopt(struct udp_sock *us, int level, int optname,
		   const void *optval, uint32_t optlen)
{
	(void)us;
	(void)level;
	(void)optname;
	(void)optval;
	(void)optlen;

	return ENOSYS; /* not mapped to Symbian sockets */

#if 0
	/* TODO: use this. see:
	  http://www.forum.nokia.com/document/Forum_Nokia_Technical_
	  Library/contents/FNTL/Increasing_WLAN_power_efficiency_for
	  _full-duplex_VoIP_and_Video_applications.htm
	 */
	iSocket.SetOpt( KSoIpTOS, KProtocolInetIp, 0xe0 );
#endif
}


int udp_sockbuf_set(struct udp_sock *us, int size)
{
	(void)us;
	(void)size;
	return ENOSYS;
}


void udp_rxsz_set(struct udp_sock *us, size_t rxsz)
{
	if (!us)
		return;

	us->rxsz = rxsz;
}


/**
 * Set preallocated space on receive buffer.
 *
 * @param us       UDP Socket
 * @param rx_presz Size of preallocate space.
 */
void udp_rxbuf_presz_set(struct udp_sock *us, size_t rx_presz)
{
	if (!us)
		return;

	us->rx_presz = rx_presz;
}


/**
 * Set receive handler on a UDP Socket
 *
 * @param us  UDP Socket
 * @param rh  Receive handler
 * @param arg Handler argument
 */
void udp_handler_set(struct udp_sock *us, udp_recv_h *rh, void *arg)
{
	if (!us)
		return;

	us->rh  = rh ? rh : dummy_udp_recv_handler;
	us->arg = arg;
}


int udp_sock_fd(const struct udp_sock *us, int af)
{
	(void)us;
	(void)af;

	return ENOSYS;
}


void udp_rconn_set(RSocketServ *sockSrv, RConnection *rconn)
{
	socketServer = sockSrv;
	rconnection = rconn;
}


int udp_send_anon(const struct sa *dst, struct mbuf *mb)
{
	struct udp_sock *us;
	int err;

	if (!dst || !mb)
		return EINVAL;

	err = udp_listen(&us, NULL, NULL, NULL);
	if (err)
		return err;

	err = udp_send(us, dst, mb);
	mem_deref(us);

	return err;
}


static void helper_destructor(void *data)
{
	struct udp_helper *uh = (struct udp_helper *)data;

	list_unlink(&uh->le);
}


static bool helper_send_handler(int *err, struct sa *dst,
				struct mbuf *mb, void *arg)
{
	(void)err;
	(void)dst;
	(void)mb;
	(void)arg;
	return false;
}


static bool helper_recv_handler(struct sa *src, struct mbuf *mb, void *arg)
{
	(void)src;
	(void)mb;
	(void)arg;
	return false;
}


static bool sort_handler(struct le *le1, struct le *le2, void *arg)
{
	struct udp_helper *uh1 = (struct udp_helper *)le1->data;
	struct udp_helper *uh2 = (struct udp_helper *)le2->data;
	(void)arg;

	return uh1->layer <= uh2->layer;
}


int udp_register_helper(struct udp_helper **uhp, struct udp_sock *us,
			int layer,
			udp_helper_send_h *sh, udp_helper_recv_h *rh,
			void *arg)
{
	struct udp_helper *uh;

	if (!us)
		return EINVAL;

	uh = (struct udp_helper *)mem_zalloc(sizeof(*uh), helper_destructor);
	if (!uh)
		return ENOMEM;

	list_append(&us->helpers, &uh->le, uh);

	uh->layer = layer;
	uh->sendh = sh ? sh : helper_send_handler;
	uh->recvh = rh ? rh : helper_recv_handler;
	uh->arg   = arg;

	list_sort(&us->helpers, sort_handler, NULL);

	if (uhp)
		*uhp = uh;

	return 0;
}
