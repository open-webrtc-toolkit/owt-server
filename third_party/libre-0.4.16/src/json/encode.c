/**
 * @file json/encode.c  JSON encoder
 *
 * Copyright (C) 2010 - 2015 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_list.h>
#include <re_odict.h>
#include <re_json.h>


static int encode_entry(struct re_printf *pf, const struct odict_entry *e)
{
	struct odict *array;
	struct le *le;
	int err;

	if (!e)
		return 0;

	switch (e->type) {

	case ODICT_OBJECT:
		err = json_encode_odict(pf, e->u.odict);
		break;

	case ODICT_ARRAY:
		array = e->u.odict;
		if (!array)
			return 0;

		err = re_hprintf(pf, "[");

		for (le=array->lst.head; le; le=le->next) {

			const struct odict_entry *ae = le->data;

			err |= re_hprintf(pf, "%H%s",
					  encode_entry, ae,
					  le->next ? "," : "");
		}

		err |= re_hprintf(pf, "]");
		break;

	case ODICT_INT:
		err = re_hprintf(pf, "%lld", e->u.integer);
		break;

	case ODICT_DOUBLE:
		err = re_hprintf(pf, "%f", e->u.dbl);
		break;

	case ODICT_STRING:
		err = re_hprintf(pf, "\"%H\"", utf8_encode, e->u.str);
		break;

	case ODICT_BOOL:
		err = re_hprintf(pf, "%s", e->u.boolean ? "true" : "false");
		break;

	case ODICT_NULL:
		err = re_hprintf(pf, "null");
		break;

	default:
		re_fprintf(stderr, "json: unsupported type %d\n", e->type);
		err = EINVAL;
	}

	return err;
}


int json_encode_odict(struct re_printf *pf, const struct odict *o)
{
	struct le *le;
	int err;

	if (!o)
		return 0;

	err = re_hprintf(pf, "{");

	for (le=o->lst.head; le; le=le->next) {

		const struct odict_entry *e = le->data;

		err |= re_hprintf(pf, "\"%H\":%H%s",
				  utf8_encode, e->key,
				  encode_entry, e,
				  le->next ? "," : "");
	}

	err |= re_hprintf(pf, "}");

	return err;
}
