/* cmm2_ca.c: the CA bundle parsed once for all the browser's connections (apps/browser; patches/netsurf-0003 calls
   cmm2_curl_ca for each fetch that verifies certificates). Left to itself, curl's mbedTLS back end reads and parses
   the whole bundle (curl's cacert.pem, 118 certificates, 200 KB) again for every connection: 0.6 to 0.8 s each on the
   CMM2 and about 300 KB of memory while it lasts (docs/Web_Browser_Study.md, step 1), and a page opens many. Here it
   is parsed the first time it is wanted and kept; each connection's mbedTLS configuration is given that chain by
   CURLOPT_SSL_CTX_FUNCTION, which curl calls after it sets up its own (empty, as CURLOPT_CAINFO is NULL) chain.
   If the bundle cannot be read, curl is left to read it itself and report why. MIT License, as Inc/cmm2img.h. */
#include <curl/curl.h>
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"

static mbedtls_x509_crt ca;
static int ca_state;                            /* 0 not read yet, 1 parsed, -1 it could not be */

static CURLcode sslctx(CURL *curl, void *ssl_ctx, void *userptr) {
    (void)curl;
    (void)userptr;
    mbedtls_ssl_conf_ca_chain((mbedtls_ssl_config *)ssl_ctx, &ca, NULL);
    return CURLE_OK;
}

void cmm2_curl_ca(CURL *handle, const char *bundle) {
    if(ca_state == 0) {
        mbedtls_x509_crt_init(&ca);
        ca_state = bundle && *bundle && mbedtls_x509_crt_parse_file(&ca, bundle) >= 0 ? 1 : -1;   /* (>0: some
                                                   certificates in it were not understood, the rest kept) */
    }
    if(ca_state < 0) return;
    curl_easy_setopt(handle, CURLOPT_CAINFO, NULL);
    curl_easy_setopt(handle, CURLOPT_CAPATH, NULL);
    curl_easy_setopt(handle, CURLOPT_SSL_CTX_FUNCTION, sslctx);
}
