/**
 * @file sipsess.h  SIP Session Private Interface
 *
 * Copyright (C) 2010 Creytiv.com
 */


struct sipsess {
	struct le he;
	struct tmr tmr;
	struct list replyl;
	struct list requestl;
	struct sip_loopstate ls;
	struct sipsess_sock *sock;
	const struct sip_msg *msg;
	struct sip_request *req;
	struct sip_dialog *dlg;
	struct sip_strans *st;
	struct sip_auth *auth;
	struct sip *sip;
	char *cuser;
	char *ctype;
	char *close_hdrs;
	struct mbuf *hdrs;
	struct mbuf *desc;
	sipsess_offer_h *offerh;
	sipsess_answer_h *answerh;
	sipsess_progr_h *progrh;
	sipsess_estab_h *estabh;
	sipsess_info_h *infoh;
	sipsess_refer_h *referh;
	sipsess_close_h *closeh;
	void *arg;
	bool owner;
	bool sent_offer;
	bool awaiting_answer;
	bool modify_pending;
	bool established;
	bool peerterm;
	int terminated;
};


struct sipsess_sock {
	struct sip_lsnr *lsnr_resp;
	struct sip_lsnr *lsnr_req;
	struct hash *ht_sess;
	struct hash *ht_ack;
	struct sip *sip;
	sipsess_conn_h *connh;
	void *arg;
};


struct sipsess_request {
	struct le le;
	struct sip_loopstate ls;
	struct sipsess *sess;
	struct sip_request *req;
	char *ctype;
	struct mbuf *body;
	sip_resp_h *resph;
	void *arg;
};


int  sipsess_alloc(struct sipsess **sessp, struct sipsess_sock *sock,
		   const char *cuser, const char *ctype, struct mbuf *desc,
		   sip_auth_h *authh, void *aarg, bool aref,
		   sipsess_offer_h *offerh, sipsess_answer_h *answerh,
		   sipsess_progr_h *progrh, sipsess_estab_h *estabh,
		   sipsess_info_h *infoh, sipsess_refer_h *referh,
		   sipsess_close_h *closeh, void *arg);
void sipsess_terminate(struct sipsess *sess, int err,
		       const struct sip_msg *msg);
int  sipsess_ack(struct sipsess_sock *sock, struct sip_dialog *dlg,
		uint32_t cseq, struct sip_auth *auth,
		const char *ctype, struct mbuf *desc);
int  sipsess_ack_again(struct sipsess_sock *sock, const struct sip_msg *msg);
int  sipsess_reply_2xx(struct sipsess *sess, const struct sip_msg *msg,
		       uint16_t scode, const char *reason, struct mbuf *desc,
		       const char *fmt, va_list *ap);
int  sipsess_reply_ack(struct sipsess *sess, const struct sip_msg *msg,
		       bool *awaiting_answer);
int  sipsess_reinvite(struct sipsess *sess, bool reset_ls);
int  sipsess_bye(struct sipsess *sess, bool reset_ls);
int  sipsess_request_alloc(struct sipsess_request **reqp, struct sipsess *sess,
			   const char *ctype, struct mbuf *body,
			   sip_resp_h *resph, void *arg);
