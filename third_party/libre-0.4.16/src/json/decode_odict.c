/**
 * @file json/decode_odict.c  JSON odict decode
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


static int container_add(const char *name, unsigned idx,
			 enum odict_type type, struct json_handlers *h)
{
	struct odict *o = h->arg, *oc;
	char index[64];
	int err;

	if (!name) {
		if (re_snprintf(index, sizeof(index), "%u", idx) < 0)
			return ENOMEM;

		name = index;
	}

	err = odict_alloc(&oc, hash_bsize(o->ht));
	if (err)
		return err;

	err = odict_entry_add(o, name, type, oc);
	mem_deref(oc);
	h->arg = oc;

	return err;
}


static int object_handler(const char *name, unsigned idx,
			  struct json_handlers *h)
{
	return container_add(name, idx, ODICT_OBJECT, h);
}


static int array_handler(const char *name, unsigned idx,
			 struct json_handlers *h)
{
	return container_add(name, idx, ODICT_ARRAY, h);
}


static int entry_add(struct odict *o, const char *name,
		     const struct json_value *val)
{
	switch (val->type) {

	case JSON_STRING:
		return odict_entry_add(o, name, ODICT_STRING, val->v.str);

	case JSON_INT:
		return odict_entry_add(o, name, ODICT_INT, val->v.integer);

	case JSON_DOUBLE:
		return odict_entry_add(o, name, ODICT_DOUBLE, val->v.dbl);

	case JSON_BOOL:
		return odict_entry_add(o, name, ODICT_BOOL, val->v.boolean);

	case JSON_NULL:
		return odict_entry_add(o, name, ODICT_NULL);

	default:
		return ENOSYS;
	}
}


static int object_entry_handler(const char *name, const struct json_value *val,
				void *arg)
{
	struct odict *o = arg;

	return entry_add(o, name, val);
}


static int array_entry_handler(unsigned idx, const struct json_value *val,
			       void *arg)
{
	struct odict *o = arg;
	char index[64];

	if (re_snprintf(index, sizeof(index), "%u", idx) < 0)
		return ENOMEM;

	return entry_add(o, index, val);
}


int json_decode_odict(struct odict **op, uint32_t hash_size, const char *str,
		      size_t len, unsigned maxdepth)
{
	struct odict *o;
	int err;

	if (!op || !str)
		return EINVAL;

	err = odict_alloc(&o, hash_size);
	if (err)
		return err;

	err = json_decode(str, len, maxdepth, object_handler, array_handler,
			  object_entry_handler, array_entry_handler, o);
	if (err)
		mem_deref(o);
	else
		*op = o;

	return err;
}
