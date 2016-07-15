/**
 * @file actsched.cpp  Symbian Active Scheduler
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <e32base.h>
#include <stdio.h>
#include "../main.h"

extern "C" {
#include <re_types.h>
#include <re_list.h>
#include <re_tmr.h>

#define DEBUG_MODULE "actsched"
#define DEBUG_LEVEL 5
#include <re_dbg.h>

extern struct list *tmrl_get(void);
}


class CMyTimer : public CTimer
{
public:
	static CMyTimer *NewL(TInt aPriority);
	void Start();

protected:
	CMyTimer(TInt aPriority);
	virtual void RunL();
};

static CMyTimer *t;
static CActiveScheduler as;
static bool inited = false;
static bool running = false;


CMyTimer *CMyTimer::NewL(TInt aPriority)
{
	CMyTimer *t = new CMyTimer(aPriority);
	t->ConstructL();
	return t;
}


CMyTimer::CMyTimer(TInt aPriority)
	:CTimer(aPriority)
{
	CActiveScheduler::Add(this);
}


void CMyTimer::Start()
{
	Cancel();

	/*
	  CTimer range is +-2147483647, which is +-35 minutes, 47 seconds.
	*/
	uint64_t to = tmr_next_timeout(tmrl_get());
	if (to) {
		if (to >= 2147482) {
			to = 2147482;
		}
		After(1000*to);
	}
	else {
		DEBUG_INFO("idle.. no timers\n");
	}
}


void CMyTimer::RunL()
{
	tmr_poll(tmrl_get());

	if (!t)
		return;

	Start();
}


void actsched_init(void)
{
	DEBUG_INFO("actsched init (inited=%d)\n", inited);

	if (inited)
		return;

	inited = true;

	/* Setup the Active Scheduler */
	if (CActiveScheduler::Current()) {
		DEBUG_INFO("Active Scheduler already installed\n");
	}
	else {
		DEBUG_INFO("Installing new Active Scheduler\n");
		CActiveScheduler::Install(&as);
	}
}


int actsched_start(void)
{
	actsched_init();

	DEBUG_INFO("actsched start: (running=%d)\n", running);

	if (running) {
		DEBUG_WARNING("actsched start: already running\n");
		return EALREADY;
	}

	running = true;

	if (!t) {
		t = CMyTimer::NewL(0);
		t->After(1000);
	}

	/* Main loop */
	if (CActiveScheduler::Current() == &as) {
		DEBUG_INFO("Starting own Active Scheduler\n");
		CActiveScheduler::Start(); /* loop here */
		running = false;
	}

	return 0;
}


void actsched_stop(void)
{
	DEBUG_INFO("actsched stop (inited=%d)\n", inited);

	if (!running)
		return;

	/* Must be called from same thread as ::Start() */
	if (CActiveScheduler::Current() == &as) {
		DEBUG_INFO("Stopping own Active Scheduler\n");
		CActiveScheduler::Stop();
	}

	if (t) {
		delete t;
		t = NULL;
	}

	inited = false;
	running = false;
}


/*
 * Called from tmr_start() when a new timer is added.
 *
 * TODO: this is a hack; consider moving it to tmr.c
 */
void actsched_restart_timer(void)
{
	if (!t)
		return;

	t->Start();
}
