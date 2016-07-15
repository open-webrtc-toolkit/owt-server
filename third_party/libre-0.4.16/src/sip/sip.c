/**
 * @file sip.c  SIP Core
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_hash.h>
#include <re_fmt.h>
#include <re_uri.h>
#include <re_sys.h>
#include <re_tmr.h>
#include <re_udp.h>
#include <re_stun.h>
#include <re_msg.h>
#include <re_sip.h>
#include "sip.h"


static void destructor(void *arg)
{
	struct sip *sip = arg;

	if (sip->closing) {
		sip->closing = false;
		mem_ref(sip);
		if (sip->exith)
			sip->exith(sip->arg);
		return;
	}

	sip_request_close(sip);
	sip_request_close(sip);

	hash_flush(sip->ht_ctrans);
	mem_deref(sip->ht_ctrans);

	hash_flush(sip->ht_strans);
	hash_clear(sip->ht_strans_mrg);
	mem_deref(sip->ht_strans);
	mem_deref(sip->ht_strans_mrg);

	hash_flush(sip->ht_conn);
	mem_deref(sip->ht_conn);

	hash_flush(sip->ht_udpconn);
	mem_deref(sip->ht_udpconn);

	list_flush(&sip->transpl);
	list_flush(&sip->lsnrl);

	mem_deref(sip->software);
	mem_deref(sip->dnsc);
	mem_deref(sip->stun);
}


static void lsnr_destructor(void *arg)
{
	struct sip_lsnr *lsnr = arg;

	if (lsnr->lsnrp)
		*lsnr->lsnrp = NULL;

	list_unlink(&lsnr->le);
}


/**
 * Allocate a SIP stack instance
 *
 * @param sipp     Pointer to allocated SIP stack
 * @param dnsc     DNS Client (optional)
 * @param ctsz     Size of client transactions hashtable (power of 2)
 * @param stsz     Size of server transactions hashtable (power of 2)
 * @param tcsz     Size of SIP transport hashtable (power of 2)
 * @param software Software identifier
 * @param exith    SIP-stack exit handler
 * @param arg      Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_alloc(struct sip **sipp, struct dnsc *dnsc, uint32_t ctsz,
	      uint32_t stsz, uint32_t tcsz, const char *software,
	      sip_exit_h *exith, void *arg)
{
	struct sip *sip;
	int err;

	if (!sipp)
		return EINVAL;

	sip = mem_zalloc(sizeof(*sip), destructor);
	if (!sip)
		return ENOMEM;

	err = sip_transp_init(sip, tcsz);
	if (err)
		goto out;

	err = sip_ctrans_init(sip, ctsz);
	if (err)
		goto out;

	err = sip_strans_init(sip, stsz);
	if (err)
		goto out;

	err = hash_alloc(&sip->ht_udpconn, tcsz);
	if (err)
		goto out;

	err = stun_alloc(&sip->stun, NULL, NULL, NULL);
	if (err)
		goto out;

	if (software) {
		err = str_dup(&sip->software, software);
		if (err)
			goto out;
	}

	sip->dnsc  = mem_ref(dnsc);
	sip->exith = exith;
	sip->arg   = arg;

 out:
	if (err)
		mem_deref(sip);
	else
		*sipp = sip;

	return err;
}


/**
 * Close the SIP stack instance
 *
 * @param sip   SIP stack instance
 * @param force Don't wait for transactions to complete
 */
void sip_close(struct sip *sip, bool force)
{
	if (!sip)
		return;

	if (force) {
		sip_request_close(sip);
		sip_request_close(sip);
	}
	else if (!sip->closing) {
		sip->closing = true;
		mem_deref(sip);
	}
}


/**
 * Send a SIP message
 *
 * @param sip  SIP stack instance
 * @param sock Optional socket to send from
 * @param tp   SIP transport
 * @param dst  Destination network address
 * @param mb   Buffer containing SIP message
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_send(struct sip *sip, void *sock, enum sip_transp tp,
	     const struct sa *dst, struct mbuf *mb)
{
	return sip_transp_send(NULL, sip, sock, tp, dst, mb, NULL, NULL);
}


/**
 * Listen for incoming SIP Requests and SIP Responses
 *
 * @param lsnrp Pointer to allocated listener
 * @param sip   SIP stack instance
 * @param req   True for Request, false for Response
 * @param msgh  SIP message handler
 * @param arg   Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_listen(struct sip_lsnr **lsnrp, struct sip *sip, bool req,
	       sip_msg_h *msgh, void *arg)
{
	struct sip_lsnr *lsnr;

	if (!sip || !msgh)
		return EINVAL;

	lsnr = mem_zalloc(sizeof(*lsnr), lsnr_destructor);
	if (!lsnr)
		return ENOMEM;

	list_append(&sip->lsnrl, &lsnr->le, lsnr);

	lsnr->msgh = msgh;
	lsnr->arg = arg;
	lsnr->req = req;

	if (lsnrp) {
		lsnr->lsnrp = lsnrp;
		*lsnrp = lsnr;
	}

	return 0;
}


/**
 * Print debug information about the SIP stack
 *
 * @param pf  Print function for debug output
 * @param sip SIP stack instance
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_debug(struct re_printf *pf, const struct sip *sip)
{
	int err;

	if (!sip)
		return 0;

	err  = sip_transp_debug(pf, sip);
	err |= sip_ctrans_debug(pf, sip);
	err |= sip_strans_debug(pf, sip);

	return err;
}
