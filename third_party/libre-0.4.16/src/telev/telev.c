/**
 * @file telev.c  Telephony Events implementation (RFC 4733)
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <ctype.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_tmr.h>
#include <re_net.h>
#include <re_telev.h>


/*
 * Implements a subset of RFC 4733,
 * "RTP Payload for DTMF Digits, Telephony Tones, and Telephony Signals"
 *
 * Call telev_send() to send telephony events, and telev_recv() to receive.
 * The function telev_poll() must be called periodically, with a minimum
 * and stable interval of less than or equal to packet time (e.g. 50ms)
 *
 * NOTE: RTP timestamp and sequence number is kept in application
 *
 *
 *  Example sending:
 *
 *               ms  M  ts  seq  ev   dur  End
 *
 *  press X -->
 *               50  1   0   1    X   400   0
 *              100  0   0   2    X   800   0
 *              150  0   0   3    X  1200   0
 *              200  0   0   4    X  1600   0
 *              ...  .   .   .    .  ....   .
 *  release X -->
 *              250  0   0   5    X  1600   1
 *              300  0   0   6    X  1600   1
 *  press Y -->
 *              350  1  2000 7    Y   400   0
 *              ...  .  .... .    .   ...   .
 */


enum {
	TXC_DIGIT_MIN = 9,
	TXC_END       = 3,
	EVENT_END     = 0xff,
	PAYLOAD_SIZE  = 4,
	VOLUME        = 10,
	QUEUE_SIZE    = 8192,
};


enum state {
	IDLE,
	SENDING,
	ENDING,
};


struct telev_fmt {
	uint8_t event;
	bool end;
	uint8_t vol;
	uint16_t dur;
};


/** Defines a Telephony Events state */
struct telev {
	/* tx */
	struct mbuf *mb;
	uint32_t ptime;
	enum state state;
	int event;
	uint16_t dur;
	uint32_t txc;

	/* rx */
	int rx_event;
	bool rx_end;
};


const char telev_rtpfmt[] = "telephone-event";


static int payload_encode(struct mbuf *mb, int event, bool end, uint16_t dur)
{
	size_t pos;
	int err;

	if (!mb)
		return EINVAL;

	pos = mb->pos;

	err  = mbuf_write_u8(mb, event);
	err |= mbuf_write_u8(mb, (end ? 0x80 : 0x00) | (VOLUME & 0x3f));
	err |= mbuf_write_u16(mb, htons(dur));

	if (err)
		mb->pos = pos;

	return err;
}


static int payload_decode(struct mbuf *mb, struct telev_fmt *fmt)
{
	uint8_t b;

	if (!mb || !fmt)
		return EINVAL;

	if (mbuf_get_left(mb) < PAYLOAD_SIZE)
		return ENOENT;

	fmt->event = mbuf_read_u8(mb);
	b = mbuf_read_u8(mb);
	fmt->end = (b & 0x80) == 0x80;
	fmt->vol = (b & 0x3f);
	fmt->dur = ntohs(mbuf_read_u16(mb));

	return 0;
}


static void destructor(void *arg)
{
	struct telev *t = arg;

	mem_deref(t->mb);
}


/**
 * Allocate a new Telephony Events state
 *
 * @param tp    Pointer to allocated object
 * @param ptime Packet time in [ms]
 *
 * @return 0 if success, otherwise errorcode
 */
int telev_alloc(struct telev **tp, uint32_t ptime)
{
	struct telev *t;
	int err = 0;

	if (!tp || !ptime)
		return EINVAL;

	t = mem_zalloc(sizeof(*t), destructor);
	if (!t)
		return ENOMEM;

	t->mb = mbuf_alloc(16);
	if (!t->mb) {
		err = ENOMEM;
		goto out;
	}

	t->state = IDLE;
	t->ptime = ptime;
	t->rx_event = -1;

 out:
	if (err)
		mem_deref(t);
	else
		*tp = t;

	return err;
}


