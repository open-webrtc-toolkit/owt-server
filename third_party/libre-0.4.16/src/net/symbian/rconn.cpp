/**
 * @file rconn.cpp  Networking connections for Symbian OS
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <e32std.h>
#include <commdbconnpref.h>
#include <es_sock.h>


extern "C" {
#include <re_types.h>
#include <re_sa.h>
#include <re_net.h>
#include <re_udp.h>
#include <re_tcp.h>

#define DEBUG_MODULE "rconn"
#define DEBUG_LEVEL 5
#include <re_dbg.h>
}


class CRconn : public CActive
{
public:
	static CRconn *NewL(net_conn_h *ch);
	~CRconn();

	TInt Start(TUint32 aIapId, TBool aPrompt);
	void Stop();

private:
	CRconn(net_conn_h *ch);
	void ConstructL();

	/* from CActive */
	virtual void RunL();
	virtual void DoCancel();

private:
	RSocketServ iServ;
	RConnection iConn;
	net_conn_h *ch;
};


static CRconn *rconn = NULL;


CRconn *CRconn::NewL(net_conn_h *ch)
{
	CRconn *c = new (ELeave) CRconn(ch);
	CleanupStack::PushL(c);
	c->ConstructL();
	CleanupStack::Pop(c);
	return c;
}


CRconn::~CRconn()
{
	DEBUG_INFO("~CRconn\n");
	udp_rconn_set(NULL, NULL);
	tcp_rconn_set(NULL, NULL);
	Stop();

	iConn.Close();
	iServ.Close();
}


CRconn::CRconn(net_conn_h *ch)
	:CActive(EPriorityStandard)
	,ch(ch)
{
	CActiveScheduler::Add(this);
}


void CRconn::ConstructL()
{
	TInt err;

	User::LeaveIfError(iServ.Connect());

	err = iConn.Open(iServ);
	if (KErrNone != err) {
		DEBUG_WARNING("iConn.Open err=%d\n", err);
		User::Leave(err);
	}
}


void CRconn::RunL()
{
	TUint count;
	TInt ret;

	DEBUG_INFO("CRconn::RunL err=%d\n", iStatus.Int());

	if (KErrNone == iStatus.Int()) {
		udp_rconn_set(&iServ, &iConn);
		tcp_rconn_set(&iServ, &iConn);
	}

	/* Get the IAP id of the underlying interface of this RConnection */
	TUint32 iapId = 0;
	ret = iConn.GetIntSetting(_L("IAP\\Id"), iapId);
	if (KErrNone == ret) {
		DEBUG_INFO("RConn: Using IAP=%u\n", iapId);
	}

	ret = iConn.EnumerateConnections(count);
	if (KErrNone != ret) {
		DEBUG_WARNING("EnumerateConnections: ret=%d\n", ret);
	}
	else {
		DEBUG_INFO("Enumerated connections: %u\n", count);
	}

	ch(kerr2errno(iStatus.Int()), iapId);
}


void CRconn::DoCancel()
{
}


/**
 * Connect to the network using this IAP ID
 */
TInt CRconn::Start(TUint32 aIapId, TBool aPrompt)
{
	DEBUG_INFO("CRconn::Start: IAP id %u\n", aIapId);

	Cancel();

	TCommDbConnPref prefs;
	prefs.SetIapId(aIapId);
	if (aPrompt)
		prefs.SetDialogPreference(ECommDbDialogPrefPrompt);
	else
		prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
	prefs.SetDirection(ECommDbConnectionDirectionOutgoing);

	iConn.Start(prefs, iStatus);
	SetActive();

	return 0;
}


void CRconn::Stop()
{
	DEBUG_INFO("CRconn::Stop\n");

	Cancel();
	const TInt err = iConn.Stop();
	if (err) {
		DEBUG_INFO("CRconn::Stop err=%d\n", err);
	}
}


int net_conn_start(net_conn_h *ch, uint32_t id, bool prompt)
{
	DEBUG_NOTICE("rconn_start: iap_id=%u prompt=%d\n", id, prompt);

	if (rconn)
		return 0;

	TRAPD(ret, rconn = CRconn::NewL(ch));
	if (KErrNone != ret)
		return kerr2errno(ret);

	return kerr2errno(rconn->Start(id, prompt));
}


void net_conn_stop(void)
{
	if (!rconn)
		return;

	delete rconn;
	rconn = NULL;
}
