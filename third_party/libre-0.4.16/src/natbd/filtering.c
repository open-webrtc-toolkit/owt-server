/**
 * @file filtering.c  NAT Filtering Behaviour Discovery
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_stun.h>
#include <re_natbd.h>


#define DEBUG_MODULE "natbd_filtering"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/* Determining NAT Filtering Behavior

   This will also require at most three tests.  These tests should be
   performed using a port that wasn't used for mapping or other tests as
   packets sent during those tests may affect results.  In test I, the
   client performs the UDP connectivity test.  The server will return
   its alternate address and port in OTHER-ADDRESS in the binding
   response.  If OTHER-ADDRESS is not returned, the server does not
   support this usage and this test cannot be run.

   In test II, the client sends a binding request to the primary address
   of the server with the CHANGE-REQUEST attribute set to change-port
   and change-IP.  This will cause the server to send its response from
   its alternate IP address and alternate port.  If the client receives
   a response the current behaviour of the NAT is Endpoint-Independent
   Filtering.

   If no response is received, test III must be performed to distinguish
   between Address-Dependent Filtering and Address and Port-Dependent
   Filtering.  In test III, the client sends a binding request to the
   original server address with CHANGE-REQUEST set to change-port.  If
   the client receives a response the current behaviour is Address-
   Dependent Filtering; if no response is received the current behaviour
   is Address and Port-Dependent Filtering.
 */


/** Defines a NAT Filtering Behaviour Discovery session */
struct nat_filtering {
	struct stun *stun;     /**< STUN instance              */
	struct sa srv;         /**< Server IP address and port */
	int test_phase;        /**< State machine              */
	nat_filtering_h *fh;   /**< Result handler             */
	void *arg;             /**< Handler argument           */
};


static void stun_response_handler(int err, uint16_t scode, const char *reason,
				  const struct stun_msg *msg, void *arg)
{
	struct stun_change_req change_req;
	struct nat_filtering *nf = arg;
	struct stun_attr *attr;
	(void)reason;

	if (err == ECONNABORTED) {
		nf->fh(err, NAT_TYPE_UNKNOWN, nf->arg);
		return;
	}

	attr = stun_msg_attr(msg, STUN_ATTR_OTHER_ADDR);
	if (!err && !attr) {
		DEBUG_WARNING("no OTHER-ADDRESS in response - abort\n");
		nf->fh(EINVAL, NAT_TYPE_UNKNOWN, nf->arg);
		return;
	}

	switch (nf->test_phase) {

	case 1:
		/* Test I completed */

		if (err || scode) {
			DEBUG_WARNING("Test I: stun_response_handler: %m\n",
				      err);
			nf->fh(err, NAT_TYPE_UNKNOWN, nf->arg);
			return;
		}

		/* Start Test II */

		/*
		  In test II, the client sends a binding request to the
		  primary address of the server with the CHANGE-REQUEST
		  attribute set to change-port and change-IP.
		*/

		++nf->test_phase;

		change_req.ip = true;
		change_req.port = true;

		err = stun_request(NULL, nf->stun, IPPROTO_UDP, NULL, &nf->srv,
				   0, STUN_METHOD_BINDING, NULL, 0, false,
				   stun_response_handler, nf, 2,
				   STUN_ATTR_SOFTWARE, stun_software,
				   STUN_ATTR_CHANGE_REQ, &change_req);
		if (err) {
			DEBUG_WARNING("stunc_request_send: (%m)\n", err);
			nf->fh(err, NAT_TYPE_UNKNOWN, nf->arg);
		}
		break;

	case 2:
		/* Test II completed */

		/*
		  If the client receives a response the current behaviour of
		  the NAT is Endpoint Independent Filtering.
		*/

		if (0 == err) {
			if (!scode) {
				nf->fh(0, NAT_TYPE_ENDP_INDEP, nf->arg);
			}
			else {
				nf->fh(EINVAL, NAT_TYPE_UNKNOWN, nf->arg);
			}
			return;
		}

		/* Start Test III - the client sends a binding request to the
		   original server address with CHANGE-REQUEST set to
		   change-port
		 */
		++nf->test_phase;

		change_req.ip = false;
		change_req.port = true;

		err = stun_request(NULL, nf->stun, IPPROTO_UDP, NULL, &nf->srv,
				   0, STUN_METHOD_BINDING, NULL, 0, false,
				   stun_response_handler, nf, 2,
				   STUN_ATTR_SOFTWARE, stun_software,
				   STUN_ATTR_CHANGE_REQ, &change_req);
		if (err) {
			DEBUG_WARNING("stunc_request_send: (%m)\n", err);
			nf->fh(err, NAT_TYPE_UNKNOWN, nf->arg);
		}
		break;

	case 3:
		/* Test III completed */
		DEBUG_INFO("Test III completed\n");

		if (0 == err && !scode) {
			nf->fh(0, NAT_TYPE_ADDR_DEP, nf->arg);
		}
		else {
			nf->fh(0, NAT_TYPE_ADDR_PORT_DEP, nf->arg);
		}
		break;

	default:
		DEBUG_WARNING("invalid test phase %d\n", nf->test_phase);
		nf->fh(EINVAL, NAT_TYPE_UNKNOWN, nf->arg);
		return;
	}
}


static void filtering_destructor(void *data)
{
	struct nat_filtering *nf = data;

	mem_deref(nf->stun);
}


/**
 * Allocate a NAT Filtering Behaviour Discovery session
 *
 * @param nfp      Pointer to allocated NAT filtering object
 * @param srv      STUN Server IP address and port number
 * @param conf     STUN configuration (Optional)
 * @param fh       Filtering result handler
 * @param arg      Handler argument
 *
 * @return 0 if success, errorcode if failure
 */
int nat_filtering_alloc(struct nat_filtering **nfp, const struct sa *srv,
			const struct stun_conf *conf,
			nat_filtering_h *fh, void *arg)
{
	struct nat_filtering *nf;
	int err;

	if (!nfp || !srv || !fh)
		return EINVAL;

	nf = mem_zalloc(sizeof(*nf), filtering_destructor);
	if (!nf)
		return ENOMEM;

	err = stun_alloc(&nf->stun, conf, NULL, NULL);
	if (err)
		goto out;

	sa_cpy(&nf->srv, srv);

	nf->fh  = fh;
	nf->arg = arg;

 out:
	if (err)
		mem_deref(nf);
	else
		*nfp = nf;

	return err;
}


/**
 * Start a NAT Filtering Behaviour Discovery session
 *
 * @param nf NAT filtering object
 *
 * @return 0 if success, errorcode if failure
 */
int nat_filtering_start(struct nat_filtering *nf)
{
	if (!nf)
		return EINVAL;

	nf->test_phase = 1;

	return stun_request(NULL, nf->stun, IPPROTO_UDP, NULL, &nf->srv, 0,
			    STUN_METHOD_BINDING, NULL, 0, false,
			    stun_response_handler, nf, 1,
			    STUN_ATTR_SOFTWARE, stun_software);
}
