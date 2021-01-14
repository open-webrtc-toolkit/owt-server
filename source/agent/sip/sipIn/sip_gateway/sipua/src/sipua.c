#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <re/re.h>
#include <sipua.h>
#include <baresip.h>
#include "core.h"
#include "sipua.h"
#include "sipua_actions.h"


#define DEBUG_MODULE "sipua"
#define DEBUG_LEVEL 7
#include <re/re_dbg.h>


struct sipua_entity {         /* sip ua entity */
	void *ep;
	struct le le;             /**< Linked list element                */
	struct mqueue *mq;
	struct uag *uag;
	pthread_t     thid;
};

static pthread_t dnsc_thid;

/**************** sipua  management ***************/

static sipua_bool sipua_prefer_ipv6 = false;
static sipua_bool sipua_trans_udp = true;
static sipua_bool sipua_trans_tcp = false;
static sipua_bool sipua_trans_tls = false;

static struct list sipual;               /**< List of SIP UAs(struct ua_entity) */

struct new_sipua_th_paras{
	int    pfd[2];
	void *ep;
	/*struct uag_cfg cfg;*/
	char sip_server[128];
	char user_name[64];
	char password[64];
	char disp_name[64];
};

struct new_sipua_rslt{
	struct mqueue *mq;
	struct uag    *uag;
};

/**/
static int construct_uag(struct uag **uagp, void *ep, const char *sip_server, const char *user_name,
	                     const char *password, const char *disp_name)
{
	int err;
	char account_str[512]={0};

    sprintf(account_str, "%.128s <sip:%.128s:%.64s@%.128s>\n", disp_name, user_name, password, sip_server);

	err = uag_alloc(uagp, "Intel Integrated SIPUA", ep, sipua_trans_udp ? true : false, sipua_trans_tcp ? true : false, sipua_trans_tls ? true : false, sipua_prefer_ipv6 ? true : false);
	if (err){
		*uagp = NULL;
		goto out;
	}

	err = ua_alloc(NULL, account_str, *uagp);
	if (err){
		mem_deref(*uagp);
		*uagp = NULL;
		goto out;
	}


out:
	return err;
}

static void *sipua_run(void *arg)
{
	struct new_sipua_th_paras *params = (struct new_sipua_th_paras *)arg;
	struct new_sipua_rslt rslt = {NULL, NULL};
	int err1 = 0, err2 = 0, n = 0;

	re_thread_init();
	fd_setsize(8192);
	err1 = construct_uag(&rslt.uag, params->ep, params->sip_server, params->user_name, params->password, params->disp_name);
	if (err1) {
		warning("!!construct_uag failed.\n");
		write(params->pfd[1], &rslt, sizeof(struct new_sipua_rslt));
		goto out;
    }

    err2 = mqueue_alloc(&rslt.mq, sipua_cmd_handler, rslt.uag);
    if (err2) {
    	warning("!!mqueue_alloc failed.\n");
		write(params->pfd[1], &rslt, sizeof(struct new_sipua_rslt));
		goto out;
    }

	n = write(params->pfd[1], &rslt, sizeof(struct new_sipua_rslt));
	if (n < 0) {
		warning("!!pipeline write failed.\n");
		goto out;
	}

	re_main(NULL);

out:
    /* clean up/mem_deref all state */
    if (rslt.mq)
        mem_deref(rslt.mq);
    if (rslt.uag)
    	mem_deref(rslt.uag);

    re_thread_close();
    return NULL;
}

static void sipua_destructor(void *arg)
{
	struct sipua_entity *sipua = arg;

        if (sipua->mq) {
		mqueue_push(sipua->mq, SIPUA_TERMINATE, NULL);
	}

	list_unlink(&sipua->le);
}


static void *dnsc_run(void *arg)
{
	(void)arg;

	re_thread_init();
	fd_setsize(8192);
	net_reset();
	re_main(NULL);
	re_thread_close();
	return NULL;
}

static int dnsc_new(void)
{
	return pthread_create(&dnsc_thid, NULL, dnsc_run, NULL);
}

static void dnsc_delete(void)
{
	pthread_join(dnsc_thid, NULL);
}

