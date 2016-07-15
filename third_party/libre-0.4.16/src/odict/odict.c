/**
 * @file odict.c  Ordered Dictionary
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include "re_types.h"
#include "re_fmt.h"
#include "re_mem.h"
#include "re_list.h"
#include "re_hash.h"
#include "re_odict.h"


static void destructor(void *arg)
{
	struct odict *o = arg;

	hash_clear(o->ht);
	list_flush(&o->lst);
	mem_deref(o->ht);
}


int odict_alloc(struct odict **op, uint32_t hash_size)
{
	struct odict *o;
	int err;

	if (!op || !hash_size)
		return EINVAL;

	o = mem_zalloc(sizeof(*o), destructor);
	if (!o)
		return ENOMEM;

	err = hash_alloc(&o->ht, hash_valid_size(hash_size));
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(o);
	else
		*op = o;

	return err;
}


const struct odict_entry *odict_lookup(const struct odict *o, const char *key)
{
	struct le *le;

	if (!o || !key)
		return NULL;

	le = list_head(hash_list(o->ht, hash_fast_str(key)));

	while (le) {
		const struct odict_entry *e = le->data;

		if (!str_cmp(e->key, key))
			return e;

		le = le->next;
	}

	return NULL;
}


size_t odict_count(const struct odict *o, bool nested)
{
	struct le *le;
	size_t n = 0;

	if (!o)
		return 0;

	if (!nested)
		return list_count(&o->lst);

	for (le=o->lst.head; le; le=le->next) {

		const struct odict_entry *e = le->data;

		switch (e->type) {

		case ODICT_OBJECT:
		case ODICT_ARRAY:
			n += odict_count(e->u.odict, true);
			break;

		default:
			n += 1;  /* count all entries */
			break;
		}
	}

	return n;
}


int odict_debug(struct re_printf *pf, const struct odict *o)
{
	struct le *le;
	int err;

	if (!o)
		return 0;

	err = re_hprintf(pf, "{");

	for (le=o->lst.head; le; le=le->next) {

		const struct odict_entry *e = le->data;

		err |= re_hprintf(pf, " %H", odict_entry_debug, e);
	}

	err |= re_hprintf(pf, " }");

	return err;
}
