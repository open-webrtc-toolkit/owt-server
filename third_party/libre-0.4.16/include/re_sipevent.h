/**
 * @file re_sipevent.h  SIP Event Framework
 *
 * Copyright (C) 2010 Creytiv.com
 */


/* Message Components */

struct sipevent_event {
	struct pl event;
	struct pl params;
	struct pl id;
};

enum sipevent_subst {
	SIPEVENT_ACTIVE = 0,
	SIPEVENT_PENDING,
	SIPEVENT_TERMINATED,
};

enum sipevent_reason {
	SIPEVENT_DEACTIVATED = 0,
	SIPEVENT_PROBATION,
	SIPEVENT_REJECTED,
	SIPEVENT_TIMEOUT,
	SIPEVENT_GIVEUP,
	SIPEVENT_NORESOURCE,
};

struct sipevent_substate {
	enum sipevent_subst state;
	enum sipevent_reason reason;
	struct pl expires;
	struct pl retry_after;
	struct pl params;
};

int sipevent_event_decode(struct sipevent_event *se, const struct pl *pl);
int sipevent_substate_decode(struct sipevent_substate *ss,
			     const struct pl *pl);
const char *sipevent_substate_name(enum sipevent_subst state);
const char *sipevent_reason_name(enum sipevent_reason reason);


/* Listener Socket */

struct sipevent_sock;

int sipevent_listen(struct sipevent_sock **sockp, struct sip *sip,
		    uint32_t htsize_not, uint32_t htsize_sub,
		    sip_msg_h *subh, void *arg);


/* Notifier */

struct sipnot;

typedef void (sipnot_close_h)(int err, const struct sip_msg *msg, void *arg);

int sipevent_accept(struct sipnot **notp, struct sipevent_sock *sock,
		    const struct sip_msg *msg, struct sip_dialog *dlg,
		    const struct sipevent_event *event,
		    uint16_t scode, const char *reason, uint32_t expires_min,
		    uint32_t expires_dfl, uint32_t expires_max,
		    const char *cuser, const char *ctype,
		    sip_auth_h *authh, void *aarg, bool aref,
		    sipnot_close_h *closeh, void *arg, const char *fmt, ...);
int sipevent_notify(struct sipnot *sipnot, struct mbuf *mb,
		    enum sipevent_subst state, enum sipevent_reason reason,
		    uint32_t retry_after);
int sipevent_notifyf(struct sipnot *sipnot, struct mbuf **mbp,
		     enum sipevent_subst state, enum sipevent_reason reason,
		     uint32_t retry_after, const char *fmt, ...);


/* Subscriber */

struct sipsub;

typedef int  (sipsub_fork_h)(struct sipsub **subp, struct sipsub *osub,
			     const struct sip_msg *msg, void *arg);
typedef void (sipsub_notify_h)(struct sip *sip, const struct sip_msg *msg,
			       void *arg);
typedef void (sipsub_close_h)(int err, const struct sip_msg *msg,
			      const struct sipevent_substate *substate,
			      void *arg);

int sipevent_subscribe(struct sipsub **subp, struct sipevent_sock *sock,
		       const char *uri, const char *from_name,
		       const char *from_uri, const char *event, const char *id,
		       uint32_t expires, const char *cuser,
		       const char *routev[], uint32_t routec,
		       sip_auth_h *authh, void *aarg, bool aref,
		       sipsub_fork_h *forkh, sipsub_notify_h *notifyh,
		       sipsub_close_h *closeh, void *arg,
		       const char *fmt, ...);
int sipevent_dsubscribe(struct sipsub **subp, struct sipevent_sock *sock,
			struct sip_dialog *dlg, const char *event,
			const char *id, uint32_t expires, const char *cuser,
			sip_auth_h *authh, void *aarg, bool aref,
			sipsub_notify_h *notifyh, sipsub_close_h *closeh,
			void *arg, const char *fmt, ...);

int sipevent_refer(struct sipsub **subp, struct sipevent_sock *sock,
		   const char *uri, const char *from_name,
		   const char *from_uri, const char *cuser,
		   const char *routev[], uint32_t routec,
		   sip_auth_h *authh, void *aarg, bool aref,
		   sipsub_fork_h *forkh, sipsub_notify_h *notifyh,
		   sipsub_close_h *closeh, void *arg,
		   const char *fmt, ...);
int sipevent_drefer(struct sipsub **subp, struct sipevent_sock *sock,
		    struct sip_dialog *dlg, const char *cuser,
		    sip_auth_h *authh, void *aarg, bool aref,
		    sipsub_notify_h *notifyh, sipsub_close_h *closeh,
		    void *arg, const char *fmt, ...);

int sipevent_fork(struct sipsub **subp, struct sipsub *osub,
		  const struct sip_msg *msg,
		  sip_auth_h *authh, void *aarg, bool aref,
		  sipsub_notify_h *notifyh, sipsub_close_h *closeh,
		  void *arg);
