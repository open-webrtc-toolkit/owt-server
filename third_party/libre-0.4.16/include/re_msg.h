/**
 * @file re_msg.h  Interface to generic message components
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** Content-Type */
struct msg_ctype {
	struct pl type;
	struct pl subtype;
	struct pl params;
};


int  msg_ctype_decode(struct msg_ctype *ctype, const struct pl *pl);
bool msg_ctype_cmp(const struct msg_ctype *ctype,
		   const char *type, const char *subtype);

int msg_param_decode(const struct pl *pl, const char *name, struct pl *val);
int msg_param_exists(const struct pl *pl, const char *name, struct pl *end);
