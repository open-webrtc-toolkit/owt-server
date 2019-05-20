/**
 * @file src/account.c  User-Agent account
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re/re.h>
#include <baresip.h>
#include "core.h"


enum {
	REG_INTERVAL    = 3600,
};


static void destructor(void *arg)
{
	struct account *acc = arg;
	size_t i;

	list_clear(&acc->aucodecl);
	list_clear(&acc->vidcodecl);
	mem_deref(acc->auth_user);
	mem_deref(acc->auth_pass);
	for (i=0; i<ARRAY_SIZE(acc->outbound); i++)
		mem_deref(acc->outbound[i]);
	mem_deref(acc->regq);
	mem_deref(acc->rtpkeep);
	mem_deref(acc->sipnat);
	mem_deref(acc->stun_user);
	mem_deref(acc->stun_pass);
	mem_deref(acc->stun_host);
	mem_deref(acc->mnatid);
	mem_deref(acc->mencid);
	mem_deref(acc->aor);
	mem_deref(acc->dispname);
	mem_deref(acc->buf);
}


static int param_dstr(char **dstr, const struct pl *params, const char *name)
{
	struct pl pl;

	if (msg_param_decode(params, name, &pl))
		return 0;

	return pl_strdup(dstr, &pl);
}


static int param_u32(uint32_t *v, const struct pl *params, const char *name)
{
	struct pl pl;

	if (msg_param_decode(params, name, &pl))
		return 0;

	*v = pl_u32(&pl);

	return 0;
}


static int stunsrv_decode(struct account *acc, const struct sip_addr *aor)
{
	struct pl srv;
	struct uri uri;
	int err;

	if (!acc || !aor)
		return EINVAL;

	memset(&uri, 0, sizeof(uri));

	if (0 == msg_param_decode(&aor->params, "stunserver", &srv)) {

		info("using stunserver: '%r'\n", &srv);

		err = uri_decode(&uri, &srv);
		if (err) {
			warning("account: %r: decode failed: %m\n", &srv, err);
			memset(&uri, 0, sizeof(uri));
		}

		if (0 != pl_strcasecmp(&uri.scheme, "stun")) {
			warning("account: unknown scheme: %r\n", &uri.scheme);
			return EINVAL;
		}
	}

	err = 0;
	if (pl_isset(&uri.user))
		err |= pl_strdup(&acc->stun_user, &uri.user);
	else
		err |= pl_strdup(&acc->stun_user, &aor->uri.user);

	if (pl_isset(&uri.password))
		err |= pl_strdup(&acc->stun_pass, &uri.password);
	else
		err |= pl_strdup(&acc->stun_pass, &aor->uri.password);

	if (pl_isset(&uri.host))
		err |= pl_strdup(&acc->stun_host, &uri.host);
	else
		err |= pl_strdup(&acc->stun_host, &aor->uri.host);

	acc->stun_port = uri.port;

	return err;
}


/** Decode media parameters */
static int media_decode(struct account *acc, const struct pl *prm)
{
	int err = 0;

	if (!acc || !prm)
		return EINVAL;

	err |= param_dstr(&acc->mencid,  prm, "mediaenc");
	err |= param_dstr(&acc->mnatid,  prm, "medianat");
	err |= param_dstr(&acc->rtpkeep, prm, "rtpkeep" );
	err |= param_u32(&acc->ptime,    prm, "ptime"   );

	return err;
}


/* Decode answermode parameter */
static void answermode_decode(struct account *prm, const struct pl *pl)
{
	struct pl amode;

	if (0 == msg_param_decode(pl, "answermode", &amode)) {

		if (0 == pl_strcasecmp(&amode, "manual")) {
			prm->answermode = ANSWERMODE_MANUAL;
		}
		else if (0 == pl_strcasecmp(&amode, "early")) {
			prm->answermode = ANSWERMODE_EARLY;
		}
		else if (0 == pl_strcasecmp(&amode, "auto")) {
			prm->answermode = ANSWERMODE_AUTO;
		}
		else {
			warning("account: answermode unknown (%r)\n", &amode);
			prm->answermode = ANSWERMODE_MANUAL;
		}
	}
}


