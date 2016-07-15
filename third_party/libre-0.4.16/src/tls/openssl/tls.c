/**
 * @file openssl/tls.c TLS backend using OpenSSL
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#define OPENSSL_NO_KRB5 1
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_main.h>
#include <re_sa.h>
#include <re_net.h>
#include <re_srtp.h>
#include <re_sys.h>
#include <re_tcp.h>
#include <re_tls.h>
#include "tls.h"


#define DEBUG_MODULE "tls"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/* NOTE: shadow struct defined in tls_*.c */
struct tls_conn {
	SSL *ssl;
};


static void destructor(void *data)
{
	struct tls *tls = data;

	if (tls->ctx)
		SSL_CTX_free(tls->ctx);

	if (tls->cert)
		X509_free(tls->cert);

	mem_deref(tls->pass);
}


/*The password code is not thread safe*/
static int password_cb(char *buf, int size, int rwflag, void *userdata)
{
	struct tls *tls = userdata;

	(void)rwflag;

	DEBUG_NOTICE("password callback\n");

	if (size < (int)strlen(tls->pass)+1)
		return 0;

	strncpy(buf, tls->pass, size);

	return (int)strlen(tls->pass);
}


/**
 * Allocate a new TLS context
 *
 * @param tlsp    Pointer to allocated TLS context
 * @param method  TLS method
 * @param keyfile Optional private key file
 * @param pwd     Optional password
 *
 * @return 0 if success, otherwise errorcode
 */
int tls_alloc(struct tls **tlsp, enum tls_method method, const char *keyfile,
	      const char *pwd)
{
	struct tls *tls;
	int r, err;

	if (!tlsp)
		return EINVAL;

	tls = mem_zalloc(sizeof(*tls), destructor);
	if (!tls)
		return ENOMEM;

	switch (method) {

	case TLS_METHOD_SSLV23:
		tls->ctx = SSL_CTX_new(SSLv23_method());
		break;

#ifdef USE_OPENSSL_DTLS
	case TLS_METHOD_DTLSV1:
		tls->ctx = SSL_CTX_new(DTLSv1_method());
		break;

#ifdef SSL_OP_NO_DTLSv1_2
		/* DTLS v1.2 is available in OpenSSL 1.0.2 and later */

	case TLS_METHOD_DTLS:
		tls->ctx = SSL_CTX_new(DTLS_method());
		break;

	case TLS_METHOD_DTLSV1_2:
		tls->ctx = SSL_CTX_new(DTLSv1_2_method());
		break;
#endif

#endif

	default:
		DEBUG_WARNING("tls method %d not supported\n", method);
		err = ENOSYS;
		goto out;
	}

	if (!tls->ctx) {
		ERR_clear_error();
		err = ENOMEM;
		goto out;
	}

#if (OPENSSL_VERSION_NUMBER < 0x00905100L)
	SSL_CTX_set_verify_depth(tls->ctx, 1);
#endif

	/* Load our keys and certificates */
	if (keyfile) {
		if (pwd) {
			err = str_dup(&tls->pass, pwd);
			if (err)
				goto out;

			SSL_CTX_set_default_passwd_cb(tls->ctx, password_cb);
			SSL_CTX_set_default_passwd_cb_userdata(tls->ctx, tls);
		}

		r = SSL_CTX_use_certificate_chain_file(tls->ctx, keyfile);
		if (r <= 0) {
			DEBUG_WARNING("Can't read certificate file: %s (%d)\n",
				      keyfile, r);
			ERR_clear_error();
			err = EINVAL;
			goto out;
		}

		r = SSL_CTX_use_PrivateKey_file(tls->ctx, keyfile,
						SSL_FILETYPE_PEM);
		if (r <= 0) {
			DEBUG_WARNING("Can't read key file: %s (%d)\n",
				      keyfile, r);
			ERR_clear_error();
			err = EINVAL;
			goto out;
		}
	}

	err = 0;
 out:
	if (err)
		mem_deref(tls);
	else
		*tlsp = tls;

	return err;
}


