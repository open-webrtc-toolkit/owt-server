/**
 * @file re_json.h  Interface to JavaScript Object Notation (JSON) -- RFC 7159
 *
 * Copyright (C) 2010 - 2015 Creytiv.com
 */

enum json_typ {
	JSON_STRING,
	JSON_INT,
	JSON_DOUBLE,
	JSON_BOOL,
	JSON_NULL,
};

struct json_value {
	union {
		char *str;
		int64_t integer;
		double dbl;
		bool boolean;
	} v;
	enum json_typ type;
};

struct json_handlers;

typedef int (json_object_entry_h)(const char *name,
				  const struct json_value *value, void *arg);
typedef int (json_array_entry_h)(unsigned idx,
				 const struct json_value *value, void *arg);
typedef int (json_object_h)(const char *name, unsigned idx,
			    struct json_handlers *h);
typedef int (json_array_h)(const char *name, unsigned idx,
			   struct json_handlers *h);

struct json_handlers {
	json_object_h *oh;
	json_array_h *ah;
	json_object_entry_h *oeh;
	json_array_entry_h *aeh;
	void *arg;
};

int json_decode(const char *str, size_t len, unsigned maxdepth,
		json_object_h *oh, json_array_h *ah,
		json_object_entry_h *oeh, json_array_entry_h *aeh, void *arg);

int json_decode_odict(struct odict **op, uint32_t hash_size, const char *str,
		      size_t len, unsigned maxdepth);
int json_encode_odict(struct re_printf *pf, const struct odict *o);
