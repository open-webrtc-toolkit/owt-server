#include <re/re.h>
#include <baresip.h>
#include <sipua.h>
#include "core.h"
#include "sipua_actions.h"


static void sipua_do_call(void *data, void *arg)
{
	int err = -1;
	struct uag *uag = (struct uag *)arg;
	struct sipua_call_data *call_data = (struct sipua_call_data *)data;

	err = ua_connect(uag_current(uag), NULL, NULL, call_data->calleeURI, NULL, call_data->video ? VIDMODE_ON : VIDMODE_OFF);
	if (err) {
		ep_call_closed((void *)uag->ep, call_data->calleeURI, "ua_connect failed");
	}

	mem_deref(call_data);
	return;
}

static void sipua_do_hangup(void *data, void *arg)
{
	struct uag *uag = (struct uag *)arg;
	(void)data;

	ua_hangup(uag_current(uag), NULL, 0, NULL);
	return;
}

static void sipua_do_answer(void *data, void *arg)
{
	struct uag *uag = (struct uag *)arg;
	(void)data;

	ua_answer(uag_current(uag), NULL);
	return;
}

static void sipua_do_call_connect(void *data, void *arg)
{
	struct sipua_call_connect *connect = (struct sipua_call_connect *)data;
        (void)arg;
        call_set_owner((struct call*)(connect->call), connect->owner);
	mem_deref(connect);
	return;
}

static void sipua_do_call_disconnect(void *data, void *arg)
{
	struct sipua_call_connect *connect = (struct sipua_call_connect *)data;
        (void)arg;
        call_set_owner((struct call*)(connect->call), NULL);
	mem_deref(connect);
	return;
}

static void sipua_do_tx_audio(void *data, void *arg)
{
	struct sipua_tx_rtpbuf *tx_aud = (struct sipua_tx_rtpbuf *)data;
	struct uag *uag = (struct uag *)arg;

	audio_send(call_audio(ua_call(uag_current(uag))), tx_aud->data, tx_aud->len);

	mem_deref(tx_aud);
	return;
}

static void sipua_do_tx_video(void *data, void *arg)
{
	struct sipua_tx_rtpbuf *tx_vid = (struct sipua_tx_rtpbuf *)data;
	struct uag *uag = (struct uag *)arg;

	video_send(call_video(ua_call(uag_current(uag))), tx_vid->data, tx_vid->len);

	mem_deref(tx_vid);
	return;
}

/** This function is running in the sipua thread. **/
void sipua_cmd_handler(int id, void *data, void *arg)
{
	/*struct uag *uag = (struct uag *)arg;*/
	(void)arg;
	(void)data;

	switch (id){
		case SIPUA_TERMINATE:
			re_cancel();
		    break;
		case SIPUA_CALL:
		    sipua_do_call(data, arg);
		    break;
		case SIPUA_HANGUP:
                    sipua_do_hangup(data, arg);
		    break;
		case SIPUA_ANSWER:
		    sipua_do_answer(data, arg);
		    break;
		case SIPUA_TX_AUD:
		    sipua_do_tx_audio(data, arg);
		    break;
		case SIPUA_TX_VID:
		    sipua_do_tx_video(data, arg);
		    break;
                case SIPUA_CALL_CONNECT:
                    sipua_do_call_connect(data, arg);
                    break;
                case SIPUA_CALL_DISCONNECT:
                    sipua_do_call_disconnect(data, arg);
                    break;
		default:
		    printf("\n Unknown cmd code");
		    break;
	}
	return;
}