int sipua_new(struct sipua_entity **sipuap, void *endpoint, const char *sip_server, const char * user_name,
	          const char *password, const char *disp_name)
{
    struct sipua_entity *sipua = NULL;
	pthread_t            thread;
    int                  err = 0, n = 0;
    struct new_sipua_th_paras  params = {{-1, -1}, NULL, {0}, {0}, {0}, {0}};
    struct new_sipua_rslt      rslt = {NULL, NULL};

    if (!endpoint) {
    	err = -1;
    	goto out;
    }

    sipua = mem_zalloc(sizeof(struct sipua_entity), sipua_destructor);

    if (sipua == NULL || pipe(params.pfd) < 0){
    	goto out;
    }

    params.ep = endpoint;
    /*memcpy((void *)&params.cfg, (void *)cfg, sizeof(struct uag_cfg));*/
    strncpy(params.sip_server, sip_server, 127);
    strncpy(params.user_name, user_name, 63);
    strncpy(params.password, password, 63);
    strncpy(params.disp_name, disp_name, 63);

	pthread_create(&thread, NULL, sipua_run, (void *)&params);
	n = read(params.pfd[0], &rslt, sizeof(struct new_sipua_rslt));
	if (params.pfd[0] >= 0) {
		fd_close(params.pfd[0]);
		(void)close(params.pfd[0]);
	}
	if (params.pfd[1] >= 0)
		(void)close(params.pfd[1]);

	/*sipua->thid = thread;*/
	sipua->mq = rslt.mq;
	sipua->uag = rslt.uag;
	sipua->thid = thread;

out:
    if (n < 0 || sipua == NULL || sipua->mq == NULL || sipua->uag == NULL){
    	err = -1;
    	mem_deref(sipua);
    }else if (sipuap){
    	*sipuap = sipua;
    	list_append(&sipual, &sipua->le, sipua);
    }

    return err;
}

void sipua_delete(struct sipua_entity *sipua)
{
	pthread_t thread = sipua->thid;
	mem_deref(sipua);
	pthread_join(thread, NULL);
}

int sipua_mod_init(void/*const char *dlpath*/)
{
    int err = 0;

	(void)sys_coredump_set(true);

	err = libre_init();
	if (err)
		goto out;

    err = conf_init();
    if (err) {
		warning("sipua: configuration init: %m\n", err);
		goto out;
	}

	err = load_modules(/*dlpath*/);
    if (err) {
		warning("sipua: configuration init: %m\n", err);
		goto out;
	}

	list_init(&sipual);

    err = dnsc_new();
	if (err) {
		warning("sipua: dnsc_new failed: %m\n", err);
		goto out;
	}

	log_enable_debug(true);

	info("sipua is ready.\n");

 out:
	return err;
}

int usleep(unsigned long usec);
void sipua_mod_close(void){
	list_flush(&sipual);
	unload_modules();

    dnsc_delete();
    net_close();
	libre_close();

    usleep(40000);
	/* Check for memory leaks */
	tmr_debug();
	mem_debug();
}

/******* signals *******/
void sipua_call(struct sipua_entity *sipua, sipua_bool audio, sipua_bool video, const char *calleeURI)
{
	struct sipua_call_data *call_data = NULL;

	if (!sipua || !sipua->mq) {
		warning("sipua entity NULL!\n");
		return;
	}

	call_data = mem_zalloc(sizeof(struct sipua_call_data), NULL);
	call_data->audio = NATURAL_BOOL(audio);
	call_data->video = NATURAL_BOOL(video);
	strncpy(call_data->calleeURI, calleeURI, sizeof(call_data->calleeURI));

	mqueue_push(sipua->mq, SIPUA_CALL, call_data);
	return;
}

void sipua_hangup(struct sipua_entity *sipua, void* call)
{
	if (!sipua || !sipua->mq) {
		warning("sipua entity NULL!\n");
		return;
	}

	mqueue_push(sipua->mq, SIPUA_HANGUP, call);
	return;
}

void sipua_accept(struct sipua_entity *sipua, const char* peer)
{
	if (!sipua || !sipua->mq) {
		warning("sipua entity NULL!\n");
		return;
	}

	mqueue_push(sipua->mq, SIPUA_ANSWER, (void*)peer);
	return;
}

void sipua_set_call_owner(struct sipua_entity *sipua, void *call, void *callowner)
{
       struct sipua_call_connect * data = NULL;
       //FIX ME, lock
	if (!sipua || !sipua->mq) {
		warning("sipua entity NULL!\n");
		return;
	}
	data = mem_zalloc(sizeof(struct sipua_call_connect), NULL);
	data->call = call;
	data->owner = callowner;

        if (callowner) {
	    mqueue_push(sipua->mq, SIPUA_CALL_CONNECT, data);
        } else {
	    mqueue_push(sipua->mq, SIPUA_CALL_DISCONNECT, data);
        }
	return;
}

/******* media data *******/
/*
static void tx_buf(struct sipua_entity *sipua, uint8_t *data, size_t len, enum sipua_cmd_code cmd_code)
{
	struct sipua_tx_rtpbuf *txbuf = NULL;

	if (!(cmd_code == SIPUA_TX_AUD || cmd_code == SIPUA_TX_VID)) {
		warning("incorrect command code!\n");
		return;
	}

	if (!sipua || !sipua->mq) {
		warning("sipua entity NULL!\n");
		return;
	}
    
    txbuf = mem_zalloc(sizeof(struct sipua_tx_rtpbuf), NULL);
    txbuf->data = data;
    txbuf->len = len;

	mqueue_push(sipua->mq, cmd_code, txbuf);
	return;
}
*/

