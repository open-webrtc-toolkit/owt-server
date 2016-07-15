/**
 * @file bfcp.h Internal interface to Binary Floor Control Protocol (BFCP)
 *
 * Copyright (C) 2010 Creytiv.com
 */

struct bfcp_strans {
	enum bfcp_prim prim;
	uint32_t confid;
	uint16_t tid;
	uint16_t userid;
};

struct bfcp_conn {
	struct bfcp_strans st;
	struct list ctransl;
	struct tmr tmr1;
	struct tmr tmr2;
	struct udp_sock *us;
	struct mbuf *mb;
	bfcp_recv_h *recvh;
	void *arg;
	enum bfcp_transp tp;
	unsigned txc;
	uint16_t tid;
};


/* attributes */
int bfcp_attrs_decode(struct list *attrl, struct mbuf *mb, size_t len,
		      struct bfcp_unknown_attr *uma);
struct bfcp_attr *bfcp_attrs_find(const struct list *attrl,
				  enum bfcp_attrib type);
struct bfcp_attr *bfcp_attrs_apply(const struct list *attrl,
				   bfcp_attr_h *h, void *arg);
int bfcp_attrs_print(struct re_printf *pf, const struct list *attrl,
		     unsigned level);


/* connection */
int bfcp_send(struct bfcp_conn *bc, const struct sa *dst, struct mbuf *mb);


/* request */
bool bfcp_handle_response(struct bfcp_conn *bc, const struct bfcp_msg *msg);
int  bfcp_vrequest(struct bfcp_conn *bc, const struct sa *dst, uint8_t ver,
		   enum bfcp_prim prim, uint32_t confid, uint16_t userid,
		   bfcp_resp_h *resph, void *arg, unsigned attrc, va_list *ap);
