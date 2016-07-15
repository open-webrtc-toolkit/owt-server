/**
 * @file bfcp/attr.c BFCP Attributes
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_tmr.h>
#include <re_bfcp.h>
#include "bfcp.h"


enum {
	BFCP_ATTR_HDR_SIZE = 2,
};


static void destructor(void *arg)
{
	struct bfcp_attr *attr = arg;

	switch (attr->type) {

	case BFCP_ERROR_CODE:
		mem_deref(attr->v.errcode.details);
		break;

	case BFCP_ERROR_INFO:
	case BFCP_PART_PROV_INFO:
	case BFCP_STATUS_INFO:
	case BFCP_USER_DISP_NAME:
	case BFCP_USER_URI:
		mem_deref(attr->v.str);
		break;

	case BFCP_SUPPORTED_ATTRS:
		mem_deref(attr->v.supattr.attrv);
		break;

	case BFCP_SUPPORTED_PRIMS:
		mem_deref(attr->v.supprim.primv);
		break;

	default:
		break;
	}

	list_flush(&attr->attrl);
	list_unlink(&attr->le);
}


static int attr_encode(struct mbuf *mb, bool mand, enum bfcp_attrib type,
		       const void *v)
{
	const struct bfcp_reqstatus *reqstatus = v;
	const struct bfcp_errcode *errcode = v;
	const struct bfcp_supattr *supattr = v;
	const struct bfcp_supprim *supprim = v;
	const enum bfcp_priority *priority = v;
	const uint16_t *u16 = v;
	size_t start, len, i;
	int err;

	start = mb->pos;
	mb->pos += BFCP_ATTR_HDR_SIZE;

	switch (type) {

	case BFCP_BENEFICIARY_ID:
	case BFCP_FLOOR_ID:
	case BFCP_FLOOR_REQUEST_ID:
	case BFCP_BENEFICIARY_INFO:
	case BFCP_FLOOR_REQ_INFO:
	case BFCP_REQUESTED_BY_INFO:
	case BFCP_FLOOR_REQ_STATUS:
	case BFCP_OVERALL_REQ_STATUS:
		err = mbuf_write_u16(mb, htons(*u16));
		break;

	case BFCP_PRIORITY:
		err  = mbuf_write_u8(mb, *priority << 5);
		err |= mbuf_write_u8(mb, 0x00);
		break;

	case BFCP_REQUEST_STATUS:
		err  = mbuf_write_u8(mb, reqstatus->status);
		err |= mbuf_write_u8(mb, reqstatus->qpos);
		break;

	case BFCP_ERROR_CODE:
		err = mbuf_write_u8(mb, errcode->code);
		if (errcode->details && errcode->len)
			err |= mbuf_write_mem(mb, errcode->details,
					      errcode->len);
		break;

	case BFCP_ERROR_INFO:
	case BFCP_PART_PROV_INFO:
	case BFCP_STATUS_INFO:
	case BFCP_USER_DISP_NAME:
	case BFCP_USER_URI:
		err = mbuf_write_str(mb, v);
		break;

	case BFCP_SUPPORTED_ATTRS:
		for (i=0, err=0; i<supattr->attrc; i++)
			err |= mbuf_write_u8(mb, supattr->attrv[i] << 1);
		break;

	case BFCP_SUPPORTED_PRIMS:
		for (i=0, err=0; i<supprim->primc; i++)
			err |= mbuf_write_u8(mb, supprim->primv[i]);
		break;

	default:
		err = EINVAL;
		break;
	}

	/* header */
	len = mb->pos - start;

	mb->pos = start;
	err |= mbuf_write_u8(mb, (type<<1) | (mand ? 1 : 0));
	err |= mbuf_write_u8(mb, len);
	mb->pos += (len - BFCP_ATTR_HDR_SIZE);

	/* padding */
	while ((mb->pos - start) & 0x03)
		err |= mbuf_write_u8(mb, 0x00);

	return err;
}


