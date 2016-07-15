/**
 * @file stunstr.c  STUN Strings
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_stun.h>


/* STUN Reason Phrase */
const char *stun_reason_300 = "Try Alternate";
const char *stun_reason_400 = "Bad Request";
const char *stun_reason_401 = "Unauthorized";
const char *stun_reason_403 = "Forbidden";
const char *stun_reason_420 = "Unknown Attribute";
const char *stun_reason_437 = "Allocation Mismatch";
const char *stun_reason_438 = "Stale Nonce";
const char *stun_reason_440 = "Address Family not Supported";
const char *stun_reason_441 = "Wrong Credentials";
const char *stun_reason_442 = "Unsupported Transport Protocol";
const char *stun_reason_443 = "Peer Address Family Mismatch";
const char *stun_reason_486 = "Allocation Quota Reached";
const char *stun_reason_500 = "Server Error";
const char *stun_reason_508 = "Insufficient Capacity";


/**
 * Get the name of a given STUN Transport
 *
 * @param tp STUN Transport
 *
 * @return Name of the corresponding STUN Transport
 */
const char *stun_transp_name(enum stun_transp tp)
{
	switch (tp) {

	case STUN_TRANSP_UDP:  return "UDP";
	case STUN_TRANSP_TCP:  return "TCP";
	case STUN_TRANSP_DTLS: return "DTLS";
	default:               return "???";
	}
}
