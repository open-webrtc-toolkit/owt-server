/**
 * @file unicode.c  Unicode character coding
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <ctype.h>
#include <re_types.h>
#include <re_fmt.h>


static const char *hex_chars = "0123456789ABCDEF";


/**
 * UTF-8 encode
 *
 * @param pf  Print function for output
 * @param str Input string to encode
 *
 * @return 0 if success, otherwise errorcode
 */
int utf8_encode(struct re_printf *pf, const char *str)
{
	char ubuf[6] = "\\u00", ebuf[2] = "\\";

	if (!pf)
		return EINVAL;

	if (!str)
		return 0;

	while (*str) {
		const uint8_t c = *str++;  /* NOTE: must be unsigned 8-bit */
		bool unicode = false;
		char ec = 0;
		int err;

		switch (c) {

		case '"':  ec = '"'; break;
		case '\\': ec = '\\'; break;
		case '/':  ec = '/'; break;
		case '\b': ec = 'b'; break;
		case '\f': ec = 'f'; break;
		case '\n': ec = 'n'; break;
		case '\r': ec = 'r'; break;
		case '\t': ec = 't'; break;
		default:
			if (c < ' ') {
				unicode = true;
			}
			/* chars in range 0x80-0xff are not escaped */
			break;
		}

		if (unicode) {
			ubuf[4] = hex_chars[(c>>4) & 0xf];
			ubuf[5] = hex_chars[c & 0xf];

			err = pf->vph(ubuf, sizeof(ubuf), pf->arg);
		}
		else if (ec) {
			ebuf[1] = ec;

			err = pf->vph(ebuf, sizeof(ebuf), pf->arg);
		}
		else {
			err = pf->vph((char *)&c, 1, pf->arg);
		}

		if (err)
			return err;
	}

	return 0;
}


/**
 * UTF-8 decode
 *
 * @param pf Print function for output
 * @param pl Input buffer to decode
 *
 * @return 0 if success, otherwise errorcode
 */
int utf8_decode(struct re_printf *pf, const struct pl *pl)
{
	size_t i;

	if (!pf)
		return EINVAL;

	if (!pl)
		return 0;

	for (i=0; i<pl->l; i++) {

		char ch = pl->p[i];
		int err;

		if (ch == '\\') {

			uint16_t u = 0;

			++i;

			if (i >= pl->l)
				return EBADMSG;

			ch = pl->p[i];

			switch (ch) {

			case 'b':
				ch = '\b';
				break;

			case 'f':
				ch = '\f';
				break;

			case 'n':
				ch = '\n';
				break;

			case 'r':
				ch = '\r';
				break;

			case 't':
				ch = '\t';
				break;

			case 'u':
				if (i+4 >= pl->l)
					return EBADMSG;

				if (!isxdigit(pl->p[i+1]) ||
				    !isxdigit(pl->p[i+2]) ||
				    !isxdigit(pl->p[i+3]) ||
				    !isxdigit(pl->p[i+4]))
					return EBADMSG;

				u |= ((uint16_t)ch_hex(pl->p[++i])) << 12;
				u |= ((uint16_t)ch_hex(pl->p[++i])) << 8;
				u |= ((uint16_t)ch_hex(pl->p[++i])) << 4;
				u |= ((uint16_t)ch_hex(pl->p[++i])) << 0;

				if (u > 255) {
					ch  = u>>8;
					err = pf->vph(&ch, 1, pf->arg);
					if (err)
						return err;
				}

				ch = u & 0xff;
				break;
			}
		}

		err = pf->vph(&ch, 1, pf->arg);
		if (err)
			return err;
	}

	return 0;
}
