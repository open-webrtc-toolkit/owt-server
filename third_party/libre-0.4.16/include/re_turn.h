/**
 * @file re_turn.h  Interface to TURN implementation
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** TURN Protocol values */
enum {
	TURN_DEFAULT_LIFETIME = 600,  /**< Default lifetime is 10 minutes */
	TURN_MAX_LIFETIME     = 3600  /**< Maximum lifetime is 1 hour     */
};

typedef void(turnc_h)(int err, uint16_t scode, const char *reason,
		      const struct sa *relay_addr,
		      const struct sa *mapped_addr,
		      const struct stun_msg *msg,
		      void *arg);
typedef void(turnc_perm_h)(void *arg);
typedef void(turnc_chan_h)(void *arg);

struct turnc;

int turnc_alloc(struct turnc **turncp, const struct stun_conf *conf, int proto,
		void *sock, int layer, const struct sa *srv,
		const char *username, const char *password,
		uint32_t lifetime, turnc_h *th, void *arg);
int turnc_send(struct turnc *turnc, const struct sa *dst, struct mbuf *mb);
int turnc_recv(struct turnc *turnc, struct sa *src, struct mbuf *mb);
int turnc_add_perm(struct turnc *turnc, const struct sa *peer,
		   turnc_perm_h *ph, void *arg);
int turnc_add_chan(struct turnc *turnc, const struct sa *peer,
		   turnc_chan_h *ch, void *arg);
