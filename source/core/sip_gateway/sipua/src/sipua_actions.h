#ifndef SIPUA_ACTIONS_H_
#define SIPUA_ACTIONS_H_


enum sipua_cmd_code {
	SIPUA_TERMINATE = 1,
	SIPUA_CALL,
	SIPUA_HANGUP,
	SIPUA_ANSWER,
	SIPUA_TX_AUD,
	SIPUA_TX_VID,
        SIPUA_CALL_CONNECT
};

struct sipua_call_data{
	bool audio;
	bool video;
	const char *calleeURI;
};

struct sipua_tx_rtpbuf {
	uint8_t *data;
	size_t len;
};

struct sipua_call_connect {
       void *call;
       void *owner;
};

void sipua_cmd_handler(int id, void *data, void *arg);

#endif