/**
 * Encode BFCP Attributes with variable arguments
 *
 * @param mb    Mbuf to encode into
 * @param attrc Number of attributes
 * @param ap    Variable argument of attributes
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_attrs_vencode(struct mbuf *mb, unsigned attrc, va_list *ap)
{
	unsigned i;

	if (!mb)
		return EINVAL;

	for (i=0; i<attrc; i++) {

		int  type     = va_arg(*ap, int);
		unsigned subc = va_arg(*ap, unsigned);
		const void *v = va_arg(*ap, const void *);
		size_t start, len;
		int err;

		if (!v)
			continue;

		start = mb->pos;

		if (type == BFCP_ENCODE_HANDLER) {

			const struct bfcp_encode *enc = v;

			if (enc->ench) {
				err = enc->ench(mb, enc->arg);
				if (err)
					return err;
			}

			continue;
		}

		err = attr_encode(mb, type>>7, type & 0x7f, v);
		if (err)
			return err;

		if (subc == 0)
			continue;

		err = bfcp_attrs_vencode(mb, subc, ap);
		if (err)
			return err;

		/* update total length for grouped attributes */
		len = mb->pos - start;

		mb->pos = start + 1;
		err = mbuf_write_u8(mb, (uint8_t)len);
		if (err)
			return err;

		mb->pos += (len - BFCP_ATTR_HDR_SIZE);
	}

	return 0;
}


/**
 * Encode BFCP Attributes
 *
 * @param mb      Mbuf to encode into
 * @param attrc   Number of attributes
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_attrs_encode(struct mbuf *mb, unsigned attrc, ...)
{
	va_list ap;
	int err;

	va_start(ap, attrc);
	err = bfcp_attrs_vencode(mb, attrc, &ap);
	va_end(ap);

	return err;
}


static int attr_decode(struct bfcp_attr **attrp, struct mbuf *mb,
		       struct bfcp_unknown_attr *uma)
{
	struct bfcp_attr *attr;
	union bfcp_union *v;
	size_t i, start, len;
	int err = 0;
	uint8_t b;

	if (mbuf_get_left(mb) < BFCP_ATTR_HDR_SIZE)
		return EBADMSG;

	attr = mem_zalloc(sizeof(*attr), destructor);
	if (!attr)
		return ENOMEM;

	start = mb->pos;

	b = mbuf_read_u8(mb);
	attr->type = b >> 1;
	attr->mand = b & 1;
	len = mbuf_read_u8(mb);

	if (len < BFCP_ATTR_HDR_SIZE)
		goto badmsg;

	len -= BFCP_ATTR_HDR_SIZE;

	if (mbuf_get_left(mb) < len)
		goto badmsg;

	v = &attr->v;

	switch (attr->type) {

	case BFCP_BENEFICIARY_ID:
	case BFCP_FLOOR_ID:
	case BFCP_FLOOR_REQUEST_ID:
		if (len < 2)
			goto badmsg;

		v->u16 = ntohs(mbuf_read_u16(mb));
		break;

	case BFCP_PRIORITY:
		if (len < 2)
			goto badmsg;

		v->priority = mbuf_read_u8(mb) >> 5;
		(void)mbuf_read_u8(mb);
		break;

	case BFCP_REQUEST_STATUS:
		if (len < 2)
			goto badmsg;

		v->reqstatus.status = mbuf_read_u8(mb);
		v->reqstatus.qpos   = mbuf_read_u8(mb);
		break;

	case BFCP_ERROR_CODE:
		if (len < 1)
			goto badmsg;

		v->errcode.code = mbuf_read_u8(mb);
		v->errcode.len  = len - 1;

		if (v->errcode.len == 0)
			break;

		v->errcode.details = mem_alloc(v->errcode.len, NULL);
		if (!v->errcode.details) {
			err = ENOMEM;
			goto error;
		}

		(void)mbuf_read_mem(mb, v->errcode.details,
				    v->errcode.len);
		break;

	case BFCP_ERROR_INFO:
	case BFCP_PART_PROV_INFO:
	case BFCP_STATUS_INFO:
	case BFCP_USER_DISP_NAME:
	case BFCP_USER_URI:
		err = mbuf_strdup(mb, &v->str, len);
		break;

	case BFCP_SUPPORTED_ATTRS:
		v->supattr.attrc = len;
		v->supattr.attrv = mem_alloc(len*sizeof(*v->supattr.attrv),
					     NULL);
		if (!v->supattr.attrv) {
			err = ENOMEM;
			goto error;
		}

		for (i=0; i<len; i++)
			v->supattr.attrv[i] = mbuf_read_u8(mb) >> 1;
		break;

	case BFCP_SUPPORTED_PRIMS:
		v->supprim.primc = len;
		v->supprim.primv = mem_alloc(len * sizeof(*v->supprim.primv),
					     NULL);
		if (!v->supprim.primv) {
			err = ENOMEM;
			goto error;
		}

		for (i=0; i<len; i++)
			v->supprim.primv[i] = mbuf_read_u8(mb);
		break;

		/* grouped attributes */

	case BFCP_BENEFICIARY_INFO:
	case BFCP_FLOOR_REQ_INFO:
	case BFCP_REQUESTED_BY_INFO:
	case BFCP_FLOOR_REQ_STATUS:
	case BFCP_OVERALL_REQ_STATUS:
		if (len < 2)
			goto badmsg;

		v->u16 = ntohs(mbuf_read_u16(mb));
		err = bfcp_attrs_decode(&attr->attrl, mb, len - 2, uma);
		break;

	default:
		mb->pos += len;

		if (!attr->mand)
			break;

		if (uma && uma->typec < ARRAY_SIZE(uma->typev))
			uma->typev[uma->typec++] = attr->type<<1;
		break;
	}

	if (err)
		goto error;

	/* padding */
	while (((mb->pos - start) & 0x03) && mbuf_get_left(mb))
		++mb->pos;

	*attrp = attr;

	return 0;

 badmsg:
	err = EBADMSG;
 error:
	mem_deref(attr);

	return err;
}


