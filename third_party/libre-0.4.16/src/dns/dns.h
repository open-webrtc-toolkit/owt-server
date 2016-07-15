/**
 * @file dns.h  Internal DNS header file
 *
 * Copyright (C) 2010 Creytiv.com
 */


#ifdef HAVE_LIBRESOLV
int get_resolv_dns(char *domain, size_t dsize, struct sa *nsv, uint32_t *n);
#endif
#ifdef WIN32
int get_windns(char *domain, size_t dsize, struct sa *nav, uint32_t *n);
#endif
#ifdef DARWIN
int get_darwin_dns(char *domain, size_t dsize, struct sa *nsv, uint32_t *n);
#endif
