#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cassert>
#include <iostream>
#include "OpenSSLInit.h"

#include <openssl/e_os2.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include <openssl/bn.h>
#include <openssl/srtp.h>
#include <openssl/pem.h>

#include "DtlsFactory.h"
#include "DtlsSocket.h"

using namespace dtls;
using namespace std;
const char* DtlsFactory::DefaultSrtpProfile = "SRTP_AES128_CM_SHA1_80";

X509 *DtlsFactory::mCert = NULL;
EVP_PKEY *DtlsFactory::privkey = NULL;

DEFINE_LOGGER(DtlsFactory, "dtls.DtlsFactory");
log4cxx::LoggerPtr sslLogger(log4cxx::Logger::getLogger("dtls.SSL"));

void
SSLInfoCallback(const SSL* s, int where, int ret) {
  const char* str = "undefined";
  int w = where & ~SSL_ST_MASK;
  if (w & SSL_ST_CONNECT) {
    str = "SSL_connect";
  } else if (w & SSL_ST_ACCEPT) {
    str = "SSL_accept";
  }
  if (where & SSL_CB_LOOP) {
    ELOG_DEBUG2(sslLogger, "%s", SSL_state_string_long(s));
  } else if (where & SSL_CB_ALERT) {
    str = (where & SSL_CB_READ) ? "read" : "write";
    ELOG_DEBUG2(sslLogger, "SSL3 alert %d - %s; %s : %s",
      ret,
      str,
      SSL_alert_type_string_long(ret),
      SSL_alert_desc_string_long(ret));
  } else if (where & SSL_CB_EXIT) {
    if (ret == 0) {
      ELOG_WARN2(sslLogger, "failed in %s", SSL_state_string_long(s));
    } else if (ret < 0) {
      ELOG_WARN2(sslLogger, "error in %s", SSL_state_string_long(s));
    }
  }
}

int SSLVerifyCallback(int ok, X509_STORE_CTX* store) {

  if (!ok) {
    char data[256], data2[256];
    X509* cert = X509_STORE_CTX_get_current_cert(store);
    int depth = X509_STORE_CTX_get_error_depth(store);
    int err = X509_STORE_CTX_get_error(store);
    X509_NAME_oneline(X509_get_issuer_name(cert), data, sizeof(data));
    X509_NAME_oneline(X509_get_subject_name(cert), data2, sizeof(data2));
    ELOG_DEBUG2(sslLogger, "Error with certificate at depth: %d, issuer: %s, subject: %s, err: %d : %s",
      depth,
      data,
      data2,
      err,
      X509_verify_cert_error_string(err));
  }

  // Get our SSL structure from the store
  //SSL* ssl = reinterpret_cast<SSL*>(X509_STORE_CTX_get_ex_data(
  //                                      store,
  //                                      SSL_get_ex_data_X509_STORE_CTX_idx()));


  // In peer-to-peer mode, no root cert / certificate authority was
  // specified, so the libraries knows of no certificate to accept,
  // and therefore it will necessarily call here on the first cert it
  // tries to verify.
  if (!ok) {
    //X509* cert = X509_STORE_CTX_get_current_cert(store);
    int err = X509_STORE_CTX_get_error(store);

    ELOG_DEBUG2(sslLogger, "Error: %d", X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT);

    // peer-to-peer mode: allow the certificate to be self-signed,
    // assuming it matches the digest that was specified.
    if (err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) {
      //unsigned char digest[EVP_MAX_MD_SIZE];
      //std::size_t digest_length;

      ELOG_DEBUG2(sslLogger, "Accepted self-signed peer certificate authority");
      ok = 1;

      /* TODO

      if (OpenSSLCertificate::
         ComputeDigest(cert,
                       stream->peer_certificate_digest_algorithm_,
                       digest, sizeof(digest),
                       &digest_length)) {
        Buffer computed_digest(digest, digest_length);
        if (computed_digest == stream->peer_certificate_digest_value_) {
          ELOG_DEBUG2(sslLogger, "Accepted self-signed peer certificate authority");
          ok = 1;
        }
      }
      */
    }
  }
  if (!ok) {
    ELOG_DEBUG2(sslLogger, "Ignoring cert error while verifying cert chain");
    ok = 1;
  }

  return ok;
}

static const int KEY_LENGTH = 1024;

