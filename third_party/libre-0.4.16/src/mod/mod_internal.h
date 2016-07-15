/**
 * @file mod_internal.h  Internal interface to loadable module
 *
 * Copyright (C) 2010 Creytiv.com
 */


#ifdef __cplusplus
extern "C" {
#endif


void *_mod_open(const char *name);
void *_mod_sym(void *h, const char *symbol);
void  _mod_close(void *h);


#ifdef __cplusplus
}
#endif
