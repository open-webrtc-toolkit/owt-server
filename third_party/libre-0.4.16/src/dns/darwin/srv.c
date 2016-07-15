/**
 * @file darwin/srv.c  Get DNS Server IP code for Mac OS X
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_dns.h>
#include "../dns.h"
#define __CF_USE_FRAMEWORK_INCLUDES__
#include <SystemConfiguration/SystemConfiguration.h>


int get_darwin_dns(char *domain, size_t dsize, struct sa *nsv, uint32_t *n)
{
#if TARGET_OS_IPHONE
	(void)domain;
	(void)dsize;
	(void)nsv;
	(void)n;
	return ENOSYS;
#else
	SCDynamicStoreContext context = {0, NULL, NULL, NULL, NULL};
	CFArrayRef addresses, domains;
	SCDynamicStoreRef store;
	CFStringRef key, dom;
	CFDictionaryRef dict;
	uint32_t c, i;
	int err = ENOENT;

	if (!nsv || !n)
		return EINVAL;

	store = SCDynamicStoreCreate(NULL, CFSTR("get_darwin_dns"),
				     NULL, &context);
	if (!store)
		return ENOENT;

	key = CFSTR("State:/Network/Global/DNS");
	dict = SCDynamicStoreCopyValue(store, key);
	if (!dict)
		goto out1;

	addresses = CFDictionaryGetValue(dict, kSCPropNetDNSServerAddresses);
	if (!addresses)
		goto out;

	c = (uint32_t)CFArrayGetCount(addresses);
	*n = min(*n, c);

	for (i=0; i<*n; i++) {
		CFStringRef address = CFArrayGetValueAtIndex(addresses, i);
		char str[64];

		CFStringGetCString(address, str, sizeof(str),
				   kCFStringEncodingUTF8);

		err = sa_set_str(&nsv[i], str, DNS_PORT);
		if (err)
			break;
	}

	domains = CFDictionaryGetValue(dict, kSCPropNetDNSSearchDomains);
	if (!domains)
		goto out;

	if (CFArrayGetCount(domains) < 1)
		goto out;

	dom = CFArrayGetValueAtIndex(domains, 0);
	CFStringGetCString(dom, domain, dsize, kCFStringEncodingUTF8);

 out:
	CFRelease(dict);
 out1:
	CFRelease(store);

	return err;
#endif
}