static int csl_parse(struct pl *pl, char *str, size_t sz)
{
	struct pl ws = PL_INIT, val, ws2 = PL_INIT, cma = PL_INIT;
	int err;

	err = re_regex(pl->p, pl->l, "[ \t]*[^, \t]+[ \t]*[,]*",
		       &ws, &val, &ws2, &cma);
	if (err)
		return err;

	pl_advance(pl, ws.l + val.l + ws2.l + cma.l);

	(void)pl_strcpy(&val, str, sz);

	return 0;
}


static int audio_codecs_decode(struct account *acc, const struct pl *prm)
{
	struct pl tmp;

	if (!acc || !prm)
		return EINVAL;

	list_init(&acc->aucodecl);

	if (0 == msg_param_exists(prm, "audio_codecs", &tmp)) {
		struct pl acs;
		char cname[64];
		unsigned i = 0;

		if (msg_param_decode(prm, "audio_codecs", &acs))
			return 0;

		while (0 == csl_parse(&acs, cname, sizeof(cname))) {
			struct aucodec *ac;
			struct pl pl_cname, pl_srate, pl_ch = PL_INIT;
			uint32_t srate = 8000;
			uint8_t ch = 1;

			/* Format: "codec/srate/ch" */
			if (0 == re_regex(cname, str_len(cname),
					  "[^/]+/[0-9]+[/]*[0-9]*",
					  &pl_cname, &pl_srate,
					  NULL, &pl_ch)) {
				(void)pl_strcpy(&pl_cname, cname,
						sizeof(cname));
				srate = pl_u32(&pl_srate);
				if (pl_isset(&pl_ch))
					ch = pl_u32(&pl_ch);
			}

			ac = (struct aucodec *)aucodec_find(cname, srate, ch);
			if (!ac) {
				warning("account: audio codec not found:"
					" %s/%u/%d\n",
					cname, srate, ch);
				continue;
			}

			/* NOTE: static list with references to aucodec */
			list_append(&acc->aucodecl, &acc->acv[i++], ac);

			if (i >= ARRAY_SIZE(acc->acv))
				break;
		}
	}

	return 0;
}


#ifdef USE_VIDEO
static int video_codecs_decode(struct account *acc, const struct pl *prm)
{
	struct pl tmp;

	if (!acc || !prm)
		return EINVAL;

	list_init(&acc->vidcodecl);

	if (0 == msg_param_exists(prm, "video_codecs", &tmp)) {
		struct pl vcs;
		char cname[64];
		unsigned i = 0;

		if (msg_param_decode(prm, "video_codecs", &vcs))
			return 0;

		while (0 == csl_parse(&vcs, cname, sizeof(cname))) {
			struct vidcodec *vc;

			vc = (struct vidcodec *)vidcodec_find(cname, NULL);
			if (!vc) {
				warning("account: video codec not found: %s\n",
					cname);
				continue;
			}

			/* NOTE: static list with references to vidcodec */
			list_append(&acc->vidcodecl, &acc->vcv[i++], vc);

			if (i >= ARRAY_SIZE(acc->vcv))
				break;
		}
	}

	return 0;
}
#endif


static int sip_params_decode(struct account *acc, const struct sip_addr *aor)
{
	struct pl auth_user;
	size_t i;
	int err = 0;

	if (!acc || !aor)
		return EINVAL;

	acc->regint = REG_INTERVAL + (rand_u32()&0xff);
	err |= param_u32(&acc->regint, &aor->params, "regint");

	acc->pubint = 0;
	err |= param_u32(&acc->pubint, &aor->params, "pubint");

	err |= param_dstr(&acc->regq, &aor->params, "regq");

	for (i=0; i<ARRAY_SIZE(acc->outbound); i++) {

		char expr[16] = "outbound";

		expr[8] = i + 1 + 0x30;
		expr[9] = '\0';

		err |= param_dstr(&acc->outbound[i], &aor->params, expr);
	}

	/* backwards compat */
	if (!acc->outbound[0]) {
		err |= param_dstr(&acc->outbound[0], &aor->params, "outbound");
	}

	err |= param_dstr(&acc->sipnat, &aor->params, "sipnat");

	if (0 == msg_param_decode(&aor->params, "auth_user", &auth_user))
		err |= pl_strdup(&acc->auth_user, &auth_user);
	else
		err |= pl_strdup(&acc->auth_user, &aor->uri.user);

	if (pl_isset(&aor->dname))
		err |= pl_strdup(&acc->dispname, &aor->dname);

	return err;
}