/**
 * Send a Telephony Event
 *
 * @param tel   Telephony Event state
 * @param event The Event to send
 * @param end   End-of-event flag
 *
 * @return 0 if success, otherwise errorcode
 */
int telev_send(struct telev *tel, int event, bool end)
{
	size_t pos;
	int err;

	if (!tel)
		return EINVAL;

	if (tel->mb->end >= QUEUE_SIZE)
		return EOVERFLOW;

	pos = tel->mb->pos;

	tel->mb->pos = tel->mb->end;
	err = mbuf_write_u8(tel->mb, end ? EVENT_END : event);
	tel->mb->pos = pos;

	return err;
}


/**
 * Receive a Telephony Event
 *
 * @param tel   Telephony Event state
 * @param mb    Buffer to decode
 * @param event The received event, set on return
 * @param end   End-of-event flag, set on return
 *
 * @return 0 if success, otherwise errorcode
 */
int telev_recv(struct telev *tel, struct mbuf *mb, int *event, bool *end)
{
	struct telev_fmt fmt;
	int err;

	if (!tel || !mb || !event || !end)
		return EINVAL;

	err = payload_decode(mb, &fmt);
	if (err)
		return err;

	if (fmt.end) {
		if (tel->rx_end)
			return EALREADY;

		*event = fmt.event;
		*end = true;
		tel->rx_event = -1;
		tel->rx_end = true;
		return 0;
	}

	if (fmt.event == tel->rx_event)
		return EALREADY;

	*event = tel->rx_event = fmt.event;
	*end   = tel->rx_end   = false;

	return 0;
}


/**
 * Poll a Telephony Event state for sending
 *
 * @param tel    Telephony Event state
 * @param marker Marker bit, set on return
 * @param mb     Buffer with encoded data to send
 *
 * @return 0 if success, otherwise errorcode
 */
int telev_poll(struct telev *tel, bool *marker, struct mbuf *mb)
{
	bool mrk = false;
	int err = ENOENT;

	if (!tel || !marker || !mb)
		return EINVAL;

	switch (tel->state) {

	case IDLE:
		if (!mbuf_get_left(tel->mb))
			break;

		mrk = true;

		tel->event = mbuf_read_u8(tel->mb);
		tel->dur   = tel->ptime * 8;
		tel->state = SENDING;
		tel->txc   = 1;

		err = payload_encode(mb, tel->event, false, tel->dur);
		break;

	case SENDING:
		tel->dur += tel->ptime * 8;

		err = payload_encode(mb, tel->event, false, tel->dur);

		if (++tel->txc < TXC_DIGIT_MIN)
			break;

		if (!mbuf_get_left(tel->mb))
			break;

		if (mbuf_read_u8(tel->mb) != EVENT_END)
			tel->mb->pos--;

		tel->state = ENDING;
		tel->txc   = 0;
		break;

	case ENDING:
		err = payload_encode(mb, tel->event, true, tel->dur);

		if (++tel->txc < TXC_END)
			break;

		if (!mbuf_get_left(tel->mb))
			tel->mb->pos = tel->mb->end = 0;

		tel->state = IDLE;
		break;
	}

	if (!err)
		*marker = mrk;

	return err;
}


/**
 * Convert DTMF digit to Event code
 *
 * @param digit DTMF Digit
 *
 * @return Event code, or -1 if error
 */
int telev_digit2code(int digit)
{
	if (isdigit(digit))
		return digit - '0';
	else if (digit == '*')
		return 10;
	else if (digit == '#')
		return 11;
	else if ('a' <= digit && digit <= 'd')
		return digit - 'a' + 12;
	else if ('A' <= digit && digit <= 'D')
		return digit - 'A' + 12;
	else
		return -1;
}


/**
 * Convert Event code to DTMF digit
 *
 * @param code Event code
 *
 * @return DTMF Digit, or -1 if error
 */
int telev_code2digit(int code)
{
	static const char map[] = "0123456789*#ABCD";

	if (code < 0 || code >= 16)
		return -1;

	return map[code];
}
