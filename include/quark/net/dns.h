#ifndef QRK_NET_DNS_H
#define QRK_NET_DNS_H

#include <uv.h>
#include <quark/std/stream.h>

typedef void (*qrk_getaddrinfo_cb)(qrk_loop_t *loop, qrk_err_t *err, struct sockaddr *addr);

int qrk_dns_is_numeric_host_af (const char *hostname, int family);
int qrk_dns_is_numeric_host_v6 (const char *hostname);
int qrk_dns_is_numeric_host_v4 (const char *hostname);
int qrk_dns_is_numeric_host_v (const char *hostname);
int qrk_dns_is_numeric_host (const char *hostname);
void qrk_dns_getaddrinfo (qrk_loop_t *loop, const char *hostname, const char *port, qrk_getaddrinfo_cb callback);
// TODO: add getnameinfo

#endif