static int encode_uri_user(struct re_printf *pf, const struct uri *uri)
{
	struct uri uuri = *uri;

	uuri.password = uuri.params = uuri.headers = pl_null;

	return uri_encode(pf, &uuri);
}


int account_alloc(struct account **accp, const char *sipaddr)
{
	struct account *acc;
	struct pl pl;
	int err = 0;

	if (!accp || !sipaddr)
		return EINVAL;

	acc = mem_zalloc(sizeof(*acc), destructor);
	if (!acc)
		return ENOMEM;

	err = str_dup(&acc->buf, sipaddr);
	if (err)
		goto out;

	pl_set_str(&pl, acc->buf);
	err = sip_addr_decode(&acc->laddr, &pl);
	if (err) {
		warning("account: invalid SIP address: `%r'\n", &pl);
		goto out;
	}

	acc->luri = acc->laddr.uri;
	acc->luri.password = pl_null;

	err = re_sdprintf(&acc->aor, "%H", encode_uri_user, &acc->luri);
	if (err)
		goto out;

	/* Decode parameters */
	acc->ptime = 20;
	err |= sip_params_decode(acc, &acc->laddr);
	       answermode_decode(acc, &acc->laddr.params);
	err |= audio_codecs_decode(acc, &acc->laddr.params);
#ifdef USE_VIDEO
	err |= video_codecs_decode(acc, &acc->laddr.params);
#endif
	err |= media_decode(acc, &acc->laddr.params);
	if (err)
		goto out;

	/* optional password prompt */
	if (!pl_isset(&acc->laddr.uri.password)) {
		warning("no password for %s@%s: ",
				&acc->luri.user, &acc->luri.host);

		/* TODO: move interactive code away from CORE, to a module */
	}
	else {
		err = pl_strdup(&acc->auth_pass, &acc->laddr.uri.password);
		if (err)
			goto out;
	}

	if (acc->mnatid) {
		err = stunsrv_decode(acc, &acc->laddr);
		if (err)
			goto out;

		acc->mnat = mnat_find(acc->mnatid);
		if (!acc->mnat) {
			warning("account: medianat not found: `%s'\n",
				acc->mnatid);
		}
	}

	if (acc->mencid) {
		acc->menc = menc_find(acc->mencid);
		if (!acc->menc) {
			warning("account: mediaenc not found: `%s'\n",
				acc->mencid);
		}
	}

 out:
	if (err)
		mem_deref(acc);
	else
		*accp = acc;

	return err;
}


/**
 * Sets the displayed name. Pass null in dname to disable display name
 *
 * @param acc      User-Agent account
 * @param dname    Display name (NULL to disable)
 *
 * @return 0 if success, otherwise errorcode
 */
int account_set_display_name(struct account *acc, const char *dname)
{
	if (!acc)
		return EINVAL;

	acc->dispname = mem_deref(acc->dispname);

	if (dname)
		return str_dup(&acc->dispname, dname);

	return 0;
}


/**
 * Authenticate a User-Agent (UA)
 *
 * @param acc      User-Agent account
 * @param username Pointer to allocated username string
 * @param password Pointer to allocated password string
 * @param realm    Realm string
 *
 * @return 0 if success, otherwise errorcode
 */
int account_auth(const struct account *acc, char **username, char **password,
		 const char *realm)
{
	if (!acc)
		return EINVAL;

	(void)realm;

	*username = mem_ref(acc->auth_user);
	*password = mem_ref(acc->auth_pass);

	return 0;
}