/**
 * Set default locations for trusted CA certificates
 *
 * @param tls    TLS Context
 * @param capath Path to CA certificates
 *
 * @return 0 if success, otherwise errorcode
 */
int tls_add_ca(struct tls *tls, const char *capath)
{
	if (!tls || !capath)
		return EINVAL;

	/* Load the CAs we trust */
	if (!(SSL_CTX_load_verify_locations(tls->ctx, capath, 0))) {
		DEBUG_WARNING("Can't read CA list: %s\n", capath);
		ERR_clear_error();
		return EINVAL;
	}

	return 0;
}


/**
 * Generate and set selfsigned certificate on TLS context
 *
 * @param tls TLS Context
 * @param cn  Common Name
 *
 * @return 0 if success, otherwise errorcode
 */
int tls_set_selfsigned(struct tls *tls, const char *cn)
{
	X509_NAME *subj = NULL;
	EVP_PKEY *key = NULL;
	X509 *cert = NULL;
	BIGNUM *bn = NULL;
	RSA *rsa = NULL;
	int r, err = ENOMEM;

	if (!tls || !cn)
		return EINVAL;

	rsa = RSA_new();
	if (!rsa)
		goto out;

	bn = BN_new();
	if (!bn)
		goto out;

	BN_set_word(bn, RSA_F4);
	if (!RSA_generate_key_ex(rsa, 1024, bn, NULL))
		goto out;

	key = EVP_PKEY_new();
	if (!key)
		goto out;

	if (!EVP_PKEY_set1_RSA(key, rsa))
		goto out;

	cert = X509_new();
	if (!cert)
		goto out;

	if (!X509_set_version(cert, 2))
		goto out;

	if (!ASN1_INTEGER_set(X509_get_serialNumber(cert), rand_u32()))
		goto out;

	subj = X509_NAME_new();
	if (!subj)
		goto out;

	if (!X509_NAME_add_entry_by_txt(subj, "CN", MBSTRING_ASC,
					(unsigned char *)cn,
					(int)strlen(cn), -1, 0))
		goto out;

	if (!X509_set_issuer_name(cert, subj) ||
	    !X509_set_subject_name(cert, subj))
		goto out;

	if (!X509_gmtime_adj(X509_get_notBefore(cert), -3600*24*365) ||
	    !X509_gmtime_adj(X509_get_notAfter(cert),   3600*24*365*10))
		goto out;

	if (!X509_set_pubkey(cert, key))
		goto out;

	if (!X509_sign(cert, key, EVP_sha1()))
		goto out;

	r = SSL_CTX_use_certificate(tls->ctx, cert);
	if (r != 1)
		goto out;

	r = SSL_CTX_use_PrivateKey(tls->ctx, key);
	if (r != 1)
		goto out;

	if (tls->cert)
		X509_free(tls->cert);

	tls->cert = cert;
	cert = NULL;

	err = 0;

 out:
	if (subj)
		X509_NAME_free(subj);

	if (cert)
		X509_free(cert);

	if (key)
		EVP_PKEY_free(key);

	if (rsa)
		RSA_free(rsa);

	if (bn)
		BN_free(bn);

	if (err)
		ERR_clear_error();

	return err;
}


/**
 * Set the certificate and private key on a TLS context
 *
 * @param tls TLS Context
 * @param pem Certificate and private key in PEM format
 * @param len Length of PEM string
 *
 * @return 0 if success, otherwise errorcode
 */
