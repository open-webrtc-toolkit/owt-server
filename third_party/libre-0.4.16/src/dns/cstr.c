/**
 * @file cstr.c  DNS character strings encoding
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_list.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_dns.h>


/**
 * Encode a DNS character string into a memory buffer
 *
 * @param mb  Memory buffer to encode into
 * @param str Character string
 *
 * @return 0 if success, otherwise errorcode
 */
int dns_cstr_encode(struct mbuf *mb, const char *str)
{
	uint8_t len;
	int err = 0;

	if (!mb || !str)
		return EINVAL;

	len = (uint8_t)strlen(str);

	err |= mbuf_write_u8(mb, len);
	err |= mbuf_write_mem(mb, (const uint8_t *)str, len);

	return err;
}


/**
 * Decode a DNS character string from a memory buffer
 *
 * @param mb  Memory buffer to decode from
 * @param str Pointer to allocated character string
 *
 * @return 0 if success, otherwise errorcode
 */
int dns_cstr_decode(struct mbuf *mb, char **str)
{
	uint8_t len;

	if (!mb || !str || (mbuf_get_left(mb) < 1))
		return EINVAL;

	len = mbuf_read_u8(mb);

	if (mbuf_get_left(mb) < len)
		return EBADMSG;

	return mbuf_strdup(mb, str, len);
}
