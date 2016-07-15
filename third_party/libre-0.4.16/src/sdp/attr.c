/**
 * @file sdp/attr.c  SDP Attributes
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_sdp.h>
#include "sdp.h"


struct sdp_attr {
	struct le le;
	char *name;
	char *val;
};


static void destructor(void *arg)
{
	struct sdp_attr *attr = arg;

	list_unlink(&attr->le);
	mem_deref(attr->name);
	mem_deref(attr->val);
}


int sdp_attr_add(struct list *lst, struct pl *name, struct pl *val)
{
	struct sdp_attr *attr;
	int err;

	attr = mem_zalloc(sizeof(*attr), destructor);
	if (!attr)
		return ENOMEM;

	list_append(lst, &attr->le, attr);

	err = pl_strdup(&attr->name, name);

	if (pl_isset(val))
		err |= pl_strdup(&attr->val, val);

	if (err)
		mem_deref(attr);

	return err;
}


int sdp_attr_addv(struct list *lst, const char *name, const char *val,
		  va_list ap)
{
	struct sdp_attr *attr;
	int err;

	attr = mem_zalloc(sizeof(*attr), destructor);
	if (!attr)
		return ENOMEM;

	list_append(lst, &attr->le, attr);

	err = str_dup(&attr->name, name);

	if (str_isset(val))
		err |= re_vsdprintf(&attr->val, val, ap);

	if (err)
		mem_deref(attr);

	return err;
}


void sdp_attr_del(const struct list *lst, const char *name)
{
	struct le *le = list_head(lst);

	while (le) {

		struct sdp_attr *attr = le->data;

		le = le->next;

		if (0 == str_casecmp(name, attr->name))
			mem_deref(attr);
	}
}


const char *sdp_attr_apply(const struct list *lst, const char *name,
			   sdp_attr_h *attrh, void *arg)
{
	struct le *le = list_head(lst);

	while (le) {

		const struct sdp_attr *attr = le->data;

		le = le->next;

		if (name && (!attr->name || strcmp(name, attr->name)))
			continue;

		if (!attrh || attrh(attr->name, attr->val?attr->val : "", arg))
			return attr->val ? attr->val : "";
	}

	return NULL;
}


int sdp_attr_print(struct re_printf *pf, const struct sdp_attr *attr)
{
	if (!attr)
		return 0;

	if (attr->val)
		return re_hprintf(pf, "a=%s:%s\r\n", attr->name, attr->val);
	else
		return re_hprintf(pf, "a=%s\r\n", attr->name);
}


int sdp_attr_debug(struct re_printf *pf, const struct sdp_attr *attr)
{
	if (!attr)
		return 0;

	if (attr->val)
		return re_hprintf(pf, "%s='%s'", attr->name, attr->val);
	else
		return re_hprintf(pf, "%s", attr->name);
}