int bfcp_attrs_decode(struct list *attrl, struct mbuf *mb, size_t len,
		      struct bfcp_unknown_attr *uma)
{
	int err = 0;
	size_t end;

	if (!attrl || !mb || mbuf_get_left(mb) < len)
		return EINVAL;

	end     = mb->end;
	mb->end = mb->pos + len;

	while (mbuf_get_left(mb) >= BFCP_ATTR_HDR_SIZE) {

		struct bfcp_attr *attr;

		err = attr_decode(&attr, mb, uma);
		if (err)
			break;

		list_append(attrl, &attr->le, attr);
	}

	mb->end = end;

	return err;
}


struct bfcp_attr *bfcp_attrs_find(const struct list *attrl,
				  enum bfcp_attrib type)
{
	struct le *le = list_head(attrl);

	while (le) {
		struct bfcp_attr *attr = le->data;

		le = le->next;

		if (attr->type == type)
			return attr;
	}

	return NULL;
}


struct bfcp_attr *bfcp_attrs_apply(const struct list *attrl,
				   bfcp_attr_h *h, void *arg)
{
	struct le *le = list_head(attrl);

	while (le) {
		struct bfcp_attr *attr = le->data;

		le = le->next;

		if (h && h(attr, arg))
			return attr;
	}

	return NULL;
}


/**
 * Get a BFCP sub-attribute from a BFCP attribute
 *
 * @param attr BFCP attribute
 * @param type Attribute type
 *
 * @return Matching BFCP attribute if found, otherwise NULL
 */
struct bfcp_attr *bfcp_attr_subattr(const struct bfcp_attr *attr,
				    enum bfcp_attrib type)
{
	if (!attr)
		return NULL;

	return bfcp_attrs_find(&attr->attrl, type);
}


/**
 * Apply a function handler to all sub-attributes in a BFCP attribute
 *
 * @param attr BFCP attribute
 * @param h    Handler
 * @param arg  Handler argument
 *
 * @return BFCP attribute returned by handler, or NULL
 */
struct bfcp_attr *bfcp_attr_subattr_apply(const struct bfcp_attr *attr,
					  bfcp_attr_h *h, void *arg)
{
	if (!attr)
		return NULL;

	return bfcp_attrs_apply(&attr->attrl, h, arg);
}


