/**
 * @file natstr.c  NAT Behaviour Discovery strings
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_stun.h>
#include <re_natbd.h>


/**
 * Get the name of the NAT Mapping/Filtering type
 *
 * @param type NAT Mapping/Filtering type
 *
 * @return Name of the NAT Mapping/Filtering type
 */
const char *nat_type_str(enum nat_type type)
{
	switch (type) {

	case NAT_TYPE_UNKNOWN:       return "Unknown";
	case NAT_TYPE_ENDP_INDEP:    return "Endpoint Independent";
	case NAT_TYPE_ADDR_DEP:      return "Address Dependent";
	case NAT_TYPE_ADDR_PORT_DEP: return "Address and Port Dependent";
	default:                     return "???";
	}
}
