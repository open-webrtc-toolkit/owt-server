/**
 * @file re_mod.h  Interface to loadable modules
 *
 * Copyright (C) 2010 Creytiv.com
 */


/**
 * @def MOD_PRE
 *
 * Module Prefix
 *
 * @def MOD_EXT
 *
 * Module Extension
 */
#if defined (WIN32)
#define MOD_PRE ""
#define MOD_EXT ".dll"
#elif defined (__SYMBIAN32__)
#define MOD_PRE "mod_"
#define MOD_EXT ".dll"
#else
#define MOD_PRE ""
#define MOD_EXT ".so"
#endif


/** Symbol to enable exporting of functions from a module */
#ifdef WIN32
#define EXPORT_SYM __declspec(dllexport)
#else
#define EXPORT_SYM
#endif


/* ----- Module API ----- */


/**
 * Defines the module initialisation handler
 *
 * @return 0 for success, otherwise errorcode
 */
typedef int (mod_init_h)(void);

/**
 * Defines the module close handler
 *
 * @return 0 for success, otherwise errorcode
 */
typedef int (mod_close_h)(void);


struct mod;
struct re_printf;


/** Defines the module export */
struct mod_export {
	const char *name;    /**< Module name             */
	const char *type;    /**< Module type             */
	mod_init_h *init;    /**< Module init handler     */
	mod_close_h *close;  /**< Module close handler    */
};


/* ----- Application API ----- */

void        mod_init(void);
void        mod_close(void);

int         mod_load(struct mod **mp, const char *name);
int         mod_add(struct mod **mp, const struct mod_export *me);
struct mod *mod_find(const char *name);
const struct mod_export *mod_export(const struct mod *m);
int         mod_debug(struct re_printf *pf, void *unused);
