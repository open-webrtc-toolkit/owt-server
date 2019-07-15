#ifndef SIP_UA_H_
#define SIP_UA_H_

#include <re/re_types.h>

#ifdef __cplusplus
extern "C" {
#endif


#define sipua_bool unsigned char
#define sipua_true 1
#define sipua_false 0

#define SIPUA_BOOL(b) (b ? sipua_true : sipua_false)
#define NATURAL_BOOL(b) (b ? true : false)

/******************/
/* Management:    */
/******************/
struct sipua_entity;
struct call;

int sipua_mod_init(void/*const char *dlpath*/);
void sipua_mod_close(void);

int sipua_new(struct sipua_entity **sipua, void *endpoint, const char *sip_server, const char * user_name,
	          const char *password, const char *disp_name);
void sipua_delete(struct sipua_entity *sipua);


/*****************/
/* signals:      */
/*****************/
/*endpoint -> sipua */
void sipua_call(struct sipua_entity *sipua, sipua_bool audio, sipua_bool video, const char *calleeURI);
void sipua_hangup(struct sipua_entity *sipua, void *call);
void sipua_accept(struct sipua_entity *sipua, const char *peer);
void sipua_set_call_owner(struct sipua_entity *sipua, void *call, void *call_owner);

/* sipua -> endpoint */
void ep_register_result(void *endpoint, sipua_bool successful);
int ep_incoming_call(void *endpoint, sipua_bool audio, sipua_bool video, const char *callerURI);
void ep_peer_ringing(void *endpoint, const char *peer);
void ep_call_closed(void *endpoint, const char *peer, const char *reason);
void ep_call_established(void *endpoint, const char *peer, void *call, const char *audio_dir, const char *video_dir);
void ep_call_updated(void *endpoint, const char *peer, const char *audio_dir, const char *video_dir);

//void ep_update_audio_params(void *endpoint, const char *cdcname, int srate, int ch, const char *fmtp);
//void ep_update_video_params(void *endpoint, const char *cdcname, int bitrate, int packetsize, int fps, const char *fmtp);
//
///******************/
///* Media:         */
///******************/
///* Attention: all media data bufs sent and received through sipua CONTAIN rtp-headers. */
///*endpoint -> sipua */
//void sipua_tx_audio(struct sipua_entity *sipua, uint8_t *data, size_t len);
//void sipua_tx_video(struct sipua_entity *sipua, uint8_t *data, size_t len);
//void sipua_fir(struct sipua_entity *sipua);
///* sipua -> endpoint */
//void ep_rx_audio(void *ep, uint8_t *data, size_t len);
//void ep_rx_video(void *ep, uint8_t *data, size_t len);


#ifdef __cplusplus
}
#endif

#endif