int loadCert(const std::string& certFileName, const std::string& keyFileName, const std::string& privatePasswd, X509*& cert, EVP_PKEY*& key) {
  FILE* certFile = NULL;
  FILE* keyFile = NULL;
  keyFile = fopen(keyFileName.c_str(), "r");
  if (keyFile == NULL) {
    return 0;
  }
  certFile = fopen(certFileName.c_str(), "r");
  if (certFile == NULL) {
    fclose(keyFile);
    return 0;
  }
  key = PEM_read_PrivateKey(keyFile, NULL, NULL, (void*)privatePasswd.c_str());
  cert = PEM_read_X509(certFile, NULL, NULL, NULL);
  fclose(keyFile);
  fclose(certFile);
  ELOG_DEBUG2(sslLogger, "Static Key & Cert loaded!");
  return 1;
}

int saveCert(X509*& cert, EVP_PKEY*& key, const std::string& privatePasswd) {
  FILE* certFile = fopen("static_cert.pem", "w+");
  if (certFile == NULL) {
    return 0;
  }
  if(!PEM_write_X509(certFile, cert)) {
    ELOG_DEBUG2(sslLogger, "Write to cert file failed!");
    fclose(certFile);
    return 0;
  }
  fclose(certFile);
  FILE* keyFile = fopen("static_key.pem", "w+");
  if (keyFile == NULL) {
    return 0;
  }
  if (!PEM_write_PrivateKey(keyFile, key, EVP_aes_256_cbc(), (unsigned char*)privatePasswd.c_str(), privatePasswd.length(), NULL, NULL)) {
    ELOG_DEBUG2(sslLogger, "Write to key file failed!");
    fclose(keyFile);
    return 0;
  }
  fclose(keyFile);
  return 1;
}

int createCert(const string& pAor, int expireDays, int keyLen, X509*& outCert, EVP_PKEY*& outKey, const std::string& privatePasswd)
{
   std::ostringstream info;
   info << "Generating new user cert for" << pAor;
   ELOG_DEBUG2(sslLogger, "%s", info.str().c_str());
   string aor = "sip:" + pAor;

   // Make sure that necessary algorithms exist:
   assert(EVP_sha1());

   EVP_PKEY* privkey = EVP_PKEY_new();
   assert(privkey);

   RSA* rsa = RSA_new();

   BIGNUM* exponent = BN_new();
   BN_set_word(exponent, 0x10001);

   RSA_generate_key_ex(rsa, KEY_LENGTH, exponent, NULL);

   //RSA* rsa = RSA_generate_key(keyLen, RSA_F4, NULL, NULL);
   assert(rsa);    // couldn't make key pair

   int ret = EVP_PKEY_set1_RSA(privkey, rsa);
   assert(ret);

   X509* cert = X509_new();
   assert(cert);

   X509_NAME* subject = X509_NAME_new();
   X509_EXTENSION* ext = X509_EXTENSION_new();

   // set version to X509v3 (starts from 0)
   //X509_set_version(cert, 0L);

   int serial = rand();  // get an int worth of randomness
   assert(sizeof(int)==4);
   ASN1_INTEGER_set(X509_get_serialNumber(cert),serial);

//    ret = X509_NAME_add_entry_by_txt( subject, "O",  MBSTRING_ASC,
//                                      (unsigned char *) domain.data(), domain.size(),
//                                      -1, 0);
   assert(ret);
   ret = X509_NAME_add_entry_by_txt( subject, "CN", MBSTRING_ASC,
                                     (unsigned char *) aor.data(), aor.size(),
                                     -1, 0);
   assert(ret);

   ret = X509_set_issuer_name(cert, subject);
   assert(ret);
   ret = X509_set_subject_name(cert, subject);
   assert(ret);

   const long duration = 60*60*24*expireDays;
   X509_gmtime_adj(X509_get_notBefore(cert),0);
   X509_gmtime_adj(X509_get_notAfter(cert), duration);

   ret = X509_set_pubkey(cert, privkey);
   assert(ret);

   string subjectAltNameStr = string("URI:sip:") + aor
      + string(",URI:im:")+aor
      + string(",URI:pres:")+aor;
   ext = X509V3_EXT_conf_nid( NULL , NULL , NID_subject_alt_name,
                              (char*) subjectAltNameStr.c_str() );
//   X509_add_ext( cert, ext, -1);
   X509_EXTENSION_free(ext);

   static char CA_FALSE[] = "CA:FALSE";
   ext = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, CA_FALSE);
   ret = X509_add_ext( cert, ext, -1);
   assert(ret);
   X509_EXTENSION_free(ext);

   // TODO add extensions NID_subject_key_identifier and NID_authority_key_identifier

   ret = X509_sign(cert, privkey, EVP_sha1());
   assert(ret);

   outCert = cert;
   outKey = privkey;
   if (!saveCert(cert, privkey, privatePasswd)) {
    ELOG_DEBUG2(sslLogger, "Error in saveCert()");
   }
   return ret;
}