/**
 * Print a BFCP attribute
 *
 * @param pf   Print function
 * @param attr BFCP attribute
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_attr_print(struct re_printf *pf, const struct bfcp_attr *attr)
{
	const union bfcp_union *v;
	size_t i;
	int err;

	if (!attr)
		return 0;

	err = re_hprintf(pf, "%c%-28s", attr->mand ? '*' : ' ',
			 bfcp_attr_name(attr->type));

	v = &attr->v;

	switch (attr->type) {

	case BFCP_BENEFICIARY_ID:
	case BFCP_FLOOR_ID:
	case BFCP_FLOOR_REQUEST_ID:
		err |= re_hprintf(pf, "%u", v->u16);
		break;

	case BFCP_PRIORITY:
		err |= re_hprintf(pf, "%d", v->priority);
		break;

	case BFCP_REQUEST_STATUS:
		err |= re_hprintf(pf, "%s (%d), qpos=%u",
				  bfcp_reqstatus_name(v->reqstatus.status),
				  v->reqstatus.status,
				  v->reqstatus.qpos);
		break;

	case BFCP_ERROR_CODE:
		err |= re_hprintf(pf, "%d (%s)", v->errcode.code,
				  bfcp_errcode_name(v->errcode.code));

		if (v->errcode.code == BFCP_UNKNOWN_MAND_ATTR) {

			for (i=0; i<v->errcode.len; i++) {

				uint8_t type = v->errcode.details[i] >> 1;

				err |= re_hprintf(pf, " %s",
						  bfcp_attr_name(type));
			}
		}
		break;

	case BFCP_ERROR_INFO:
	case BFCP_PART_PROV_INFO:
	case BFCP_STATUS_INFO:
	case BFCP_USER_DISP_NAME:
	case BFCP_USER_URI:
		err |= re_hprintf(pf, "\"%s\"", v->str);
		break;

	case BFCP_SUPPORTED_ATTRS:
		err |= re_hprintf(pf, "%zu:", v->supattr.attrc);

		for (i=0; i<v->supattr.attrc; i++) {

			const enum bfcp_attrib type = v->supattr.attrv[i];

			err |= re_hprintf(pf, " %s", bfcp_attr_name(type));
		}
		break;

	case BFCP_SUPPORTED_PRIMS:
		err |= re_hprintf(pf, "%zu:", v->supprim.primc);

		for (i=0; i<v->supprim.primc; i++) {

			const enum bfcp_prim prim = v->supprim.primv[i];

			err |= re_hprintf(pf, " %s", bfcp_prim_name(prim));
		}
		break;

		/* Grouped Attributes */

	case BFCP_BENEFICIARY_INFO:
		err |= re_hprintf(pf, "beneficiary-id=%u", v->beneficiaryid);
		break;

	case BFCP_FLOOR_REQ_INFO:
		err |= re_hprintf(pf, "floor-request-id=%u", v->floorreqid);
		break;

	case BFCP_REQUESTED_BY_INFO:
		err |= re_hprintf(pf, "requested-by-id=%u", v->reqbyid);
		break;

	case BFCP_FLOOR_REQ_STATUS:
		err |= re_hprintf(pf, "floor-id=%u", v->floorid);
		break;

	case BFCP_OVERALL_REQ_STATUS:
		err |= re_hprintf(pf, "floor-request-id=%u", v->floorreqid);
		break;

	default:
		err |= re_hprintf(pf, "???");
		break;
	}

	return err;
}


int bfcp_attrs_print(struct re_printf *pf, const struct list *attrl,
		     unsigned level)
{
	struct le *le;
	int err = 0;

	for (le=list_head(attrl); le; le=le->next) {

		const struct bfcp_attr *attr = le->data;
		unsigned i;

		for (i=0; i<level; i++)
			err |= re_hprintf(pf, "    ");

		err |= re_hprintf(pf, "%H\n", bfcp_attr_print, attr);
		err |= bfcp_attrs_print(pf, &attr->attrl, level + 1);
	}

	return err;
}


