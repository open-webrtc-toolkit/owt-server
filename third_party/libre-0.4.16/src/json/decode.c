/**
 * @file json/decode.c  JSON decoder
 *
 * Copyright (C) 2010 - 2015 Creytiv.com
 */

#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_list.h>
#include <re_hash.h>
#include <re_odict.h>
#include <re_json.h>


static inline uint64_t mypower10(uint64_t e)
{
	uint64_t i, n = 1;

	for (i=0; i<e; i++)
		n *= 10;

	return n;
}


static bool is_string(struct pl *c, const struct pl *pl)
{
	if (pl->l < 2)
		return false;

	if (pl->p[0] != '"'|| pl->p[pl->l-1] != '"')
		return false;

	c->p = pl->p + 1;
	c->l = pl->l - 2;

	return true;
}


static bool is_number(long double *d, bool *isfloat, const struct pl *pl)
{
	bool neg = false, pos = false, frac = false, exp = false;
	long double v = 0, mul = 1;
	const char *p;
	int64_t e = 0;

	if (!pl->l)
		return false;

	p = &pl->p[pl->l];

	while (p > pl->p) {

		const char ch = *--p;

		if (ch == 'e' || ch == 'E') {

			if (exp || frac)
				return false;

			exp = true;
			e   = neg ? -v : v;
			v   = 0;
			mul = 1;
			neg = false;
			pos = false;
		}
		else if (pos || neg) {
			return false;
		}
		else if (ch == '.') {

			if (frac)
				return false;

			frac = true;
			v /= mul;
			mul = 1;
		}
		else if ('0' <= ch && ch <= '9') {
			v += mul * (ch - '0');
			mul *= 10;
		}
		else if (ch == '-') {
			neg = true;
		}
		else if (ch == '+') {
			pos = true;
		}
		else {
			return false;
		}
	}

	*isfloat = (frac || (exp && e < 0));

	if (exp) {
		if (e < 0)
			v /= mypower10(-e);
		else
			v *= mypower10(e);
	}

	if (neg)
		v = -v;

	*d = v;

	return true;
}


static int decode_name(char **str, const struct pl *pl)
{
	struct pl pls;

	if (!pl->p)
		return EBADMSG;

	if (!is_string(&pls, pl))
		return EBADMSG;

	return re_sdprintf(str, "%H", utf8_decode, &pls);
}


static int decode_value(struct json_value *val, const struct pl *pl)
{
	long double dbl;
	struct pl pls;
	bool isfloat;
	int err = 0;

	if (!pl->p)
		return EBADMSG;

	if (is_string(&pls, pl)) {

		err = re_sdprintf(&val->v.str, "%H", utf8_decode, &pls);
		val->type = JSON_STRING;
	}
	else if (is_number(&dbl, &isfloat, pl)) {

		if (isfloat) {
			val->type  = JSON_DOUBLE;
			val->v.dbl = dbl;
		}
		else {
			val->type      = JSON_INT;
			val->v.integer = dbl;
		}
	}
	else if (!pl_strcasecmp(pl, "false")) {

		val->v.boolean = false;
		val->type  = JSON_BOOL;
	}
	else if (!pl_strcasecmp(pl, "true")) {

		val->v.boolean = true;
		val->type  = JSON_BOOL;
	}
	else if (!pl_strcasecmp(pl, "null")) {

		val->type = JSON_NULL;
	}
	else {
		re_printf("json: value of unknown type: <%r>\n", pl);
		err = EBADMSG;
	}

	return err;
}


static int object_entry(const struct pl *pl_name, const struct pl *pl_val,
			json_object_entry_h *oeh, void *arg)
{
	struct json_value val;
	char *name;
	int err;

	err = decode_name(&name, pl_name);
	if (err)
		return err;

	err = decode_value(&val, pl_val);
	if (err)
		goto out;

	if (oeh)
		err = oeh(name, &val, arg);

	if (val.type == JSON_STRING)
		mem_deref(val.v.str);

 out:
	mem_deref(name);

	return err;
}


static int array_entry(unsigned idx, const struct pl *pl_val,
		       json_array_entry_h *aeh, void *arg)
{
	struct json_value val;
	int err;

	err = decode_value(&val, pl_val);
	if (err)
		return err;

	if (aeh)
		err = aeh(idx, &val, arg);

	if (val.type == JSON_STRING)
		mem_deref(val.v.str);

	return err;
}


