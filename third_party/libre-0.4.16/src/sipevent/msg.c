/**
 * @file sipevent/msg.c  SIP event messages
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_uri.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_msg.h>
#include <re_sip.h>
#include <re_sipevent.h>


int sipevent_event_decode(struct sipevent_event *se, const struct pl *pl)
{
	struct pl param;
	int err;

	if (!se || !pl)
		return EINVAL;

	err = re_regex(pl->p, pl->l, "[^; \t\r\n]+[ \t\r\n]*[^]*",
		       &se->event, NULL, &se->params);
	if (err)
		return EBADMSG;

	if (!msg_param_decode(&se->params, "id", &param))
		se->id = param;
	else
		se->id = pl_null;

	return 0;
}


int sipevent_substate_decode(struct sipevent_substate *ss, const struct pl *pl)
{
	struct pl state, param;
	int err;

	if (!ss || !pl)
		return EINVAL;

	err = re_regex(pl->p, pl->l, "[a-z]+[ \t\r\n]*[^]*",
		       &state, NULL, &ss->params);
	if (err)
		return EBADMSG;

	if (!pl_strcasecmp(&state, "active"))
		ss->state = SIPEVENT_ACTIVE;
	else if (!pl_strcasecmp(&state, "pending"))
		ss->state = SIPEVENT_PENDING;
	else if (!pl_strcasecmp(&state, "terminated"))
		ss->state = SIPEVENT_TERMINATED;
	else
		ss->state = -1;

	if (!msg_param_decode(&ss->params, "reason", &param)) {

		if (!pl_strcasecmp(&param, "deactivated"))
			ss->reason = SIPEVENT_DEACTIVATED;
		else if (!pl_strcasecmp(&param, "probation"))
			ss->reason = SIPEVENT_PROBATION;
		else if (!pl_strcasecmp(&param, "rejected"))
			ss->reason = SIPEVENT_REJECTED;
		else if (!pl_strcasecmp(&param, "timeout"))
			ss->reason = SIPEVENT_TIMEOUT;
		else if (!pl_strcasecmp(&param, "giveup"))
			ss->reason = SIPEVENT_GIVEUP;
		else if (!pl_strcasecmp(&param, "noresource"))
			ss->reason = SIPEVENT_NORESOURCE;
		else
			ss->reason = -1;
	}
	else {
		ss->reason = -1;
	}

	if (!msg_param_decode(&ss->params, "expires", &param))
		ss->expires = param;
	else
		ss->expires = pl_null;

	if (!msg_param_decode(&ss->params, "retry-after", &param))
		ss->retry_after = param;
	else
		ss->retry_after = pl_null;

	return 0;
}


const char *sipevent_substate_name(enum sipevent_subst state)
{
	switch (state) {

	case SIPEVENT_ACTIVE:     return "active";
	case SIPEVENT_PENDING:    return "pending";
	case SIPEVENT_TERMINATED: return "terminated";
	default:                  return "unknown";
	}
}


const char *sipevent_reason_name(enum sipevent_reason reason)
{
	switch (reason) {

	case SIPEVENT_DEACTIVATED: return "deactivated";
	case SIPEVENT_PROBATION:   return "probation";
	case SIPEVENT_REJECTED:    return "rejected";
	case SIPEVENT_TIMEOUT:     return "timeout";
	case SIPEVENT_GIVEUP:      return "giveup";
	case SIPEVENT_NORESOURCE:  return "noresource";
	default:                   return "unknown";
	}
}
