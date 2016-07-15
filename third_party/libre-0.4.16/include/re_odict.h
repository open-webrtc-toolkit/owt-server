/**
 * @file re_odict.h  Interface to Ordered Dictionary
 *
 * Copyright (C) 2010 - 2015 Creytiv.com
 */

enum odict_type {
	ODICT_OBJECT,
	ODICT_ARRAY,
	ODICT_STRING,
	ODICT_INT,
	ODICT_DOUBLE,
	ODICT_BOOL,
	ODICT_NULL,
};

struct odict {
	struct list lst;
	struct hash *ht;
};

struct odict_entry {
	struct le le, he;
	char *key;
	union {
		struct odict *odict;   /* ODICT_OBJECT / ODICT_ARRAY */
		char *str;             /* ODICT_STRING */
		int64_t integer;       /* ODICT_INT    */
		double dbl;            /* ODICT_DOUBLE */
		bool boolean;          /* ODICT_BOOL   */
	} u;
	enum odict_type type;
};

int odict_alloc(struct odict **op, uint32_t hash_size);
const struct odict_entry *odict_lookup(const struct odict *o, const char *key);
size_t odict_count(const struct odict *o, bool nested);
int odict_debug(struct re_printf *pf, const struct odict *o);

int odict_entry_add(struct odict *o, const char *key,
		    enum odict_type type, ...);
void odict_entry_del(struct odict *o, const char *key);
int odict_entry_debug(struct re_printf *pf, const struct odict_entry *e);

bool odict_type_iscontainer(enum odict_type type);
bool odict_type_isreal(enum odict_type type);
const char *odict_type_name(enum odict_type type);
