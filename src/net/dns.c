#include <quark/net/dns.h>
#include <quark/std/utils.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

int qrk_dns_is_numeric_host_af (const char *hostname, int family) 
{
	struct in6_addr dst;
	return uv_inet_pton(family, hostname, &dst) == 0;
}

int qrk_dns_is_numeric_host_v6 (const char *hostname) 
{
	return qrk_dns_is_numeric_host_af(hostname, AF_INET6);
}

int qrk_dns_is_numeric_host_v4 (const char *hostname) 
{
	return qrk_dns_is_numeric_host_af(hostname, AF_INET);
}

int qrk_dns_is_numeric_host_v (const char *hostname) 
{
	int v = 0;
	if (qrk_dns_is_numeric_host_v4(hostname))
		v = 4;
	else if (qrk_dns_is_numeric_host_v6(hostname))
		v = 6;
	return v;
}

int qrk_dns_is_numeric_host (const char *hostname) 
{
	return qrk_dns_is_numeric_host_v6(hostname) ||
		   qrk_dns_is_numeric_host_v4(hostname);
}

typedef struct qrk_getaddrinfo_s {
	qrk_loop_t *loop;
	qrk_getaddrinfo_cb callback;
	uv_getaddrinfo_t req;
} qrk_getaddrinfo_t;

static void qrk__dns_getaddrinfo_cb (uv_getaddrinfo_t* req, int status, struct addrinfo* res)
{
	qrk_getaddrinfo_t *ctx = (qrk_getaddrinfo_t *)req->data;
	if (status != 0)
	{
		qrk_err_t err = {
				.code = status,
				.origin = QRK_EO_UV,
				.target_type = ctx->loop->type,
				.target = ctx->loop
		};
		ctx->callback(ctx->loop, &err, NULL);
		goto cleanup;
	}
	ctx->callback(ctx->loop, NULL, res->ai_addr);
cleanup:
	uv_freeaddrinfo(res);
	qrk_free(ctx->loop->m_ctx, ctx);
}

void qrk_dns_getaddrinfo (qrk_loop_t *loop, const char *hostname, const char *port, qrk_getaddrinfo_cb callback)
{
	int r;
	int n = qrk_dns_is_numeric_host_v(hostname);
	if (n)
	{
		switch (n)
		{
			case 4:
			{
				struct sockaddr_in dst;
				r = uv_ip4_addr(hostname, atoi(port), &dst);
				if (!r)
					callback(loop, NULL, (struct sockaddr *)&dst);
			}
			break;

			case 6:
			{
				struct sockaddr_in6 dst;
				r = uv_ip6_addr(hostname, atoi(port), &dst);
				if (!r)
					callback(loop, NULL, (struct sockaddr *)&dst);
			}
			break;

			default:
			{
				qrk_err_t err = {
					.code = QRK_E_UNREACHABLE,
					.origin = QRK_EO_IMPL,
					.target_type = loop->type,
					.target = loop
				};
				callback(loop, &err, NULL);
				return;
			}
		}

		goto cleanup;
	};

	qrk_getaddrinfo_t *ctx = qrk_malloc(loop->m_ctx, sizeof(qrk_getaddrinfo_t));
	if (ctx == NULL)
	{
		qrk_err_t err = {
			.code = QRK_E_OOM,
			.origin = QRK_EO_IMPL,
			.target_type = loop->type,
			.target = loop
		};
		callback(loop, &err, NULL);
	}
	ctx->req.data = ctx;

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	r = uv_getaddrinfo(
		ctx->loop->loop,
		&ctx->req,
		qrk__dns_getaddrinfo_cb,
		hostname, port, &hints
	);

cleanup:
	if (unlikely(r != 0))
	{
		qrk_err_t err = {
			.code = r,
			.origin = QRK_EO_UV,
			.target_type = loop->type,
			.target = loop
		};
		callback(loop, &err, NULL);
		qrk_free(loop->m_ctx, ctx);
	}
}