void DtlsFactory::Init(const std::string& certFile, const std::string& keyFile, const std::string& privatePasswd) {
  if (DtlsFactory::mCert == NULL) {
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_crypto_strings();
    //  srtp_init();
    if (!loadCert(certFile, keyFile, privatePasswd, DtlsFactory::mCert, DtlsFactory::privkey)) {
      createCert("sip:webrtc@intel.com",365,1024,DtlsFactory::mCert,DtlsFactory::privkey,privatePasswd);
    }
  }

}

DtlsFactory::DtlsFactory(const std::string& certFile, const std::string& keyFile, const std::string& privatePasswd)
{
    DtlsFactory::Init(certFile, keyFile, privatePasswd);

    mTimerContext = std::auto_ptr<TestTimerContext>(new TestTimerContext());


    ELOG_DEBUG("Creating Dtls factory");

    mContext=SSL_CTX_new(DTLSv1_client_method());
    assert(mContext);

    int r = SSL_CTX_use_certificate(mContext, mCert);
    assert(r == 1);
    (void)r;  // Unused.

    r = SSL_CTX_use_PrivateKey(mContext, privkey);
    assert(r == 1);

    SSL_CTX_set_cipher_list(mContext, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

    SSL_CTX_set_info_callback(mContext, SSLInfoCallback);

    SSL_CTX_set_verify(mContext, SSL_VERIFY_PEER |SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                     SSLVerifyCallback);

    //SSL_CTX_set_session_cache_mode(mContext, SSL_SESS_CACHE_OFF);
    //SSL_CTX_set_options(mContext, SSL_OP_NO_TICKET);
    // Set SRTP profiles
    r=SSL_CTX_set_tlsext_use_srtp(mContext, DefaultSrtpProfile);
    assert(r==0);

    SSL_CTX_set_verify_depth (mContext, 2);
    SSL_CTX_set_read_ahead(mContext, 1);

    ELOG_DEBUG("Dtls factory created");
}

DtlsFactory::~DtlsFactory()
{
   SSL_CTX_free(mContext);
}


DtlsSocket*
DtlsFactory::createClient(boost::shared_ptr<DtlsSocketContext> context)
{
   return new DtlsSocket(context,this,DtlsSocket::Client);
}

DtlsSocket*
DtlsFactory::createServer(boost::shared_ptr<DtlsSocketContext> context)
{
   return new DtlsSocket(context,this,DtlsSocket::Server);
}

void
DtlsFactory::getMyCertFingerprint(char *fingerprint)
{
   DtlsSocket::computeFingerprint(DtlsFactory::mCert,fingerprint);
}

void
DtlsFactory::setSrtpProfiles(const char *str)
{
   int r = SSL_CTX_set_tlsext_use_srtp(mContext,str);
   assert(r==0);
   (void)r;  // Unused.
}

void
DtlsFactory::setCipherSuites(const char *str)
{
   int r = SSL_CTX_set_cipher_list(mContext,str);
   assert(r==1);
   (void)r;  // Unused.
}

DtlsFactory::PacketType
DtlsFactory::demuxPacket(const unsigned char *data, unsigned int len)
{
   assert(len>=1);

   if((data[0]==0)   || (data[0]==1))
      return stun;
   if((data[0]>=128) && (data[0]<=191))
      return rtp;
   if((data[0]>=20)  && (data[0]<=64))
      return dtls;

   return unknown;
}


/* ====================================================================

 Copyright (c) 2007-2008, Eric Rescorla and Derek MacDonald
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. None of the contributors names may be used to endorse or promote
    products derived from this software without specific prior written
    permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
