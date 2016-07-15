/**
 * @file re_sipsess.h  SIP Session
 *
 * Copyright (C) 2010 Creytiv.com
 */

struct sipsess_sock;
struct sipsess;


typedef void (sipsess_conn_h)(const struct sip_msg *msg, void *arg);
typedef int  (sipsess_offer_h)(struct mbuf **descp, const struct sip_msg *msg,
			       void *arg);
typedef int  (sipsess_answer_h)(const struct sip_msg *msg, void *arg);
typedef void (sipsess_progr_h)(const struct sip_msg *msg, void *arg);
typedef void (sipsess_estab_h)(const struct sip_msg *msg, void *arg);
typedef void (sipsess_info_h)(struct sip *sip, const struct sip_msg *msg,
			      void *arg);
typedef void (sipsess_refer_h)(struct sip *sip, const struct sip_msg *msg,
			       void *arg);
typedef void (sipsess_close_h)(int err, const struct sip_msg *msg, void *arg);


int  sipsess_listen(struct sipsess_sock **sockp, struct sip *sip,
		    int htsize, sipsess_conn_h *connh, void *arg);

int  sipsess_connect(struct sipsess **sessp, struct sipsess_sock *sock,
		     const char *to_uri, const char *from_name,
		     const char *from_uri, const char *cuser,
		     const char *routev[], uint32_t routec,
		     const char *ctype, struct mbuf *desc,
		     sip_auth_h *authh, void *aarg, bool aref,
		     sipsess_offer_h *offerh, sipsess_answer_h *answerh,
		     sipsess_progr_h *progrh, sipsess_estab_h *estabh,
		     sipsess_info_h *infoh, sipsess_refer_h *referh,
		     sipsess_close_h *closeh, void *arg, const char *fmt, ...);

int  sipsess_accept(struct sipsess **sessp, struct sipsess_sock *sock,
		    const struct sip_msg *msg, uint16_t scode,
		    const char *reason, const char *cuser, const char *ctype,
		    struct mbuf *desc,
		    sip_auth_h *authh, void *aarg, bool aref,
		    sipsess_offer_h *offerh, sipsess_answer_h *answerh,
		    sipsess_estab_h *estabh, sipsess_info_h *infoh,
		    sipsess_refer_h *referh, sipsess_close_h *closeh,
		    void *arg, const char *fmt, ...);

int  sipsess_progress(struct sipsess *sess, uint16_t scode,
		      const char *reason, struct mbuf *desc,
		      const char *fmt, ...);
int  sipsess_answer(struct sipsess *sess, uint16_t scode, const char *reason,
		    struct mbuf *desc, const char *fmt, ...);
int  sipsess_reject(struct sipsess *sess, uint16_t scode, const char *reason,
		    const char *fmt, ...);
int  sipsess_modify(struct sipsess *sess, struct mbuf *desc);
int  sipsess_info(struct sipsess *sess, const char *ctype, struct mbuf *body,
		  sip_resp_h *resph, void *arg);
int  sipsess_set_close_headers(struct sipsess *sess, const char *hdrs, ...);
void sipsess_close_all(struct sipsess_sock *sock);
struct sip_dialog *sipsess_dialog(const struct sipsess *sess);