/**
 * Get the BFCP attribute name
 *
 * @param type BFCP attribute type
 *
 * @return String with BFCP attribute name
 */
const char *bfcp_attr_name(enum bfcp_attrib type)
{
	switch (type) {

	case BFCP_BENEFICIARY_ID:     return "BENEFICIARY-ID";
	case BFCP_FLOOR_ID:           return "FLOOR-ID";
	case BFCP_FLOOR_REQUEST_ID:   return "FLOOR-REQUEST-ID";
	case BFCP_PRIORITY:           return "PRIORITY";
	case BFCP_REQUEST_STATUS:     return "REQUEST-STATUS";
	case BFCP_ERROR_CODE:         return "ERROR-CODE";
	case BFCP_ERROR_INFO:         return "ERROR-INFO";
	case BFCP_PART_PROV_INFO:     return "PARTICIPANT-PROVIDED-INFO";
	case BFCP_STATUS_INFO:        return "STATUS-INFO";
	case BFCP_SUPPORTED_ATTRS:    return "SUPPORTED-ATTRIBUTES";
	case BFCP_SUPPORTED_PRIMS:    return "SUPPORTED-PRIMITIVES";
	case BFCP_USER_DISP_NAME:     return "USER-DISPLAY-NAME";
	case BFCP_USER_URI:           return "USER-URI";
	case BFCP_BENEFICIARY_INFO:   return "BENEFICIARY-INFORMATION";
	case BFCP_FLOOR_REQ_INFO:     return "FLOOR-REQUEST-INFORMATION";
	case BFCP_REQUESTED_BY_INFO:  return "REQUESTED-BY-INFORMATION";
	case BFCP_FLOOR_REQ_STATUS:   return "FLOOR-REQUEST-STATUS";
	case BFCP_OVERALL_REQ_STATUS: return "OVERALL-REQUEST-STATUS";
	default:                      return "???";
	}
}


/**
 * Get the BFCP Request status name
 *
 * @param status Request status
 *
 * @return String with BFCP Request status name
 */
const char *bfcp_reqstatus_name(enum bfcp_reqstat status)
{
	switch (status) {

	case BFCP_PENDING:   return "Pending";
	case BFCP_ACCEPTED:  return "Accepted";
	case BFCP_GRANTED:   return "Granted";
	case BFCP_DENIED:    return "Denied";
	case BFCP_CANCELLED: return "Cancelled";
	case BFCP_RELEASED:  return "Released";
	case BFCP_REVOKED:   return "Revoked";
	default:             return "???";
	}
}


/**
 * Get the BFCP Error code name
 *
 * @param code BFCP Error code
 *
 * @return String with error code
 */
const char *bfcp_errcode_name(enum bfcp_err code)
{
	switch (code) {

	case BFCP_CONF_NOT_EXIST:
		return "Conference does not Exist";

	case BFCP_USER_NOT_EXIST:
		return "User does not Exist";

	case BFCP_UNKNOWN_PRIM:
		return "Unknown Primitive";

	case BFCP_UNKNOWN_MAND_ATTR:
		return "Unknown Mandatory Attribute";

	case BFCP_UNAUTH_OPERATION:
		return "Unauthorized Operation";

	case BFCP_INVALID_FLOOR_ID:
		return "Invalid Floor ID";

	case BFCP_FLOOR_REQ_ID_NOT_EXIST:
		return "Floor Request ID Does Not Exist";

	case BFCP_MAX_FLOOR_REQ_REACHED:
		return "You have Already Reached the Maximum Number "
		       "of Ongoing Floor Requests for this Floor";

	case BFCP_USE_TLS:
		return "Use TLS";

	case BFCP_PARSE_ERROR:
		return "Unable to Parse Message";

	case BFCP_USE_DTLS:
		return "Use DTLS";

	case BFCP_UNSUPPORTED_VERSION:
		return "Unsupported Version";

	case BFCP_BAD_LENGTH:
		return "Incorrect Message Length";

	case BFCP_GENERIC_ERROR:
		return "Generic Error";

	default:
		return "???";
	}
}