int tls_set_certificate(struct tls *tls, const char *pem, size_t len)
{
	BIO *bio = NULL, *kbio = NULL;
	X509 *cert = NULL;
	RSA *rsa = NULL;
	int r, err = ENOMEM;

	if (!tls || !pem || !len)
		return EINVAL;

	bio  = BIO_new_mem_buf((char *)pem, (int)len);
	kbio = BIO_new_mem_buf((char *)pem, (int)len);
	if (!bio || !kbio)
		goto out;

	cert = PEM_read_bio_X509(bio, NULL, 0, NULL);
	rsa = PEM_read_bio_RSAPrivateKey(kbio, NULL, 0, NULL);
	if (!cert || !rsa)
		goto out;

	r = SSL_CTX_use_certificate(tls->ctx, cert);
	if (r != 1)
		goto out;

	r = SSL_CTX_use_RSAPrivateKey(tls->ctx, rsa);
	if (r != 1) {
		DEBUG_WARNING("set_certificate: use_RSAPrivateKey failed\n");
		goto out;
	}

	if (tls->cert)
		X509_free(tls->cert);

	tls->cert = cert;
	cert = NULL;

	err = 0;

 out:
	if (cert)
		X509_free(cert);
	if (rsa)
		RSA_free(rsa);
	if (bio)
		BIO_free(bio);
	if (kbio)
		BIO_free(kbio);
	if (err)
		ERR_clear_error();

	return err;
}


static int verify_handler(int ok, X509_STORE_CTX *ctx)
{
	(void)ok;
	(void)ctx;

	return 1;    /* We trust the certificate from peer */
}


/**
 * Set TLS server context to request certificate from client
 *
 * @param tls    TLS Context
 */
void tls_set_verify_client(struct tls *tls)
{
	if (!tls)
		return;

	SSL_CTX_set_verify_depth(tls->ctx, 0);
	SSL_CTX_set_verify(tls->ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE,
			   verify_handler);
}


/**
 * Set SRTP suites on TLS context
 *
 * @param tls    TLS Context
 * @param suites Secure-RTP Profiles
 *
 * @return 0 if success, otherwise errorcode
 */
int tls_set_srtp(struct tls *tls, const char *suites)
{
#ifdef USE_OPENSSL_SRTP
	if (!tls || !suites)
		return EINVAL;

	if (0 != SSL_CTX_set_tlsext_use_srtp(tls->ctx, suites)) {
		ERR_clear_error();
		return ENOSYS;
	}

	return 0;
#else
	(void)tls;
	(void)suites;

	return ENOSYS;
#endif
}


static int cert_fingerprint(X509 *cert, enum tls_fingerprint type,
			    uint8_t *md, size_t size)
{
	unsigned int len = (unsigned int)size;
	int n;

	switch (type) {

	case TLS_FINGERPRINT_SHA1:
		if (size < 20)
			return EOVERFLOW;

		n = X509_digest(cert, EVP_sha1(), md, &len);
		break;

	case TLS_FINGERPRINT_SHA256:
		if (size < 32)
			return EOVERFLOW;

		n = X509_digest(cert, EVP_sha256(), md, &len);
		break;

	default:
		return ENOSYS;
	}

	if (n != 1) {
		ERR_clear_error();
		return ENOENT;
	}

	return 0;
}


/**
 * Get fingerprint of local certificate
 *
 * @param tls  TLS Context
 * @param type Digest type
 * @param md   Buffer for fingerprint digest
 * @param size Buffer size
 *
 * @return 0 if success, otherwise errorcode
 */
int tls_fingerprint(const struct tls *tls, enum tls_fingerprint type,
		    uint8_t *md, size_t size)
{
	if (!tls || !tls->cert || !md)
		return EINVAL;

	return cert_fingerprint(tls->cert, type, md, size);
}


/**
 * Get fingerprint of peer certificate of a TLS connection
 *
 * @param tc   TLS Connection
 * @param type Digest type
 * @param md   Buffer for fingerprint digest
 * @param size Buffer size
 *
 * @return 0 if success, otherwise errorcode
 */
int tls_peer_fingerprint(const struct tls_conn *tc, enum tls_fingerprint type,
			 uint8_t *md, size_t size)
{
	X509 *cert;
	int err;

	if (!tc || !md)
		return EINVAL;

	cert = SSL_get_peer_certificate(tc->ssl);
	if (!cert)
		return ENOENT;

	err = cert_fingerprint(cert, type, md, size);

	X509_free(cert);

	return err;
}