struct list *account_aucodecl(const struct account *acc)
{
	return (acc && !list_isempty(&acc->aucodecl))
		? (struct list *)&acc->aucodecl : aucodec_list();
}


#ifdef USE_VIDEO
struct list *account_vidcodecl(const struct account *acc)
{
	return (acc && !list_isempty(&acc->vidcodecl))
		? (struct list *)&acc->vidcodecl : vidcodec_list();
}
#endif


struct sip_addr *account_laddr(const struct account *acc)
{
	return acc ? (struct sip_addr *)&acc->laddr : NULL;
}


uint32_t account_regint(const struct account *acc)
{
	return acc ? acc->regint : 0;
}


uint32_t account_pubint(const struct account *acc)
{
	return acc ? acc->pubint : 0;
}


enum answermode account_answermode(const struct account *acc)
{
	return acc ? acc->answermode : ANSWERMODE_MANUAL;
}


static const char *answermode_str(enum answermode mode)
{
	switch (mode) {

	case ANSWERMODE_MANUAL: return "manual";
	case ANSWERMODE_EARLY:  return "early";
	case ANSWERMODE_AUTO:   return "auto";
	default: return "???";
	}
}


int account_debug(struct re_printf *pf, const struct account *acc)
{
	struct le *le;
	size_t i;
	int err = 0;

	if (!acc)
		return 0;

	err |= re_hprintf(pf, "\nAccount:\n");

	err |= re_hprintf(pf, " address:      %s\n", acc->buf);
	err |= re_hprintf(pf, " luri:         %H\n",
			  uri_encode, &acc->luri);
	err |= re_hprintf(pf, " aor:          %s\n", acc->aor);
	err |= re_hprintf(pf, " dispname:     %s\n", acc->dispname);
	err |= re_hprintf(pf, " answermode:   %s\n",
			  answermode_str(acc->answermode));
	if (!list_isempty(&acc->aucodecl)) {
		err |= re_hprintf(pf, " audio_codecs:");
		for (le = list_head(&acc->aucodecl); le; le = le->next) {
			const struct aucodec *ac = le->data;
			err |= re_hprintf(pf, " %s/%u/%u",
					  ac->name, ac->srate, ac->ch);
		}
		err |= re_hprintf(pf, "\n");
	}
	err |= re_hprintf(pf, " auth_user:    %s\n", acc->auth_user);
	err |= re_hprintf(pf, " mediaenc:     %s\n",
			  acc->mencid ? acc->mencid : "none");
	err |= re_hprintf(pf, " medianat:     %s\n",
			  acc->mnatid ? acc->mnatid : "none");
	for (i=0; i<ARRAY_SIZE(acc->outbound); i++) {
		if (acc->outbound[i]) {
			err |= re_hprintf(pf, " outbound%d:    %s\n",
					  i+1, acc->outbound[i]);
		}
	}
	err |= re_hprintf(pf, " ptime:        %u\n", acc->ptime);
	err |= re_hprintf(pf, " regint:       %u\n", acc->regint);
	err |= re_hprintf(pf, " pubint:       %u\n", acc->pubint);
	err |= re_hprintf(pf, " regq:         %s\n", acc->regq);
	err |= re_hprintf(pf, " rtpkeep:      %s\n", acc->rtpkeep);
	err |= re_hprintf(pf, " sipnat:       %s\n", acc->sipnat);
	err |= re_hprintf(pf, " stunserver:   stun:%s@%s:%u\n",
			  acc->stun_user, acc->stun_host, acc->stun_port);
	if (!list_isempty(&acc->vidcodecl)) {
		err |= re_hprintf(pf, " video_codecs:");
		for (le = list_head(&acc->vidcodecl); le; le = le->next) {
			const struct vidcodec *vc = le->data;
			err |= re_hprintf(pf, " %s", vc->name);
		}
		err |= re_hprintf(pf, "\n");
	}

	return err;
}
