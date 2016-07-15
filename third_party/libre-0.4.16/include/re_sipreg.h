/**
 * @file re_sipreg.h  SIP Registration
 *
 * Copyright (C) 2010 Creytiv.com
 */

struct sipreg;


int sipreg_register(struct sipreg **regp, struct sip *sip, const char *reg_uri,
		    const char *to_uri, const char *from_uri, uint32_t expires,
		    const char *cuser, const char *routev[], uint32_t routec,
		    int regid, sip_auth_h *authh, void *aarg, bool aref,
		    sip_resp_h *resph, void *arg,
		    const char *params, const char *fmt, ...);
