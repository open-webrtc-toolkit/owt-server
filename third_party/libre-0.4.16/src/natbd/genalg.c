/**
 * @file genalg.c  Detecting Generic ALGs
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mbuf.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_stun.h>
#include <re_natbd.h>


#define DEBUG_MODULE "natbd_genalg"
#define DEBUG_LEVEL 7
#include <re_dbg.h>


/*
  Detecting Generic ALGs

   A number of NAT boxes are now being deployed into the market which
   try to provide "generic" ALG functionality.  These generic ALGs hunt
   for IP addresses, either in text or binary form within a packet, and
   rewrite them if they match a binding.  This behavior can be detected
   because the STUN server returns both the MAPPED-ADDRESS and XOR-
   MAPPED-ADDRESS in the same response.  If the result in the two does
   not match, there a NAT with a generic ALG in the path.
 */


/** Defines a NAT Generic ALG detection session */
struct nat_genalg {
	struct stun *stun;      /**< STUN Client               */
	struct sa srv;          /**< Server address and port   */
	int proto;              /**< IP protocol               */
	nat_genalg_h *h;        /**< Result handler            */
	void *arg;              /**< Handler argument          */
};


static void stun_response_handler(int err, uint16_t scode, const char *reason,
				  const struct stun_msg *msg, void *arg)
{
	struct stun_attr *xmap, *map;
	struct nat_genalg *ng = arg;
	int status = 0;
	(void)reason;

	if (err) {
		ng->h(err, 0, NULL, -1, NULL, ng->arg);
		return;
	}

	switch (scode) {

	case 0:
		map  = stun_msg_attr(msg, STUN_ATTR_MAPPED_ADDR);
		xmap = stun_msg_attr(msg, STUN_ATTR_XOR_MAPPED_ADDR);
		if (!map || !xmap) {
			ng->h(EINVAL, scode, reason, -1, NULL, ng->arg);
			break;
		}

		status = sa_cmp(&map->v.sa, &xmap->v.sa, SA_ALL) ? -1 : 1;

		ng->h(0, scode, reason, status, &xmap->v.sa, ng->arg);
		break;

	default:
		ng->h(0, scode, reason, -1, NULL, ng->arg);
		break;
	}
}


static void genalg_destructor(void *data)
{
	struct nat_genalg *ng = data;

	mem_deref(ng->stun);
}


/**
 * Allocate a new NAT Generic ALG detection session
 *
 * @param ngp       Pointer to allocated NAT Generic ALG object
 * @param srv       STUN Server IP address and port
 * @param proto     Transport protocol
 * @param conf      STUN configuration (Optional)
 * @param gh        Generic ALG handler
 * @param arg       Handler argument
 *
 * @return 0 if success, errorcode if failure
 */
int nat_genalg_alloc(struct nat_genalg **ngp, const struct sa *srv, int proto,
		     const struct stun_conf *conf,
		     nat_genalg_h *gh, void *arg)
{
	struct nat_genalg *ng;
	int err;

	if (!ngp || !srv || !proto || !gh)
		return EINVAL;

	ng = mem_zalloc(sizeof(*ng), genalg_destructor);
	if (!ng)
		return ENOMEM;

	err = stun_alloc(&ng->stun, conf, NULL, NULL);
	if (err)
		goto out;

	sa_cpy(&ng->srv, srv);
	ng->proto = proto;
	ng->h     = gh;
	ng->arg   = arg;

 out:
	if (err)
		mem_deref(ng);
	else
		*ngp = ng;

	return err;
}


/**
 * Start the NAT Generic ALG detection
 *
 * @param ng NAT Generic ALG object
 *
 * @return 0 if success, errorcode if failure
 */
int nat_genalg_start(struct nat_genalg *ng)
{
	int err;

	if (!ng)
		return EINVAL;

	err = stun_request(NULL, ng->stun, ng->proto, NULL, &ng->srv, 0,
			   STUN_METHOD_BINDING, NULL, 0, false,
			   stun_response_handler, ng, 1,
			   STUN_ATTR_SOFTWARE, stun_software);

	return err;
}
