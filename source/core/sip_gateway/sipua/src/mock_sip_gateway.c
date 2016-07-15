#include "sipua.h"
#include <stdio.h>


void ep_register_result(void *gateway, sipua_bool successful)
{
	(void)gateway;
	(void)successful;
}

int ep_incoming_call(void *gateway, sipua_bool audio, sipua_bool video, const char *callerURI)
{
	(void)gateway;
	(void)audio;
	(void)video;
	(void)callerURI;

	return -1;
}


void ep_peer_ringing(void *gateway)
{
    (void)gateway;
}

void ep_call_closed(void *gateway, const char *reason)
{
	(void)gateway;
	(void)reason;
}

void ep_call_established(void *gateway)
{
	(void)gateway;
}

void ep_update_audio_params(void *gateway, const char *cdcname, int srate, int ch, const char *fmtp)
{
	(void)gateway;
	(void)cdcname;
	(void)srate;
	(void)ch;
	(void)fmtp;
}

void ep_update_video_params(void *gateway, const char *cdcname, int bitrate, int packetsize, int fps, const char *fmtp)
{
	(void)gateway;
	(void)cdcname;
	(void)bitrate;
	(void)packetsize;
	(void)fps;
	(void)fmtp;
}


void ep_rx_audio(void *gateway, uint8_t *data, size_t len)
{
	(void)gateway;
	(void)data;
	(void)len;
	/*info("a[%d]", len);*/
}

void ep_rx_video(void *gateway, uint8_t *data, size_t len)
{
	(void)gateway;
	(void)data;
	(void)len;
	/*info("v[%d]", len);*/
}
