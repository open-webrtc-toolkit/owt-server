/**
 * @file sdp/format.c  SDP format
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <stdlib.h>
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_sdp.h>
#include "sdp.h"


static void destructor(void *arg)
{
	struct sdp_format *fmt = arg;

	list_unlink(&fmt->le);

	if (fmt->ref)
		mem_deref(fmt->data);

	mem_deref(fmt->id);
	mem_deref(fmt->params);
	mem_deref(fmt->rparams);
	mem_deref(fmt->name);
}


/**
 * Add an SDP Format to an SDP Media line
 *
 * @param fmtp    Pointer to allocated SDP Format
 * @param m       SDP Media line
 * @param prepend True to prepend, False to append
 * @param id      Format identifier
 * @param name    Format name
 * @param srate   Sampling rate
 * @param ch      Number of channels
 * @param ench    Optional format encode handler
 * @param cmph    Optional format comparison handler
 * @param data    Opaque data for handler
 * @param ref     True to mem_ref() data
 * @param params  Formatted parameters
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_format_add(struct sdp_format **fmtp, struct sdp_media *m,
		   bool prepend, const char *id, const char *name,
		   uint32_t srate, uint8_t ch, sdp_fmtp_enc_h *ench,
		   sdp_fmtp_cmp_h *cmph, void *data, bool ref,
		   const char *params, ...)
{
	struct sdp_format *fmt;
	int err;

	if (!m)
		return EINVAL;

	if (!id && (m->dynpt > RTP_DYNPT_END))
		return ERANGE;

	fmt = mem_zalloc(sizeof(*fmt), destructor);
	if (!fmt)
		return ENOMEM;

	if (prepend)
		list_prepend(&m->lfmtl, &fmt->le, fmt);
	else
		list_append(&m->lfmtl, &fmt->le, fmt);

	if (id)
		err = str_dup(&fmt->id, id);
	else
		err = re_sdprintf(&fmt->id, "%i", m->dynpt++);
	if (err)
		goto out;

	if (name) {
		err = str_dup(&fmt->name, name);
		if (err)
			goto out;
	}

	if (params) {
		va_list ap;

		va_start(ap, params);
		err = re_vsdprintf(&fmt->params, params, ap);
		va_end(ap);

		if (err)
			goto out;
	}

	fmt->pt    = atoi(fmt->id);
	fmt->srate = srate;
	fmt->ch    = ch;
	fmt->ench  = ench;
	fmt->cmph  = cmph;
	fmt->data  = ref ? mem_ref(data) : data;
	fmt->ref   = ref;
	fmt->sup   = true;

 out:
	if (err)
		mem_deref(fmt);
	else if (fmtp)
		*fmtp = fmt;

	return err;
}


int sdp_format_radd(struct sdp_media *m, const struct pl *id)
{
	struct sdp_format *fmt;
	int err;

	if (!m || !id)
		return EINVAL;

	fmt = mem_zalloc(sizeof(*fmt), destructor);
	if (!fmt)
		return ENOMEM;

	list_append(&m->rfmtl, &fmt->le, fmt);

	err = pl_strdup(&fmt->id, id);
	if (err)
		goto out;

	fmt->pt = atoi(fmt->id);

 out:
	if (err)
		mem_deref(fmt);

	return err;
}


struct sdp_format *sdp_format_find(const struct list *lst, const struct pl *id)
{
	struct le *le;

	if (!lst || !id)
		return NULL;

	for (le=lst->head; le; le=le->next) {

		struct sdp_format *fmt = le->data;

		if (pl_strcmp(id, fmt->id))
			continue;

		return fmt;
	}

	return NULL;
}


/**
 * Set the parameters of an SDP format
 *
 * @param fmt    SDP Format
 * @param params Formatted parameters
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_format_set_params(struct sdp_format *fmt, const char *params, ...)
{
	int err = 0;

	if (!fmt)
		return EINVAL;

	fmt->params = mem_deref(fmt->params);

	if (params) {
		va_list ap;

		va_start(ap, params);
		err = re_vsdprintf(&fmt->params, params, ap);
		va_end(ap);
	}

	return err;
}


/**
 * Compare two SDP Formats
 *
 * @param fmt1 First SDP format
 * @param fmt2 Second SDP format
 *
 * @return True if matching, False if not
 */
bool sdp_format_cmp(const struct sdp_format *fmt1,
		    const struct sdp_format *fmt2)
{
	if (!fmt1 || !fmt2)
		return false;

	if (fmt1->pt < RTP_DYNPT_START && fmt2->pt < RTP_DYNPT_START) {

		if (!fmt1->id || !fmt2->id)
			return false;

		return strcmp(fmt1->id, fmt2->id) ? false : true;
	}

	if (str_casecmp(fmt1->name, fmt2->name))
		return false;

	if (fmt1->srate != fmt2->srate)
		return false;

	if (fmt1->ch != fmt2->ch)
		return false;

	if (fmt1->cmph && !fmt1->cmph(fmt1->params, fmt2->params, fmt1->data))
		return false;

	if (fmt2->cmph && !fmt2->cmph(fmt2->params, fmt1->params, fmt2->data))
		return false;

	return true;
}


/**
 * Print SDP Format debug information
 *
 * @param pf  Print function for output
 * @param fmt SDP Format
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_format_debug(struct re_printf *pf, const struct sdp_format *fmt)
{
	int err;

	if (!fmt)
		return 0;

	err = re_hprintf(pf, "%3s", fmt->id);

	if (fmt->name)
		err |= re_hprintf(pf, " %s/%u/%u",
				  fmt->name, fmt->srate, fmt->ch);

	if (fmt->params)
		err |= re_hprintf(pf, " (%s)", fmt->params);

	if (fmt->sup)
		err |= re_hprintf(pf, " *");

	return err;
}
