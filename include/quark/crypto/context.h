/* *
 * CWire's secure context is based off NodeJS's implimentation.
 * Thus, most of the functions below are adapted from NodeJS's source;
 * Available here: https://github.com/nodejs/node/blob/master/src/crypto/crypto_context.cc
 * */

#ifndef QRK_CRYPTO_CONTEXT_H
#define QRK_CRYPTO_CONTEXT_H
#include <quark/std/alloc.h>
#include <quark/crypto/bio.h>
#include <quark/std/buf.h>
#include <quark/std/err.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/kdf.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>

#define qrk_USE_OPENSSL_DEFAULT_STORE 0

typedef struct qrk_secure_ctx_s qrk_secure_ctx_t;

struct qrk_secure_ctx_s {
    qrk_malloc_ctx_t *m_ctx; 
    SSL_CTX *ssl_ctx; 
    void* data;
};


int qrk_crypto_no_password_cb (char* buf, int size, int rwflag, void* u) ;
int qrk_crypto_password_cb (char* buf, int size, int rwflag, void* u);

/* *
 * This function is pulled from stackoverflow
 * https://stackoverflow.com/questions/3810058/read-certificate-files-from-memory-instead-of-a-file-using-openssl
 * */
unsigned long qrk_sec_ctx_ssl_ctx_use_certificate_chain_bio (SSL_CTX* context, BIO* cbio);

X509_STORE* qrk_sec_ctx_new_root_cert_store (qrk_malloc_ctx_t *ctx, int use_openssl_default_store); // NewRootCertStore
unsigned long qrk_sec_ctx_set_key (qrk_secure_ctx_t *ctx, BIO *key, const qrk_rbuf_t *password); // SetKey
unsigned long qrk_sec_ctx_set_cert (qrk_secure_ctx_t *ctx, BIO *cert); // SetCert
unsigned long qrk_sec_ctx_add_cacert (qrk_secure_ctx_t *ctx, BIO *bio); // AddCACert
unsigned long qrk_sec_ctx_add_crl (qrk_secure_ctx_t *ctx, BIO *bio); // AddCRL
qrk_err_impl_t qrk_sec_ctx_add_root_certs (qrk_secure_ctx_t *ctx); // AddRootCerts
unsigned long qrk_sec_ctx_set_cipher_suites (qrk_secure_ctx_t *ctx, const char* ciphers); // SetCipherSuites
unsigned long qrk_sec_ctx_set_ciphers (qrk_secure_ctx_t *ctx, const char* ciphers); // SetCiphers
unsigned long qrk_sec_ctx_set_ecdh_curve (qrk_secure_ctx_t *ctx, const char* curve); // SetECDHCurve
unsigned long qrk_sec_ctx_set_sigalgs (qrk_secure_ctx_t *ctx, const char* sigalgs); // SetSigalgs

unsigned long qrk_sec_ctx_init (qrk_secure_ctx_t *ctx, qrk_malloc_ctx_t *m_ctx, const SSL_METHOD* method, int min_version, int max_version);
void qrk_sec_ctx_free (qrk_secure_ctx_t *ctx);

/** TODO: NOT YET IMPLEMENTED
 * LoadPKCS12
 * SetDHParam
 * GetTicketKeys
 * SetTicketKeys
 * SetSessionIdContext
 * */

#endif //QRK_CRYPTO_CONTEXT_H