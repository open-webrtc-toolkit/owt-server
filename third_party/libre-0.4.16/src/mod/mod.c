/**
 * @file mod.c  Loadable modules
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <sys/types.h>
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_mod.h>
#include "mod_internal.h"


#define DEBUG_MODULE "mod"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/** Defines a loadable module */
struct mod {
	struct le le;                 /**< Linked list element */
	void *h;                      /**< Module handler      */
	const struct mod_export *me;  /**< Module exports      */
};


static struct list modl;  /* struct mod */


/**
 * Initialise module loading
 */
void mod_init(void)
{
	list_init(&modl);
}


/**
 * Unload all modules
 */
void mod_close(void)
{
	list_flush(&modl);
}


static void mod_destructor(void *data)
{
	struct mod *m = data;
	const struct mod_export *me = m->me;
	int err;

	if (me && me->close && (err = me->close())) {
		DEBUG_NOTICE("close: error (%m)\n", err);
	}

	list_unlink(&m->le);

	_mod_close(m->h);
}


/**
 * Find a module by name in the list of loaded modules
 *
 * @param name Name of module to find
 *
 * @return Module if found, NULL if not found
 */
struct mod *mod_find(const char *name)
{
	struct le *le;
	struct pl x;

	if (!name)
		return NULL;

	if (re_regex(name, strlen(name), "[/]*[^./]+" MOD_EXT, NULL, &x))
		return NULL;

	for (le = modl.head; le; le = le->next) {
		struct mod *m = le->data;

		if (0 == pl_strcasecmp(&x, m->me->name))
			return m;
	}

	return NULL;
}


/**
 * Load and initialise a loadable module by name
 *
 * @param mp   Pointer to allocated module object
 * @param name Name of loadable module
 *
 * @return 0 if success, otherwise errorcode
 */
int mod_load(struct mod **mp, const char *name)
{
	struct mod *m;
	int err = 0;

	if (!mp || !name)
		return EINVAL;

	/* check if already loaded */
	m = mod_find(name);
	if (m) {
		DEBUG_NOTICE("module already loaded: %s\n", name);
		return EALREADY;
	}

	m = mem_zalloc(sizeof(*m), mod_destructor);
	if (!m)
		return ENOMEM;

	list_append(&modl, &m->le, m);

	m->h = _mod_open(name);
	if (!m->h) {
		err = ENOENT;
		goto out;
	}

	m->me = _mod_sym(m->h, "exports");
	if (!m->me) {
		err = ELIBBAD;
		goto out;
	}

	if (m->me->init && (err = m->me->init()))
		goto out;

 out:
	if (err)
		mem_deref(m);
	else
		*mp = m;

	return err;
}


/**
 * Add and initialise an external module with exports
 *
 * @param mp   Pointer to allocated module object
 * @param me   Module exports
 *
 * @return 0 if success, otherwise errorcode
 */
int mod_add(struct mod **mp, const struct mod_export *me)
{
	struct mod *m;
	int err = 0;

	if (!mp || !me)
		return EINVAL;

	/* check if already loaded */
	m = mod_find(me->name);
	if (m) {
		DEBUG_NOTICE("module already loaded: %s\n", me->name);
		return EALREADY;
	}

	m = mem_zalloc(sizeof(*m), mod_destructor);
	if (!m)
		return ENOMEM;

	list_append(&modl, &m->le, m);

	m->me = me;

	if (m->me->init)
		err = m->me->init();

	if (err)
		mem_deref(m);
	else
		*mp = m;

	return err;
}


/**
 * Get module export from a loadable module
 *
 * @param m Loadable module
 *
 * @return Module export
 */
const struct mod_export *mod_export(const struct mod *m)
{
	return m ? m->me : NULL;
}


/**
 * Debug loadable modules
 *
 * @param pf     Print handler for debug output
 * @param unused Unused parameter
 *
 * @return 0 if success, otherwise errorcode
 */
int mod_debug(struct re_printf *pf, void *unused)
{
	struct le *le;
	int err;

	(void)unused;

	err = re_hprintf(pf, "\n--- Modules (%u) ---\n", list_count(&modl));

	for (le = modl.head; le && !err; le = le->next) {
		const struct mod *m = le->data;
		const struct mod_export *me = m->me;

		err = re_hprintf(pf, " %16s type=%-12s ref=%u\n",
				 me->name, me->type, mem_nrefs(m));
	}

	err |= re_hprintf(pf, "\n");

	return err;
}
