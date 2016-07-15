/**
 * @file re_sha.h  Interface to SHA (Secure Hash Standard) functions
 *
 * Copyright (C) 2010 Creytiv.com
 */


#ifdef USE_OPENSSL
#include <openssl/sha.h>
#else

/* public api for steve reid's public domain SHA-1 implementation */
/* this file is in the public domain */

/** SHA-1 Context */
typedef struct {
	uint32_t state[5];   /**< Context state */
	uint32_t count[2];   /**< Counter       */
	uint8_t  buffer[64]; /**< SHA-1 buffer  */
} SHA1_CTX;

/** SHA-1 Context (OpenSSL compat) */
typedef SHA1_CTX SHA_CTX;

/** SHA-1 Digest size in bytes */
#define SHA1_DIGEST_SIZE 20
/** SHA-1 Digest size in bytes (OpenSSL compat) */
#define SHA_DIGEST_LENGTH SHA1_DIGEST_SIZE

void SHA1_Init(SHA1_CTX* context);
void SHA1_Update(SHA1_CTX* context, const void *p, size_t len);
void SHA1_Final(uint8_t digest[SHA1_DIGEST_SIZE], SHA1_CTX* context);

#endif
