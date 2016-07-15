/**
 * @file re_telev.h  Interface to Telephony Events (RFC 4733)
 *
 * Copyright (C) 2010 Creytiv.com
 */

enum {
	TELEV_PTIME = 50,
	TELEV_SRATE = 8000
};

struct telev;

extern const char telev_rtpfmt[];

int telev_alloc(struct telev **tp, uint32_t ptime);
int telev_send(struct telev *tel, int event, bool end);
int telev_recv(struct telev *tel, struct mbuf *mb, int *event, bool *end);
int telev_poll(struct telev *tel, bool *marker, struct mbuf *mb);

int telev_digit2code(int digit);
int telev_code2digit(int code);
