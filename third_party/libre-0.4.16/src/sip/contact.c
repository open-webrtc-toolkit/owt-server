/**
 * @file sip/contact.c  SIP contact functions
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_uri.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_msg.h>
#include <re_sip.h>


/**
 * Set contact parameters
 *
 * @param contact SIP Contact object
 * @param uri     Username or URI
 * @param addr    IP-address and port
 * @param tp      SIP Transport
 */
void sip_contact_set(struct sip_contact *contact, const char *uri,
		     const struct sa *addr, enum sip_transp tp)
{
	if (!contact)
		return;

	contact->uri  = uri;
	contact->addr = addr;
	contact->tp   = tp;
}


/**
 * Print contact header
 *
 * @param pf      Print function
 * @param contact SIP Contact object
 *
 * @return 0 for success, otherwise errorcode
 */
int sip_contact_print(struct re_printf *pf, const struct sip_contact *contact)
{
	if (!contact)
		return 0;

	if (contact->uri && strchr(contact->uri, ':'))
		return re_hprintf(pf, "Contact: <%s>\r\n", contact->uri);
	else
		return re_hprintf(pf, "Contact: <sip:%s@%J%s>\r\n",
				  contact->uri,
				  contact->addr,
				  sip_transp_param(contact->tp));
}