/**
 * Get common name of peer certificate of a TLS connection
 *
 * @param tc   TLS Connection
 * @param cn   Returned common name
 * @param size Size of common name
 *
 * @return 0 if success, otherwise errorcode
 */
int tls_peer_common_name(const struct tls_conn *tc, char *cn, size_t size)
{
	X509 *cert;
	int n;

	if (!tc || !cn || !size)
		return EINVAL;

	cert = SSL_get_peer_certificate(tc->ssl);
	if (!cert)
		return ENOENT;

	n = X509_NAME_get_text_by_NID(X509_get_subject_name(cert),
				      NID_commonName, cn, (int)size);

	X509_free(cert);

	if (n < 0) {
		ERR_clear_error();
		return ENOENT;
	}

	return 0;
}


/**
 * Verify peer certificate of a TLS connection
 *
 * @param tc TLS Connection
 *
 * @return 0 if verified, otherwise errorcode
 */
int tls_peer_verify(const struct tls_conn *tc)
{
	if (!tc)
		return EINVAL;

	if (SSL_get_verify_result(tc->ssl) != X509_V_OK)
		return EAUTH;

	return 0;
}


/**
 * Get SRTP suite and keying material of a TLS connection
 *
 * @param tc           TLS Connection
 * @param suite        Returned SRTP suite
 * @param cli_key      Client key
 * @param cli_key_size Client key size
 * @param srv_key      Server key
 * @param srv_key_size Server key size
 *
 * @return 0 if success, otherwise errorcode
 */
int tls_srtp_keyinfo(const struct tls_conn *tc, enum srtp_suite *suite,
		     uint8_t *cli_key, size_t cli_key_size,
		     uint8_t *srv_key, size_t srv_key_size)
{
#ifdef USE_OPENSSL_SRTP
	static const char *label = "EXTRACTOR-dtls_srtp";
	size_t key_size, salt_size, size;
	SRTP_PROTECTION_PROFILE *sel;
	uint8_t keymat[256], *p;

	if (!tc || !suite || !cli_key || !srv_key)
		return EINVAL;

	sel = SSL_get_selected_srtp_profile(tc->ssl);
	if (!sel)
		return ENOENT;

	switch (sel->id) {

	case SRTP_AES128_CM_SHA1_80:
		*suite = SRTP_AES_CM_128_HMAC_SHA1_80;
		key_size  = 16;
		salt_size = 14;
		break;

	case SRTP_AES128_CM_SHA1_32:
		*suite = SRTP_AES_CM_128_HMAC_SHA1_32;
		key_size  = 16;
		salt_size = 14;
		break;

	default:
		return ENOSYS;
	}

	size = key_size + salt_size;

	if (cli_key_size < size || srv_key_size < size)
		return EOVERFLOW;

	if (sizeof(keymat) < 2*size)
		return EOVERFLOW;

	if (1 != SSL_export_keying_material(tc->ssl, keymat, 2*size, label,
					    strlen(label), NULL, 0, 0)) {
		ERR_clear_error();
		return ENOENT;
	}

	p = keymat;

	memcpy(cli_key,            p, key_size);  p += key_size;
	memcpy(srv_key,            p, key_size);  p += key_size;
	memcpy(cli_key + key_size, p, salt_size); p += salt_size;
	memcpy(srv_key + key_size, p, salt_size);

	return 0;
#else
	(void)tc;
	(void)suite;
	(void)cli_key;
	(void)cli_key_size;
	(void)srv_key;
	(void)srv_key_size;

	return ENOSYS;
#endif
}


/**
 * Get cipher name of a TLS connection
 *
 * @param tc TLS Connection
 *
 * @return name of cipher actually used.
 */
const char *tls_cipher_name(const struct tls_conn *tc)
{
	if (!tc)
		return NULL;

	return SSL_get_cipher_name(tc->ssl);
}
