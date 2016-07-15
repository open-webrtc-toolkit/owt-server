/**
 * @file odict/entry.c  Ordered Dictionary -- entry
 *
 * Copyright (C) 2010 - 2015 Creytiv.com
 */

#include "re_types.h"
#include "re_fmt.h"
#include "re_mem.h"
#include "re_list.h"
#include "re_hash.h"
#include "re_odict.h"


static void destructor(void *arg)
{
	struct odict_entry *e = arg;

	switch (e->type) {

	case ODICT_OBJECT:
	case ODICT_ARRAY:
		mem_deref(e->u.odict);
		break;

	case ODICT_STRING:
		mem_deref(e->u.str);
		break;

	default:
		break;
	}

	hash_unlink(&e->he);
	list_unlink(&e->le);
	mem_deref(e->key);
}


int odict_entry_add(struct odict *o, const char *key,
		    enum odict_type type, ...)
{
	struct odict_entry *e;
	va_list ap;
	int err;

	if (!o || !key)
		return EINVAL;

	e = mem_zalloc(sizeof(*e), destructor);
	if (!e)
		return ENOMEM;

	e->type = type;

	err = str_dup(&e->key, key);
	if (err)
		goto out;

	va_start(ap, type);

	switch (e->type) {

	case ODICT_OBJECT:
	case ODICT_ARRAY:
		e->u.odict = mem_ref(va_arg(ap, struct odict *));
		break;

	case ODICT_STRING:
		err = str_dup(&e->u.str, va_arg(ap, const char *));
		break;

	case ODICT_INT:
		e->u.integer = va_arg(ap, int64_t);
		break;

	case ODICT_DOUBLE:
		e->u.dbl = va_arg(ap, double);
		break;

	case ODICT_BOOL:
		e->u.boolean = va_arg(ap, int);
		break;

	case ODICT_NULL:
		break;

	default:
		err = EINVAL;
		break;
	}

	va_end(ap);

	if (err)
		goto out;

	list_append(&o->lst, &e->le, e);
	hash_append(o->ht, hash_fast_str(e->key), &e->he, e);

 out:
	if (err)
		mem_deref(e);

	return err;
}


void odict_entry_del(struct odict *o, const char *key)
{
	mem_deref((struct odict_entry *)odict_lookup(o, key));
}


int odict_entry_debug(struct re_printf *pf, const struct odict_entry *e)
{
	int err;

	if (!e)
		return 0;

	err = re_hprintf(pf, "%s", e->key);

	switch (e->type) {

	case ODICT_OBJECT:
	case ODICT_ARRAY:
		err |= re_hprintf(pf, ":%H", odict_debug, e->u.odict);
		break;

	case ODICT_STRING:
		err |= re_hprintf(pf, ":%s", e->u.str);
		break;

	case ODICT_INT:
		err |= re_hprintf(pf, ":%lli", e->u.integer);
		break;

	case ODICT_DOUBLE:
		err |= re_hprintf(pf, ":%f", e->u.dbl);
		break;

	case ODICT_BOOL:
		err |= re_hprintf(pf, ":%s", e->u.boolean ? "true" : "false");
		break;

	case ODICT_NULL:
		break;
	}

	return err;
}