static int object_start(const struct pl *pl_name, unsigned idx,
			struct json_handlers *h)
{
	char *name = NULL;
	int err;

	if (pl_name->p) {

		err = decode_name(&name, pl_name);
		if (err)
			return err;
	}

	if (h->oh)
		err = h->oh(name, idx, h);

	mem_deref(name);

	return err;
}


static int array_start(const struct pl *pl_name, unsigned idx,
		       struct json_handlers *h)
{
	char *name = NULL;
	int err;

	if (pl_name->p) {

		err = decode_name(&name, pl_name);
		if (err)
			return err;
	}

	if (h->ah)
		err = h->ah(name, idx, h);

	mem_deref(name);

	return err;
}


static inline int chkval(struct pl *val, const char *p)
{
	if (!val->p || p<val->p)
		return EINVAL;

	val->l = p - val->p;

	return 0;
}


static int _json_decode(const char **str, size_t *len,
			unsigned depth, unsigned maxdepth,
			json_object_h *oh, json_array_h *ah,
			json_object_entry_h *oeh, json_array_entry_h *aeh,
			void *arg)
{
	bool esc = false, inquot = false, inobj = false, inarray = false;
	struct pl name = PL_INIT, val = PL_INIT;
	size_t ws = 0;
	unsigned idx = 0;
	int err;

	for (; *len>0; ++(*str), --(*len)) {

		if (inquot) {
			if (esc)
				esc = false;
			else if (**str == '\"')
				inquot = false;
			else if (**str == '\\')
				esc = true;

			continue;
		}

		switch (**str) {

		case ':':
			if (!inobj || name.p || chkval(&val, *str - ws))
				return EBADMSG;

			name = val;
			val  = pl_null;
			break;

		case ',':
			if (chkval(&val, *str - ws))
				break;

			if (inobj) {

				if (!name.p)
					return EBADMSG;

				err = object_entry(&name, &val, oeh, arg);
				if (err)
					return err;
			}
			else if (inarray) {

				err = array_entry(idx, &val, aeh, arg);
				if (err)
					return err;

				++idx;
			}
			else
				return EBADMSG;

			name = pl_null;
			val  = pl_null;
			break;

		case '{':
			if (inobj || inarray) {

				struct json_handlers h = {oh,ah,oeh,aeh,arg};

				if (depth >= maxdepth)
					return EOVERFLOW;

				if (inobj && !name.p)
					return EBADMSG;

				err = object_start(&name, idx, &h);
				if (err)
					return err;

				name = pl_null;

				err = _json_decode(str, len, depth + 1,
						   maxdepth, h.oh, h.ah,
						   h.oeh, h.aeh, h.arg);
				if (err)
					return err;

				if (inarray)
					++idx;
			}
			else {
				inobj = true;
			}
			break;

		case '[':
			if (inobj || inarray) {

				struct json_handlers h = {oh,ah,oeh,aeh,arg};

				if (depth >= maxdepth)
					return EOVERFLOW;

				if (inobj && !name.p)
					return EBADMSG;

				err = array_start(&name, idx, &h);
				if (err)
					return err;

				name = pl_null;

				err = _json_decode(str, len, depth + 1,
						   maxdepth, h.oh, h.ah,
						   h.oeh, h.aeh, h.arg);
				if (err)
					return err;

				if (inarray)
					++idx;
			}
			else {
				inarray = true;
				idx = 0;
			}
			break;

		case '}':
			if (!inobj)
				return EBADMSG;

			if (chkval(&val, *str - ws))
				return 0;

			if (!name.p)
				return EBADMSG;

			return object_entry(&name, &val, oeh, arg);

		case ']':
			if (!inarray)
				return EBADMSG;

			if (chkval(&val, *str - ws))
				return 0;

			return array_entry(idx, &val, aeh, arg);

		case ' ':
		case '\t':
		case '\r':
		case '\n':
			++ws;
			break;

		default:
			if (val.p)
				break;

			if (**str == '\"')
				inquot = true;

			val.p = *str;
			val.l = 0;
			ws = 0;
			break;
		}
	}

	if (inobj || inarray)
		return EBADMSG;

	return 0;
}


int json_decode(const char *str, size_t len, unsigned maxdepth,
		json_object_h *oh, json_array_h *ah,
		json_object_entry_h *oeh, json_array_entry_h *aeh, void *arg)
{
	if (!str)
		return EINVAL;

	return _json_decode(&str, &len, 0, maxdepth, oh, ah, oeh, aeh, arg);
}
