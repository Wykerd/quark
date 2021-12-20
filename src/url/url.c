#include <quark/url/url.h>
#include <quark/std/utils.h>
#include <ctype.h>
#include <stdlib.h>
#include <unicode/utypes.h>
#include <unicode/uidna.h>
#include <math.h>
#include <uv.h>

#include <quark/no_malloc.h>

const char PERCENT_ENCODING[1024] =
    "%00\0%01\0%02\0%03\0%04\0%05\0%06\0%07\0"
    "%08\0%09\0%0A\0%0B\0%0C\0%0D\0%0E\0%0F\0"
    "%10\0%11\0%12\0%13\0%14\0%15\0%16\0%17\0"
    "%18\0%19\0%1A\0%1B\0%1C\0%1D\0%1E\0%1F\0"
    "%20\0%21\0%22\0%23\0%24\0%25\0%26\0%27\0"
    "%28\0%29\0%2A\0%2B\0%2C\0%2D\0%2E\0%2F\0"
    "%30\0%31\0%32\0%33\0%34\0%35\0%36\0%37\0"
    "%38\0%39\0%3A\0%3B\0%3C\0%3D\0%3E\0%3F\0"
    "%40\0%41\0%42\0%43\0%44\0%45\0%46\0%47\0"
    "%48\0%49\0%4A\0%4B\0%4C\0%4D\0%4E\0%4F\0"
    "%50\0%51\0%52\0%53\0%54\0%55\0%56\0%57\0"
    "%58\0%59\0%5A\0%5B\0%5C\0%5D\0%5E\0%5F\0"
    "%60\0%61\0%62\0%63\0%64\0%65\0%66\0%67\0"
    "%68\0%69\0%6A\0%6B\0%6C\0%6D\0%6E\0%6F\0"
    "%70\0%71\0%72\0%73\0%74\0%75\0%76\0%77\0"
    "%78\0%79\0%7A\0%7B\0%7C\0%7D\0%7E\0%7F\0"
    "%80\0%81\0%82\0%83\0%84\0%85\0%86\0%87\0"
    "%88\0%89\0%8A\0%8B\0%8C\0%8D\0%8E\0%8F\0"
    "%90\0%91\0%92\0%93\0%94\0%95\0%96\0%97\0"
    "%98\0%99\0%9A\0%9B\0%9C\0%9D\0%9E\0%9F\0"
    "%A0\0%A1\0%A2\0%A3\0%A4\0%A5\0%A6\0%A7\0"
    "%A8\0%A9\0%AA\0%AB\0%AC\0%AD\0%AE\0%AF\0"
    "%B0\0%B1\0%B2\0%B3\0%B4\0%B5\0%B6\0%B7\0"
    "%B8\0%B9\0%BA\0%BB\0%BC\0%BD\0%BE\0%BF\0"
    "%C0\0%C1\0%C2\0%C3\0%C4\0%C5\0%C6\0%C7\0"
    "%C8\0%C9\0%CA\0%CB\0%CC\0%CD\0%CE\0%CF\0"
    "%D0\0%D1\0%D2\0%D3\0%D4\0%D5\0%D6\0%D7\0"
    "%D8\0%D9\0%DA\0%DB\0%DC\0%DD\0%DE\0%DF\0"
    "%E0\0%E1\0%E2\0%E3\0%E4\0%E5\0%E6\0%E7\0"
    "%E8\0%E9\0%EA\0%EB\0%EC\0%ED\0%EE\0%EF\0"
    "%F0\0%F1\0%F2\0%F3\0%F4\0%F5\0%F6\0%F7\0"
    "%F8\0%F9\0%FA\0%FB\0%FC\0%FD\0%FE\0%FF";

const uint8_t C0_CONTROL_PERCENT_ENCODE_SET[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

const uint8_t FRAGMENT_PERCENT_ENCODE_SET[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

const  uint8_t QUERY_PERCENT_ENCODE_SET[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

const uint8_t SPECIAL_QUERY_PERCENT_ENCODE_SET[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

const uint8_t PATH_PERCENT_ENCODE_SET[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

const uint8_t USERINFO_PERCENT_ENCODE_SET[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

const uint8_t COMPONENT_PERCENT_ENCODE_SET[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

const uint8_t FORBIDDEN_HOST_CODE_POINT[256] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const struct {
    const char *key;
    size_t keylen;
    int port;
} SPECIAL_SCHEME[6] = {
    {
        "ftp", 3, 21
    },
    {
        "file", 4, -1
    },
    {
        "http", 4, 80
    },
    {
        "https", 5, 443
    },
    {
        "ws", 2, 80
    },
    {
        "wss", 3, 443
    }
};

static inline
int qrk_url_scheme_is_special (qrk_rbuf_t *scheme)
{
    for (int i = 0; i < 6; i++)
        if ((SPECIAL_SCHEME[i].keylen == scheme->len) &&
            !memcmp(scheme->base, SPECIAL_SCHEME[i].key, SPECIAL_SCHEME[i].keylen))
            return 1;

    return 0;
}

static inline
int qrk_url_port_is_default (qrk_rbuf_t *scheme, int port)
{
    for (int i = 0; i < 6; i++)
        if ((SPECIAL_SCHEME[i].keylen == scheme->len) &&
            !memcmp(scheme->base, SPECIAL_SCHEME[i].key, SPECIAL_SCHEME[i].keylen))
            return port == SPECIAL_SCHEME[i].port;

    return 0;
}

int qrk_url_percent_decode2 (qrk_str_t *dest, qrk_rbuf_t *src, int decode_plus)
{
    char a, b, ch;
    for (size_t i = 0; i < src->len; i++)
    {
        if ((src->base[i] == '%') && (i + 2 <= src->len) &&
            isxdigit(a = src->base[i + 1]) && isxdigit(b = src->base[i + 2]))
        {
            i += 2;
            if (a < 'A') a -= '0';
            else if(a < 'a') a -= 'A' - 10;
            else a -= 'a' - 10;
            if (b < 'A') b -= '0';
            else if(b < 'a') b -= 'A' - 10;
            else b -= 'a' - 10;
            ch = 16 * a + b;
            qrk_str_putc(dest, ch);
        }
        else
            if (!qrk_str_putc(dest, (decode_plus && src->base[i] == '+') ? ' ' : (src->base[i])))
                return 1;
    }
    return 0;
}

int qrk_url_percent_decode (qrk_str_t *dest, qrk_rbuf_t *src)
{
    return qrk_url_percent_decode2(dest, src, 0);
}

int qrk_url_percent_encode2 (qrk_str_t *dest, const char *src, size_t len, const uint8_t *set)
{
    for (size_t i = 0; i < len; i++)
    {
        if (set[(uint8_t)src[i]])
        {
            if (!qrk_str_push_back(dest, PERCENT_ENCODING + (uint8_t)src[i] * 4, 3))
                return 1;
        }
        else
        if (!qrk_str_putc(dest, src[i]))
            return 1;
    }
    return 0;
}

int qrk_url_percent_encode (qrk_str_t *dest, qrk_rbuf_t *src, const uint8_t *set)
{
    return qrk_url_percent_encode2(dest, src->base, src->len, set);
}

int qrk_url_domain_to_ascii (qrk_str_t *dest, qrk_rbuf_t *src, int be_strict)
{
    UErrorCode err = U_ZERO_ERROR;

    uint32_t options = UIDNA_CHECK_BIDI | UIDNA_CHECK_CONTEXTJ | UIDNA_NONTRANSITIONAL_TO_ASCII;
    if (be_strict)
        options |= UIDNA_USE_STD3_RULES;

    UIDNA *idna = uidna_openUTS46(options, &err);

    if (U_FAILURE(err))
        return 1;

    UIDNAInfo info = UIDNA_INFO_INITIALIZER;

    int output_length = uidna_nameToASCII_UTF8(idna, src->base, (int32_t)src->len, dest->base, (int32_t)dest->size, &info, &err);

    if (err == U_BUFFER_OVERFLOW_ERROR)
    {
        err = U_ZERO_ERROR;
        qrk_str_resize(dest, output_length);
        output_length = uidna_nameToASCII_UTF8(idna,  src->base, (int32_t)src->len, dest->base, (int32_t)dest->size, &info, &err);
    }

    dest->len = output_length;

    // From chromium source code: https://chromium.googlesource.com/chromium/src/+/lkgr/url/url_idna_icu.cc
    // Ignore various errors for web compatibility. The options are specified
    // by the WHATWG URL Standard. See
    //  - https://unicode.org/reports/tr46/
    //  - https://url.spec.whatwg.org/#concept-domain-to-ascii
    //    (we set beStrict to false)

    // Disable the "CheckHyphens" option in UTS #46. See
    //  - https://crbug.com/804688
    //  - https://github.com/whatwg/url/issues/267
    info.errors &= ~UIDNA_ERROR_HYPHEN_3_4;
    info.errors &= ~UIDNA_ERROR_LEADING_HYPHEN;
    info.errors &= ~UIDNA_ERROR_TRAILING_HYPHEN;

    if (!be_strict) {
        // Disable the "VerifyDnsLength" option in UTS #46.
        info.errors &= ~UIDNA_ERROR_EMPTY_LABEL;
        info.errors &= ~UIDNA_ERROR_LABEL_TOO_LONG;
        info.errors &= ~UIDNA_ERROR_DOMAIN_NAME_TOO_LONG;
    }

    int ret = 0;
    if (U_SUCCESS(err) && info.errors == 0)
    {
        if (output_length == 0)
            ret = 1;
    }

    if (U_FAILURE(err) || info.errors != 0)
        ret = 1;

    uidna_close(idna);
    return ret;
}

int qrk_url_domain_to_unicode (qrk_str_t *dest, qrk_rbuf_t *src)
{
    UErrorCode err = U_ZERO_ERROR;

    uint32_t options = UIDNA_NONTRANSITIONAL_TO_UNICODE; // | UIDNA_CHECK_BIDI | UIDNA_CHECK_CONTEXTJ;
                                                         // we do not report validation errors so these aren't required

    UIDNA *idna = uidna_openUTS46(options, &err);
    if (U_FAILURE(err))
        return 1;

    UIDNAInfo info = UIDNA_INFO_INITIALIZER;

    int output_length = uidna_nameToUnicode(idna, (const UChar *) src->base, src->len, (UChar *) dest->base, dest->size, &info, &err);

    if (err == U_BUFFER_OVERFLOW_ERROR)
    {
        err = U_ZERO_ERROR;
        qrk_str_resize(dest, output_length);
        output_length = uidna_nameToUnicode(idna, (const UChar *) src->base, src->len, (UChar *) dest->base, dest->size, &info, &err);
    }

    if (U_FAILURE(err))
        dest->len = 0;
    else
        dest->len = output_length;

    uidna_close(idna);

    return 0;
}

int qrk_url_host_ipv4_number (uint32_t *dest, const char *src, size_t len, int *validation_error)
{
    int r = 10;
    if (len >= 2 && ((src[0] == '0' && src[1] == 'x') || (src[0] == '0' && src[1] == 'X')))
    {
        *validation_error = 1;
        r = 16;
        src += 2;
        len -= 2;
    }
    else if (len >= 1 && src[0] == '0')
    {
        *validation_error = 1;
        r = 8;
        src++;
        len--;
    }
    if (len == 0)
    {
        *validation_error = 1;
        *dest = 0;
        return 0;
    }
    const char *cur = src;
    const char *end = src + len;
    while (cur < end)
    {
        const char ch = *cur;
        switch(r)
        {
            case 8:
                if (ch < '0' || ch > '7')
                    return 1;
                break;
            case 10:
                if (ch < '0' || ch > '9')
                    return 1;
                break;
            case 16:
                if (!isxdigit(ch))
                    return 1;
                break;
            default:
                return 1;
        }
        cur++;
    }
    *dest = strtoul(src, NULL, r);
    return 0;
}

int qrk_url_host_ipv4_parse (uint32_t *dest, qrk_rbuf_t *src, int *validation_error)
{
    *validation_error = 0;
    size_t parts[5] = {0};

    if (src->len == 0)
        return 1;

    int tokens = qrk_infra_split_strict_static(src, '.', parts, 5);
    tokens--; // exclude the eol token (-1)

    if (tokens < 0)
    {
        *validation_error = 1;
        return 1;
    }

    if (tokens > 3)
    {
        *validation_error = 1;

        if (parts[tokens - 1] != src->len - 1)
            return 1; // if the last part is not empty string, fail.

        tokens--; // exclude the empty string
    }

    tokens++; // add the eol token

    uint32_t numbers[4] = {0};

    int big_numbers = 0;

    for (int i = 0; i < tokens; i++)
    {
        int r = qrk_url_host_ipv4_number(
            &numbers[i],
            i == 0 ? src->base : src->base + parts[i - 1] + 1,
            i == 0 ? parts[i] : parts[i] - parts[i - 1] - 1,
            validation_error);
        if (numbers[i] > 255)
        {
            big_numbers++;
            *validation_error = 1;
        }
        if (r != 0)
        {
            *validation_error = 1;
            return 1;
        }
    }

    if (big_numbers > 1 || numbers[tokens - 1] >= pow(256, 5 - tokens))
        return 2;

    uint32_t ipv4 = numbers[tokens - 1];
    for (int n = 0; n < tokens - 1; n++)
    {
        ipv4 += numbers[n] * (uint32_t)pow(256, 3 - n);
    }

    *dest = ipv4;
    return 0;
}

int qrk_url_host_ipv6_parse (uint16_t *dest, qrk_rbuf_t *src, int *validation_error)
{
    *validation_error = 0;

    int pieceIndex = 0;
    int compress = -1;
    const char *c = src->base;
    const char *end = src->base + src->len - 1;

    if (*c == ':') {
        if (src->len == 1 || (c[1] != ':'))
        {
            *validation_error = 1;
            return 1;
        }
        c += 2;
        pieceIndex++;
        compress = pieceIndex;
    }

    while (c <= end)
    {
        if (pieceIndex == 8)
        {
            *validation_error = 1;
            return 1;
        }

        if (*c == ':')
        {
            if (compress != -1)
            {
                *validation_error = 1;
                return 1;
            }
            c++;
            pieceIndex++;
            compress = pieceIndex;
            continue;
        }

        uint16_t value = 0;
        int length = 0;

        while (c <= end && length < 4 && isxdigit(*c))
        {
            value = value * 0x10 + (*c <= '9' ? *c - '0' : (tolower(*c) - 'a' + 10));
            c++;
            length++;
        }

        if (*c == '.')
        {
            if (length == 0) {
                *validation_error = 1;
                return 1;
            }

            c -= length;

            if (pieceIndex > 6)
            {
                *validation_error = 1;
                return 1;
            }

            int numbersSeen = 0;

            while (c <= end)
            {
                uint32_t ipv4Piece = -1;
                if (numbersSeen > 0)
                {
                    if (*c != '.' && numbersSeen < 4)
                    {
                        c++;
                    }
                    else
                    {
                        *validation_error = 1;
                        return 1;
                    }
                }
                if (!isxdigit(*c))
                {
                    *validation_error = 1;
                    return 1;
                }

                while (c <= end && isdigit(*c))
                {
                    uint32_t number = *c - '0';

                    if (ipv4Piece == -1)
                    {
                        ipv4Piece = number;
                    }
                    else if (ipv4Piece == 0)
                    {
                        *validation_error = 1;
                        return 1;
                    }
                    else
                    {
                        ipv4Piece = ipv4Piece * 10 + number;
                    }

                    if (ipv4Piece > 255)
                    {
                        *validation_error = 1;
                        return 1;
                    }

                    c++;
                }

                dest[pieceIndex] = dest[pieceIndex] * 0x100 + ipv4Piece;
                numbersSeen++;

                if (numbersSeen == 2 || numbersSeen == 4)
                    pieceIndex++;
            }

            if (numbersSeen != 4)
            {
                *validation_error = 1;
                return 1;
            }

            break;
        }
        else if (*c == ':')
        {
            c++;
            if (c > end)
            {
                *validation_error = 1;
                return 1;
            }
        }
        else if (c < end)
        {
            *validation_error = 1;
            return 1;
        }
        dest[pieceIndex] = value;
        pieceIndex++;
    }

    if (compress != -1)
    {
        int swaps = pieceIndex - compress;
        pieceIndex = 7;
        while (pieceIndex != 0 && swaps > 0)
        {
            uint16_t temp = dest[pieceIndex];
            dest[pieceIndex] = dest[compress + swaps - 1];
            dest[compress + swaps - 1] = temp;
            pieceIndex--;
            swaps--;
        }
    }
    else if (pieceIndex != 8)
    {
        *validation_error = 1;
        return 1;
    }

    return 0;
}

int qrk_url_host_opaque_parse (qrk_str_t *dest, qrk_rbuf_t *src, int *validation_error)
{
    *validation_error = 0;

    // TODO: we might want to add these validation checks in the future
    // XXX: we do not validate that %-encoded characters are valid.
    // XXX: we do not validate that characters are URL code points.
    for (size_t i = 0; i < src->len; i++)
        if (src->base[i] != '%' && FORBIDDEN_HOST_CODE_POINT[(uint8_t)src->base[i]])
        {
            *validation_error = 1;
            return 1;
        }

    return qrk_url_percent_encode(dest, src, C0_CONTROL_PERCENT_ENCODE_SET);
}

int qrk_url_host_parse (qrk_url_host_t *dest, qrk_rbuf_t *src, int is_not_special, int *validation_error)
{
    if (src->len == 0)
        return 0;
    if (src->base[0] == '[')
    {
        if (src->base[src->len - 1] != ']')
            return 1;
        qrk_rbuf_t tsrc = {
            src->base + 1, src->len - 2
        }; // remove leading [ and trailing ]
        dest->type = QRK_URL_HOST_IPV6;
        return qrk_url_host_ipv6_parse(dest->ipv6, &tsrc, validation_error);
    }
    if (is_not_special)
    {
        dest->type = QRK_URL_HOST_OPAQUE;
        int r = qrk_url_host_opaque_parse(&dest->str, src, validation_error);
        if (dest->str.len == 0)
            dest->type = QRK_URL_HOST_EMPTY;
        return r;
    }
    qrk_str_t domain;
    if (qrk_str_malloc(&domain, dest->str.m_ctx, src->len) == NULL)
        return 1;
    if (qrk_url_percent_decode(&domain, src))
        goto fail1;
    qrk_str_t asciiDomain;
    if (qrk_str_malloc(&asciiDomain, dest->str.m_ctx, domain.len) == NULL)
        goto fail1;
    if (qrk_url_domain_to_ascii(&asciiDomain, (qrk_rbuf_t *) &domain, 0))
        goto fail;
    for (size_t i = 0; i < asciiDomain.len; i++)
        if (FORBIDDEN_HOST_CODE_POINT[(uint8_t)asciiDomain.base[i]])
            goto fail;

    int v4_validation_error = 0;
    int r = qrk_url_host_ipv4_parse(&dest->ipv4, (qrk_rbuf_t *) &asciiDomain, &v4_validation_error);

    if (!r)
    {
        qrk_str_free(&asciiDomain);
        qrk_str_free(&domain);
        *validation_error = v4_validation_error;
        dest->type = QRK_URL_HOST_IPV4;
        return 0;
    }

    if (r == 2)
        goto fail;

    dest->type = QRK_URL_HOST_DOMAIN;
    dest->str.len = 0;
    if (qrk_str_push_back(&dest->str, asciiDomain.base, asciiDomain.len) == NULL)
        goto fail;

    if (asciiDomain.len == 0)
        dest->type = QRK_URL_HOST_EMPTY;

    qrk_str_free(&asciiDomain);
    qrk_str_free(&domain);
    return 0;
fail:
    qrk_str_free(&asciiDomain);
fail1:
    qrk_str_free(&domain);
    return 1;
}

int qrk_url_host_serialize (qrk_str_t *dest, qrk_url_host_t *src)
{
    switch (src->type)
    {
        case QRK_URL_HOST_IPV4:
        {
            uint32_t n = src->ipv4;
            n = ((n & 0xFF000000) >> 24) | ((n & 0x00FF0000) >> 8) | ((n & 0x0000FF00) << 8) | ((n & 0x000000FF) << 24);
            for (int i = 0; i < 3; i++)
            {
                qrk_str_printf(dest, "%d.", n % 256);
                n /= 256;
            }
            qrk_str_printf(dest, "%d", n % 256);
        }
        break;

        case QRK_URL_HOST_IPV6:
        {
            qrk_str_putc(dest, '[');
            // find the longest sequence of 0s
            uint16_t *start = src->ipv6;
            uint16_t *end = src->ipv6 + 8;
            uint16_t *compress = NULL;

            uint16_t *current = NULL;
            unsigned counter = 0, longest = 1;

            while (start < end)
            {
                if (*start == 0)
                {
                    if (current == NULL)
                        current = start;
                    counter++;
                }
                else
                {
                    if (counter > longest)
                    {
                        longest = counter;
                        compress = current;
                    }
                    current = NULL;
                    counter = 0;
                }
                start++;
            }

            if (counter > longest)
                compress = current;


            int ignore0 = 0;

            for (int i = 0; i < 8; i++)
            {
                if (ignore0 && src->ipv6[i] == 0)
                    continue;
                ignore0 = 0;
                if (compress == src->ipv6 + i)
                {
                    if (i == 0)
                        qrk_str_push_back(dest, "::", 2);
                    else
                        qrk_str_putc(dest, ':');
                    ignore0 = 1;
                    continue;
                }
                qrk_str_printf(dest, "%x", src->ipv6[i]);
                if (i != 7)
                    qrk_str_putc(dest, ':');
            }

            qrk_str_putc(dest, ']');
        }
        break;

        case QRK_URL_HOST_OPAQUE:
        case QRK_URL_HOST_DOMAIN:
        {
            qrk_str_push_back(dest, src->str.base, src->str.len);
        }
        break;

        default:
            return 1;
    }

    return 0;
}

int qrk_url_host_init (qrk_url_host_t *host, qrk_malloc_ctx_t *ctx)
{
    memset(host, 0, sizeof(qrk_url_host_t));
    return qrk_str_malloc(&host->str, ctx, 64) == NULL;
}

int qrk_url_host_clone (qrk_url_host_t *dest, qrk_url_host_t *src)
{
    dest->type = src->type;
    dest->ipv4 = src->ipv4;
    memcpy(dest->ipv6, src->ipv6, sizeof(dest->ipv6));
    dest->str.len = 0;
    if (!qrk_str_push_back(&dest->str, src->str.base, src->str.len))
        return 1;
    return 0;
}

void qrk_url_host_free (qrk_url_host_t *host)
{
    qrk_str_free(&host->str);
}

int qrk_url_path_shorten (qrk_url_t *url)
{
    if (url->flags & QRK_URL_FLAG_PATH_IS_OPAQUE)
        return 1;

    if ((url->flags & QRK_URL_FLAG_IS_FILE) && url->path.nmemb == 1)
    {
        qrk_str_t *str = qrk_buf_get(&url->path, 0);
        if (str->len == 2 && isalpha(str->base[0]) && str->base[1] == ':')
            return 0;
    }

    if (url->path.nmemb > 0)
    {
        qrk_str_t *remove = qrk_buf_get(&url->path, url->path.nmemb - 1);
        qrk_str_free(remove);
        url->path.nmemb--;
    }

    return 0;
}

int qrk_url_path_starts_windows_drive_letter (const char *start, const char *end)
{
    if (end - start < 2)
        return 0;

    if (!(isalpha(start[0]) && (start[1] == ':' || start[1] == '|')))
        return 0;

    if (end - start > 2 && (!(start[2] == '/' || start[2] == '\\' || start[2] == '?' || start[2] == '#')))
        return 0;


    return 1;
}

int32_t qrk__url_port_normalize (uint32_t port, qrk_str_t *scheme)
{
    for (int i = 0; i < 6; i++) {
        if (scheme->len == SPECIAL_SCHEME[i].keylen &&
            port == SPECIAL_SCHEME[i].port &&
            !memcmp(scheme->base, SPECIAL_SCHEME[i].key, scheme->len))
            return -1;
    }
    return port;
}

int qrk_url_parse_basic (qrk_url_parser_t *parser, qrk_rbuf_t *_input, qrk_url_t *base,
                         qrk_url_t **_url, qrk_url_parser_state_t state_override)
{
    char *p = _input->base;
    char *end = _input->base + _input->len - 1;

    if (*_url == NULL)
    {
        *_url = qrk_malloc(parser->m_ctx, sizeof(qrk_url_t));
        if (*_url == NULL)
            return 1;
        memset(*_url, 0, sizeof(qrk_url_t));
        (*_url)->port = -1;

        qrk_malloc_ctx_t *ctx = parser->m_ctx;

        int r =
            !qrk_url_host_init(&(*_url)->host, ctx) &&
            qrk_str_malloc(&(*_url)->scheme, ctx, 16) &&
            qrk_str_malloc(&(*_url)->username, ctx, 1) &&
            qrk_str_malloc(&(*_url)->password, ctx, 1) &&
            qrk_str_malloc(&(*_url)->query, ctx, 1) &&
            qrk_str_malloc(&(*_url)->fragment, ctx, 1) &&
            qrk_buf_malloc(&(*_url)->path, ctx, sizeof(qrk_str_t), 1);

        if (r == 0)
            return 1;

        // remove leading or trailing C0 control or space characters
        for (const char *c = p; c < end; c++)
        {
            if (C0_CONTROL_PERCENT_ENCODE_SET[(uint8_t)*c] || *c == ' ')
                p++;
            else
                break;
        }

        for (const char *c = end; c >= p; c--)
        {
            if (C0_CONTROL_PERCENT_ENCODE_SET[(uint8_t)*c] || *c == ' ')
                end--;
            else
                break;
        }

        if (end != _input->base + _input->len - 1 || p != _input->base)
            (*_url)->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
    }

    qrk_str_t input;

    if (!qrk_str_malloc(&input, parser->m_ctx, end - p + 1))
        return 1;

    for (char *c = p; c <= end; c++)
        if (!(p[c - p] == '\n' || p[c - p] == '\r' || p[c - p] == '\t'))
            input.base[input.len++] = *c;

    qrk_url_t *url = *_url;

    if (state_override)
        parser->state = state_override;
    else
        parser->state = QRK_URL_PARSER_STATE_SCHEME_START;

    qrk_str_t buffer;

    if (!qrk_str_malloc(&buffer, parser->m_ctx, input.len))
        goto fail1;

    int atSignSeen = 0,
        insideBrackets = 0,
        passwordTokenSeen = 0;

    const char *c = input.base;
    const char *cend = input.base + input.len - 1;

    while (c <= cend + 1)
    {
        switch (parser->state)
        {
            case QRK_URL_PARSER_STATE_SCHEME_START:
            {
                if (isalpha(*c))
                {
                    const char ch = (char)tolower(*c);
                    if (qrk_str_putc(&buffer, ch) == NULL)
                        goto fail;
                    parser->state = QRK_URL_PARSER_STATE_SCHEME;
                }
                else if (!state_override)
                {
                    c--;
                    parser->state = QRK_URL_PARSER_STATE_NO_SCHEME;
                }
                else
                {
                    url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    goto fail;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_SCHEME:
            {
                if (isalnum(*c) || *c == '+' || *c == '-' || *c == '.')
                {
                    const char ch = (char)tolower(*c);
                    if (qrk_str_putc(&buffer, ch) == NULL)
                        goto fail;
                }
                else if (*c == ':')
                {
                    if (state_override)
                    {
                        int is_special = qrk_url_scheme_is_special((qrk_rbuf_t *) &buffer);

                        // if buffer is special does not match url is special:
                        if (is_special != (url->flags & QRK_URL_FLAG_SPECIAL))
                            goto ret;

                        // if buffer is file and url includes credentials or has non-null port:
                        if ((buffer.len == 4 && !memcmp(buffer.base, "file", 4)) &&
                            (((url->flags & QRK_URL_FLAG_HAS_PASSWORD) && (url->flags & QRK_URL_FLAG_HAS_USERNAME)) ||
                            (url->port != -1)))
                            goto ret;

                        // if url scheme is file and its host is empty host:
                        if (url->scheme.len == 4 && !memcmp(url->scheme.base, "file", 4) &&
                            url->host.type == QRK_URL_HOST_EMPTY)
                            goto ret;
                    }

                    url->scheme.len = 0;
                    if (qrk_str_push_back(&url->scheme, buffer.base, buffer.len) == NULL)
                        goto fail;

                    int is_default_port = qrk_url_port_is_default((qrk_rbuf_t *) &url->scheme, url->port);
                    if (state_override && is_default_port)
                    {
                        url->port = -1;
                        goto ret;
                    }

                    buffer.len = 0;
                    int special = qrk_url_scheme_is_special((qrk_rbuf_t *) &url->scheme);

                    if (special)
                        url->flags |= QRK_URL_FLAG_SPECIAL;
                    else
                        url->flags &= ~QRK_URL_FLAG_SPECIAL;

                    int is_file = url->scheme.len == 4 && !memcmp(url->scheme.base, "file", 4);

                    if (is_file)
                        url->flags |= QRK_URL_FLAG_IS_FILE;
                    else
                        url->flags &= ~QRK_URL_FLAG_IS_FILE;

                    if (is_file)
                    {
                        if ((c + 3 > cend) || memcmp(c + 1, "//", 2) != 0)
                        {
                            url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                        }
                        parser->state = QRK_URL_PARSER_STATE_FILE;
                    }
                    else if (special && base != NULL &&
                            (url->scheme.len == base->scheme.len && !memcmp(url->scheme.base, base->scheme.base, url->scheme.len)))
                    {
                        parser->state = QRK_URL_PARSER_STATE_SPECIAL_RELATIVE_OR_AUTHORITY;
                    }
                    else if (special)
                    {
                        parser->state = QRK_URL_PARSER_STATE_SPECIAL_AUTHORITY_SLASHES;
                    }
                    else if (c < cend && c[1] == '/')
                    {
                        parser->state = QRK_URL_PARSER_STATE_PATH_OR_AUTHORITY;
                        c++;
                    }
                    else
                    {
                        if (url->path.nmemb != 0)
                            for (size_t i = 0; i <url->path.nmemb; i++)
                                qrk_str_free((qrk_str_t *)(url->path.base + (i * url->path.memb_size)));

                        url->path.nmemb = 0;
                        qrk_str_t str;
                        if (!qrk_str_malloc(&str, parser->m_ctx, 1))
                            return 1;
                        qrk_buf_push_back1(&url->path, &str);

                        url->flags |= QRK_URL_FLAG_PATH_IS_OPAQUE;

                        parser->state = QRK_URL_PARSER_STATE_OPAQUE_PATH;
                    }
                }
                else if (!state_override)
                {
                    parser->state = QRK_URL_PARSER_STATE_NO_SCHEME;
                    buffer.len = 0;
                    c = input.base;
                    continue;
                }
                else
                {
                    url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    goto fail;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_NO_SCHEME:
            {
                if ((base == NULL || (base->flags & QRK_URL_FLAG_PATH_IS_OPAQUE)) && (*c != '#'))
                {
                    url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    goto fail;
                }
                else if (base != NULL && (base->flags & QRK_URL_FLAG_PATH_IS_OPAQUE) && (*c == '#'))
                {
                    url->scheme.len = 0;
                    if (!qrk_str_push_back(&url->scheme, base->scheme.base, base->scheme.len))
                        goto fail;

                    if (base->flags & QRK_URL_FLAG_SPECIAL)
                        url->flags |= QRK_URL_FLAG_SPECIAL;
                    else
                        url->flags &= ~QRK_URL_FLAG_SPECIAL;

                    if (base->flags & QRK_URL_FLAG_IS_FILE)
                        url->flags |= QRK_URL_FLAG_IS_FILE;
                    else
                        url->flags &= ~QRK_URL_FLAG_IS_FILE;

                    if (url->path.nmemb != 0)
                        for (size_t i = 0; i <url->path.nmemb; i++)
                            qrk_str_free((qrk_str_t *)(url->path.base + (i * url->path.memb_size)));
                    url->path.nmemb = 0;
                    for (size_t i = 0; i < base->path.nmemb; i++)
                    {
                        qrk_str_t *path_i = (qrk_str_t *)(base->path.base + (i * base->path.memb_size));
                        qrk_str_t str;
                        if (!qrk_str_malloc(&str, parser->m_ctx, path_i->len))
                            return 1;
                        if (!qrk_buf_push_back1(&url->path, &str))
                            goto fail;
                    }
                    url->flags |= QRK_URL_FLAG_PATH_IS_OPAQUE;

                    url->query.len = 0;
                    if (!qrk_str_push_back(&url->query, base->query.base, base->query.len))
                        goto fail;

                    url->fragment.len = 0;

                    parser->state = QRK_URL_PARSER_STATE_FRAGMENT;
                }
                else if ((base != NULL) && !(base->flags & QRK_URL_FLAG_IS_FILE))
                {
                    parser->state = QRK_URL_PARSER_STATE_RELATIVE;
                    c--;
                }
                else
                {
                    parser->state = QRK_URL_PARSER_STATE_FILE;
                    c--;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_SPECIAL_RELATIVE_OR_AUTHORITY:
            {
                if (*c == '/' && c < cend & c[1] == '/')
                {
                    parser->state = QRK_URL_PARSER_STATE_SPECIAL_AUTHORITY_IGNORE_SLASHES;
                    c++;
                }
                else
                {
                    url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    parser->state = QRK_URL_PARSER_STATE_RELATIVE;
                    c--;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_PATH_OR_AUTHORITY:
            {
                if (*c == '/')
                {
                    parser->state = QRK_URL_PARSER_STATE_AUTHORITY;
                }
                else
                {
                    parser->state = QRK_URL_PARSER_STATE_PATH;
                    c--;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_RELATIVE:
            {
                if (base->flags & QRK_URL_FLAG_IS_FILE)
                    goto fail;
                url->scheme.len = 0;
                if (!qrk_str_push_back(&url->scheme, base->scheme.base, base->scheme.len))
                    goto fail;

                if (base->flags & QRK_URL_FLAG_SPECIAL)
                    url->flags |= QRK_URL_FLAG_SPECIAL;
                else
                    url->flags &= ~QRK_URL_FLAG_SPECIAL;

                if (base->flags & QRK_URL_FLAG_IS_FILE)
                    url->flags |= QRK_URL_FLAG_IS_FILE;
                else
                    url->flags &= ~QRK_URL_FLAG_IS_FILE;

                if (*c == '/')
                    parser->state = QRK_URL_PARSER_STATE_RELATIVE_SLASH;
                else if ((url->flags & QRK_URL_FLAG_SPECIAL) && (*c == '\\'))
                {
                    url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    parser->state = QRK_URL_PARSER_STATE_RELATIVE_SLASH;
                }
                else
                {
                    url->username.len = 0;
                    if (!qrk_str_push_back(&url->username, base->username.base, base->username.len))
                        goto fail;

                    url->password.len = 0;
                    if (!qrk_str_push_back(&url->password, base->password.base, base->password.len))
                        goto fail;

                    if (qrk_url_host_clone(&url->host, &base->host))
                        goto fail;

                    url->port = base->port;

                    // clone path
                    if (url->path.nmemb != 0)
                        for (size_t i = 0; i <url->path.nmemb; i++)
                            qrk_str_free((qrk_str_t *)(url->path.base + (i * url->path.memb_size)));
                    url->path.nmemb = 0;
                    for (size_t i = 0; i < base->path.nmemb; i++)
                    {
                        qrk_str_t *path_i = (qrk_str_t *)(base->path.base + (i * base->path.memb_size));
                        qrk_str_t str;
                        if (!qrk_str_malloc(&str, parser->m_ctx, path_i->len))
                            goto fail;
                        if (!qrk_str_push_back(&str, path_i->base, path_i->len))
                        {
                            qrk_str_free(&str);
                            goto fail;
                        }
                        if (!qrk_buf_push_back1(&url->path, &str))
                            goto fail;
                    }
                    if (base->flags & QRK_URL_FLAG_PATH_IS_OPAQUE)
                        url->flags |= QRK_URL_FLAG_PATH_IS_OPAQUE;
                    else
                        url->flags &= ~QRK_URL_FLAG_PATH_IS_OPAQUE;

                    url->query.len = 0;
                    if (!qrk_str_push_back(&url->query, base->query.base, base->query.len))
                        goto fail;

                    if (*c == '?')
                    {
                        url->query.len = 0;
                        parser->state = QRK_URL_PARSER_STATE_QUERY;
                    }
                    else if (*c == '#')
                    {
                        url->fragment.len = 0;
                        parser->state = QRK_URL_PARSER_STATE_FRAGMENT;
                    }
                    else if (c <= cend)
                    {
                        url->query.len = 0;
                        if (qrk_url_path_shorten(url))
                            goto fail;
                        parser->state = QRK_URL_PARSER_STATE_PATH;
                        c--;
                    }
                }
            }
            break;

            case QRK_URL_PARSER_STATE_RELATIVE_SLASH:
            {
                if (url->flags & QRK_URL_FLAG_SPECIAL && (*c == '/' || *c == '\\'))
                {
                    if (*c == '\\')
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    parser->state = QRK_URL_PARSER_STATE_SPECIAL_AUTHORITY_IGNORE_SLASHES;
                }
                else if (*c == '/')
                {
                    parser->state = QRK_URL_PARSER_STATE_AUTHORITY;
                }
                else
                {
                    url->username.len = 0;
                    if (!qrk_str_push_back(&url->username, base->username.base, base->username.len))
                        goto fail;

                    url->password.len = 0;
                    if (!qrk_str_push_back(&url->password, base->password.base, base->password.len))
                        goto fail;

                    if (qrk_url_host_clone(&url->host, &base->host))
                        goto fail;

                    url->port = base->port;

                    parser->state = QRK_URL_PARSER_STATE_PATH;
                    c--;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_SPECIAL_AUTHORITY_SLASHES:
            {
                if (*c == '/' && c < cend && c[1] == '/')
                {
                    parser->state = QRK_URL_PARSER_STATE_SPECIAL_AUTHORITY_IGNORE_SLASHES;
                    c++;
                }
                else
                {
                    url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    parser->state = QRK_URL_PARSER_STATE_SPECIAL_AUTHORITY_IGNORE_SLASHES;
                    c--;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_SPECIAL_AUTHORITY_IGNORE_SLASHES:
            {
                if (*c != '/' && *c != '\\')
                {
                    parser->state = QRK_URL_PARSER_STATE_AUTHORITY;
                    c--;
                }
                else
                {
                    url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_AUTHORITY:
            {
                if (*c == '@')
                {
                    url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;

                    if (atSignSeen) {
                        qrk_str_push_front(&buffer, "%40", 3);
                    }

                    atSignSeen = 1;

                    if (buffer.len > 0 && buffer.base[0] != ':')
                        url->flags |= QRK_URL_FLAG_HAS_USERNAME;

                    for (size_t n = 0; n < buffer.len; n++)
                    {
                        if (buffer.base[n] == ':')
                        {
                            url->flags |= QRK_URL_FLAG_HAS_PASSWORD;
                            passwordTokenSeen = 1;
                            continue;
                        }


                        if (passwordTokenSeen)
                        {
                            if (qrk_url_percent_encode2(&url->password, buffer.base + n, 1, USERINFO_PERCENT_ENCODE_SET))
                                goto fail;
                        }
                        else
                            if (qrk_url_percent_encode2(&url->username, buffer.base + n, 1, USERINFO_PERCENT_ENCODE_SET))
                                goto fail;
                    }

                    buffer.len = 0;
                }
                else if ((c > cend) || *c == '/' || *c == '?' || *c == '#' || ((url->flags & QRK_URL_FLAG_SPECIAL) && *c == '\\'))
                {
                    if (atSignSeen && (buffer.len == 0))
                    {
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                        goto fail;
                    }
                    c = c - (buffer.len + 1);
                    buffer.len = 0;
                    parser->state = QRK_URL_PARSER_STATE_HOST;
                }
                else
                {
                    if (!qrk_str_putc(&buffer, *c))
                        goto fail;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_HOST:
            case QRK_URL_PARSER_STATE_HOSTNAME:
            {
                if (state_override && (url->flags & QRK_URL_FLAG_IS_FILE))
                {
                    c--;
                    parser->state = QRK_URL_PARSER_STATE_FILE_HOST;
                }
                else if (*c == ':' && !insideBrackets)
                {
                    if (buffer.len == 0)
                    {
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                        goto fail;
                    }
                    if (state_override == QRK_URL_PARSER_STATE_HOSTNAME)
                        goto ret;
                    int validation_error = 0;
                    int r = qrk_url_host_parse(&url->host, (qrk_rbuf_t *) &buffer, !(url->flags & QRK_URL_FLAG_SPECIAL), &validation_error);
                    if (r)
                    {
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                        goto fail;
                    }
                    if (validation_error)
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    buffer.len = 0;
                    parser->state = QRK_URL_PARSER_STATE_PORT;
                }
                else if ((c > cend) || *c == '/' || *c == '?' || *c == '#' || ((url->flags & QRK_URL_FLAG_SPECIAL) && *c == '\\'))
                {
                    c--;
                    if ((url->flags & QRK_URL_FLAG_SPECIAL) && buffer.len == 0)
                    {
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                        goto fail;
                    }
                    else if (state_override &&
                             buffer.len == 0 &&
                             ((url->username.len > 0 || url->password.len > 0) || url->port != -1)) {
                        goto ret;
                    }
                    int validation_error = 0;
                    int r = qrk_url_host_parse(&url->host, (qrk_rbuf_t *) &buffer, !(url->flags & QRK_URL_FLAG_SPECIAL), &validation_error);
                    if (r)
                    {
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                        goto fail;
                    }
                    if (validation_error)
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    buffer.len = 0;
                    parser->state = QRK_URL_PARSER_STATE_PATH_START;
                    if (state_override)
                        goto ret;
                }
                else
                {
                    if (*c == '[')
                        insideBrackets = 1;
                    else if (*c == ']')
                        insideBrackets = 0;
                    if (!qrk_str_putc(&buffer, *c))
                        goto fail;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_PORT:
            {
                if (isdigit(*c))
                {
                    if (!qrk_str_putc(&buffer, *c))
                        goto fail;
                }
                else if ((c > cend) || *c == '/' || *c == '?' || *c == '#' || ((url->flags & QRK_URL_FLAG_SPECIAL) && *c == '\\') || state_override)
                {
                    if (buffer.len != 0)
                    {
                        uint32_t port = 0;
                        for (size_t i = 0; i < buffer.len && port <= 0xffff; i++)
                            port = port * 10 + buffer.base[i] - '0';
                        if (port > 0xffff)
                        {
                            url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                            goto fail;
                        }

                        url->port = qrk__url_port_normalize(port, &url->scheme);

                        buffer.len = 0;
                    }
                    if (state_override)
                        goto ret;
                    parser->state = QRK_URL_PARSER_STATE_PATH_START;
                    c--;
                }
                else
                {
                    url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    goto fail;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_FILE:
            {
                url->scheme.len = 0;
                url->flags |= QRK_URL_FLAG_IS_FILE;
                if (!qrk_str_push_back(&url->scheme, "file", 4))
                    goto fail;
                if (*c == '/')
                    parser->state = QRK_URL_PARSER_STATE_FILE_SLASH;
                else if (*c == '\\')
                {
                    url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    parser->state = QRK_URL_PARSER_STATE_FILE_SLASH;
                }
                else if (base && (base->flags & QRK_URL_FLAG_IS_FILE))
                {
                    if (qrk_url_host_clone(&url->host, &base->host))
                        goto fail;

                    url->port = base->port;

                    // clone path
                    if (url->path.nmemb != 0)
                        for (size_t i = 0; i <url->path.nmemb; i++)
                            qrk_str_free((qrk_str_t *)(url->path.base + (i * url->path.memb_size)));
                    url->path.nmemb = 0;
                    for (size_t i = 0; i < base->path.nmemb; i++)
                    {
                        qrk_str_t *path_i = (qrk_str_t *)(base->path.base + (i * base->path.memb_size));
                        qrk_str_t str;
                        if (!qrk_str_malloc(&str, parser->m_ctx, path_i->len))
                            goto fail;

                        if (!qrk_str_push_back(&str, path_i->base, path_i->len))
                        {
                            qrk_str_free(&str);
                            goto fail;
                        }
                        if (!qrk_buf_push_back1(&url->path, &str))
                            goto fail;
                    }
                    if (base->flags & QRK_URL_FLAG_PATH_IS_OPAQUE)
                        url->flags |= QRK_URL_FLAG_PATH_IS_OPAQUE;
                    else
                        url->flags &= ~QRK_URL_FLAG_PATH_IS_OPAQUE;

                    url->query.len = 0;
                    if (!qrk_str_push_back(&url->query, base->query.base, base->query.len))
                        goto fail;

                    if (*c == '?')
                    {
                        url->query.len = 0;
                        parser->state = QRK_URL_PARSER_STATE_QUERY;
                    }
                    else if (*c == '#')
                    {
                        url->fragment.len = 0;
                        parser->state = QRK_URL_PARSER_STATE_FRAGMENT;
                    }
                    else if (c <= cend)
                    {
                        url->query.len = 0;
                        if (!qrk_url_path_starts_windows_drive_letter(c, cend))
                        {
                            if (qrk_url_path_shorten(url))
                                goto fail;
                        }
                        else
                        {
                            url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                            if (url->path.nmemb != 0)
                                for (size_t i = 0; i <url->path.nmemb; i++)
                                    qrk_str_free((qrk_str_t *)(url->path.base + (i * url->path.memb_size)));
                            url->path.nmemb = 0;
                        }
                        parser->state = QRK_URL_PARSER_STATE_PATH;
                        c--;
                    }
                }
                else
                {
                    parser->state = QRK_URL_PARSER_STATE_PATH;
                    c--;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_FILE_SLASH:
            {
                if (*c == '/')
                {
                    parser->state = QRK_URL_PARSER_STATE_FILE_HOST;
                }
                else if (*c == '\\')
                {
                    url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    parser->state = QRK_URL_PARSER_STATE_FILE_HOST;
                }
                else
                {
                    if (base && (base->flags & QRK_URL_FLAG_IS_FILE))
                    {
                        if (qrk_url_host_clone(&url->host, &base->host))
                            goto fail;

                        if (!qrk_url_path_starts_windows_drive_letter(c, cend) &&
                            base->path.nmemb >= 1)
                        {
                            qrk_str_t *path0 = ((qrk_str_t *)qrk_buf_get(&base->path, 0));
                            if (path0->len == 2 && isalpha(path0->base[0]) && path0->base[1] == ':')
                            {
                                qrk_str_t str;
                                if (!qrk_str_malloc(&str, parser->m_ctx, path0->len))
                                    goto fail;

                                if (!qrk_str_push_back(&str, path0->base, path0->len))
                                {
                                    qrk_str_free(&str);
                                    goto fail;
                                }
                                if (!qrk_buf_push_back1(&url->path, &str))
                                    goto fail;
                            }
                        }
                    }
                    parser->state = QRK_URL_PARSER_STATE_PATH;
                    c--;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_FILE_HOST:
            {
                if ((c > cend) || *c == '/' || *c == '\\' || *c == '?' || *c == '#')
                {
                    c--;
                    if (!state_override && buffer.len == 2 && isalpha(buffer.base[0]) && (buffer.base[1] == ':' || buffer.base[1] == '|'))
                    {
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                        parser->state = QRK_URL_PARSER_STATE_PATH;
                    }
                    else if (buffer.len == 0)
                    {
                        url->host.type = QRK_URL_HOST_EMPTY;
                        url->host.str.len = 0;
                        if (state_override)
                            goto ret;
                        parser->state = QRK_URL_PARSER_STATE_PATH_START;
                    }
                    else
                    {
                        int validation_error = 0;
                        int r = qrk_url_host_parse(&url->host, (qrk_rbuf_t *) &buffer, !(url->flags & QRK_URL_FLAG_SPECIAL), &validation_error);
                        if (r)
                        {
                            url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                            goto fail;
                        }
                        if (validation_error)
                            url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                        if (url->host.type == QRK_URL_HOST_OPAQUE || url->host.type == QRK_URL_HOST_DOMAIN)
                        {
                            if (url->host.str.len == 9 && !memcmp(url->host.str.base, "localhost", 9))
                            {
                                url->host.type = QRK_URL_HOST_EMPTY;
                                url->host.str.len = 0;
                            }
                        }
                        if (state_override)
                            goto ret;
                        parser->state = QRK_URL_PARSER_STATE_PATH_START;
                        buffer.len = 0;
                    }
                }
                else
                {
                    if (!qrk_str_putc(&url->host.str, *c))
                        goto fail;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_PATH_START:
            {
                if (url->flags & QRK_URL_FLAG_SPECIAL)
                {
                    if (*c == '\\')
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;

                    parser->state = QRK_URL_PARSER_STATE_PATH;
                    if (!(*c == '/' || *c == '\\'))
                        c--;
                }
                else if (!state_override && *c == '?')
                {
                    url->query.len = 0;
                    parser->state = QRK_URL_PARSER_STATE_QUERY;
                }
                else if (!state_override && *c == '#')
                {
                    url->fragment.len = 0;
                    parser->state = QRK_URL_PARSER_STATE_FRAGMENT;
                }
                else if (c <= cend)
                {
                    parser->state = QRK_URL_PARSER_STATE_PATH;
                    if (*c != '/')
                        c--;
                }
                else if (state_override && url->host.type == QRK_URL_HOST_NULL)
                {
                    qrk_str_t str;
                    if (!qrk_str_malloc(&str, parser->m_ctx, 1))
                        goto fail;
                    if (!qrk_buf_push_back1(&url->path, &str))
                        goto fail;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_PATH:
            {
                if ((c > cend || *c == '/') ||
                    ((url->flags & QRK_URL_FLAG_SPECIAL) && *c == '\\') ||
                    (!state_override && (*c == '?' || *c == '#')))
                {
                    if ((url->flags & QRK_URL_FLAG_SPECIAL) && *c == '\\')
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    if ((buffer.len == 2 && !memcmp(buffer.base, "..", 2)) ||
                        (buffer.len == 4 && (!strncasecmp(buffer.base, ".%2e", 4) || !strncasecmp(buffer.base, "%2e.", 4))) ||
                        (buffer.len == 6 && !strncasecmp(buffer.base, "%2e%2e", 6)))
                    {
                        if (qrk_url_path_shorten(url))
                            goto fail;
                        if (!(*c == '/' || ((url->flags & QRK_URL_FLAG_SPECIAL) && *c == '\\')))
                        {
                            qrk_str_t str;
                            if (!qrk_str_malloc(&str, parser->m_ctx, 1))
                                goto fail;
                            if (!qrk_buf_push_back1(&url->path, &str))
                                goto fail;
                        }
                    }
                    else if ((buffer.len == 1 && buffer.base[0] == '.') ||
                            (buffer.len == 3 && !strncasecmp(buffer.base, "%2e", 3)))
                    {
                        if (!(*c == '/' || ((url->flags & QRK_URL_FLAG_SPECIAL) && *c == '\\')))
                        {
                            qrk_str_t str;
                            if (!qrk_str_malloc(&str, parser->m_ctx, 1))
                                goto fail;
                            if (!qrk_buf_push_back1(&url->path, &str))
                                goto fail;
                        }
                    }
                    else
                    {
                        if ((url->flags & QRK_URL_FLAG_IS_FILE) && url->path.nmemb == 0 && buffer.len == 2 &&
                            (isalpha(buffer.base[0]) && (buffer.base[1] == ':' || buffer.base[1] == '|')))
                        {
                            buffer.base[1] = ':';
                        }
                        // TODO: this is how all of the copies should be, with memcpy not push_back since malloc already assures memory is allocated
                        qrk_str_t str;
                        if (!qrk_str_malloc(&str, parser->m_ctx, buffer.len))
                            goto fail;
                        memcpy(str.base, buffer.base, buffer.len);
                        str.len = buffer.len;
                        if (!qrk_buf_push_back1(&url->path, &str))
                            goto fail;

                    }
                    buffer.len = 0;
                    if (*c == '?')
                    {
                        url->query.len = 0;
                        parser->state = QRK_URL_PARSER_STATE_QUERY;
                    }
                    else if (*c == '#')
                    {
                        url->fragment.len = 0;
                        parser->state = QRK_URL_PARSER_STATE_FRAGMENT;
                    }
                }
                else
                {
                    // TODO: If c is not a URL code point and not U+0025 (%), validation error.
                    // XXX: We're not validating URL code points. See above.
                    if (*c == '%' && (cend - c >= 2) && !(isxdigit(c[1]) && isxdigit(c[2])))
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;

                    if (qrk_url_percent_encode2(&buffer, c, 1, PATH_PERCENT_ENCODE_SET))
                        goto fail;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_OPAQUE_PATH:
            {
                if (*c == '?')
                {
                    url->query.len = 0;
                    parser->state = QRK_URL_PARSER_STATE_QUERY;
                }
                else if (*c == '#')
                {
                    url->fragment.len = 0;
                    parser->state = QRK_URL_PARSER_STATE_FRAGMENT;
                }
                else
                {
                    // TODO: If c is not the EOF code point, not a URL code point, and not U+0025 (%), validation error.
                    // XXX: We're not validating URL code points. See above.
                    if (*c == '%' && (cend - c >= 2) && !(isxdigit(c[1]) && isxdigit(c[2])))
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    if ((c <= cend))
                    {
                        if (url->path.nmemb == 0)
                        {
                            qrk_str_t str;
                            if (!qrk_str_malloc(&str, parser->m_ctx, 1))
                                goto fail;
                            if (!qrk_buf_push_back1(&url->path, &str))
                                goto fail;
                        }
                        qrk_str_t *str = (qrk_str_t *)url->path.base;
                        if (qrk_url_percent_encode2(str, c, 1, C0_CONTROL_PERCENT_ENCODE_SET))
                            goto fail;
                    }
                }
            }
            break;

            case QRK_URL_PARSER_STATE_QUERY:
            {
                // XXX: We assume encoding is UTF-8.
                if ((!state_override && *c == '#') || (c > cend))
                {
                    if (qrk_url_percent_encode2(&url->query, buffer.base, buffer.len,
                                                (url->flags & QRK_URL_FLAG_SPECIAL) ?
                                                        SPECIAL_QUERY_PERCENT_ENCODE_SET :
                                                        QUERY_PERCENT_ENCODE_SET)
                    )
                        goto fail;
                    buffer.len = 0;
                    if (*c == '#')
                    {
                        url->fragment.len = 0;
                        parser->state = QRK_URL_PARSER_STATE_FRAGMENT;
                    }
                }
                else if (c <= cend)
                {
                    // TODO: If c is not a URL code point and not U+0025 (%), validation error.
                    // XXX: We're not validating URL code points. See above.
                    if (*c == '%' && (cend - c >= 2) && !(isxdigit(c[1]) && isxdigit(c[2])))
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    if (!qrk_str_putc(&buffer, *c))
                        goto fail;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_FRAGMENT:
            {
                if (c <= cend)
                {
                    // TODO: If c is not a URL code point and not U+0025 (%), validation error.
                    // XXX: We're not validating URL code points. See above.
                    if (*c == '%' && (cend - c >= 2) && !(isxdigit(c[1]) && isxdigit(c[2])))
                        url->flags |= QRK_URL_FLAG_VALIDATION_ERROR;
                    if (qrk_url_percent_encode2(&url->fragment, c, 1, FRAGMENT_PERCENT_ENCODE_SET))
                        goto fail;
                }
            }
            break;

            case QRK_URL_PARSER_STATE_NO_OVERRIDE:
                break;
        }
        c++;
    }

ret:
    qrk_str_free(&buffer);
    qrk_str_free(&input);
    return 0;
fail:
    qrk_str_free(&buffer);
fail1:
    qrk_str_free(&input);
    return 1;
}

int qrk_url_serialize_path (qrk_url_t *url, qrk_str_t *str)
{
    if (url->flags & QRK_URL_FLAG_PATH_IS_OPAQUE)
    {
        qrk_str_t *path = (qrk_str_t *) qrk_buf_get(&url->path, 0);
        if (!qrk_str_push_back(str, path->base, path->len))
            return 1;
    }
    else
    {
        for (size_t i = 0; i < url->path.nmemb; i++)
        {
            qrk_str_t *path_i = (qrk_str_t *) qrk_buf_get(&url->path, i);
            if (!qrk_str_putc(str, '/'))
                return 1;
            if (!qrk_str_push_back(str, path_i->base, path_i->len))
                return 1;
        }
    }
    return 0;
}

int qrk_url_serialize (qrk_url_t *url, qrk_str_t *str, int exclude_fragment)
{
    if (!qrk_str_push_back(str, url->scheme.base, url->scheme.len))
        return 1;
    if (!qrk_str_putc(str, ':'))
        return 1;

    if (url->host.type != QRK_URL_HOST_NULL)
    {
        if (!qrk_str_push_back(str, "//", 2))
            return 1;

        if (url->flags & QRK_URL_FLAG_HAS_USERNAME)
        {
            if (!qrk_str_push_back(str, url->username.base, url->username.len))
                return 1;
            if (url->flags & QRK_URL_FLAG_HAS_PASSWORD)
            {
                if (!qrk_str_putc(str, ':'))
                    return 1;
                if (!qrk_str_push_back(str, url->password.base, url->password.len))
                    return 1;
            }
            if (!qrk_str_putc(str, '@'))
                return 1;
        }

        if (qrk_url_host_serialize(str, &url->host))
            return 1;

        if (url->port != -1)
        {
            if (!qrk_str_putc(str, ':'))
                return 1;
            if (!qrk_str_printf(str, "%d", url->port))
                return 1;
        }
    }

    if (url->host.type == QRK_URL_HOST_NULL && !(url->flags & QRK_URL_FLAG_PATH_IS_OPAQUE) &&
        url->path.nmemb > 1 && ((qrk_str_t *) qrk_buf_get(&url->path, 0))->len == 0)
    {
        if (!qrk_str_push_back(str, "/.", 2))
            return 1;
    }

    if (qrk_url_serialize_path(url, str))
        return 1;

    if (url->query.len > 0)
    {
        if (!qrk_str_putc(str, '?'))
            return 1;
        if (!qrk_str_push_back(str, url->query.base, url->query.len))
            return 1;
    }

    if (!exclude_fragment)
    {
        if (url->fragment.len > 0)
        {
            if (!qrk_str_putc(str, '#'))
                return 1;
            if (!qrk_str_push_back(str, url->fragment.base, url->fragment.len))
                return 1;
        }
    }

    return 0;
}

int qrk_html_origin_tuple_init (qrk_html_origin_tuple_t *tuple, qrk_malloc_ctx_t *mctx)
{
    memset(tuple, 0, sizeof(qrk_html_origin_tuple_t));
    if (!qrk_str_malloc(&tuple->scheme, mctx, 16))
        return 1;
    if (qrk_url_host_init(&tuple->host, mctx))
    {
        qrk_str_free(&tuple->scheme);
        return 1;
    }
    if (!qrk_str_malloc(&tuple->domain, mctx, 1))
    {
        qrk_str_free(&tuple->scheme);
        qrk_url_host_free(&tuple->host);
        return 1;
    }
    return 0;
}

void qrk_html_origin_tuple_free(qrk_html_origin_tuple_t *tuple)
{
    qrk_str_free(&tuple->scheme);
    qrk_str_free(&tuple->domain);
    qrk_url_host_free(&tuple->host);
}

int qrk_html_origin_serialize (qrk_str_t *str, qrk_html_origin_tuple_t *tuple)
{
    if (tuple == NULL)
    {
        if (!qrk_str_push_back(str, "null", 4))
            return 1;
        return 0;
    }

    if (!qrk_str_push_back(str, tuple->scheme.base, tuple->scheme.len))
        return 1;

    if (!qrk_str_push_back(str, "://", 3))
        return 1;

    if (qrk_url_host_serialize(str, &tuple->host))
        return 1;

    if (tuple->port != -1)
    {
        if (!qrk_str_printf(str, ":%d", tuple->port))
            return 1;
    }

    return 0;
}

int qrk_url_origin (qrk_url_t *url, qrk_html_origin_tuple_t **origin, qrk_malloc_ctx_t *mctx)
{
    if (url->flags & QRK_URL_FLAG_IS_FILE)
    {
        *origin = NULL;
    }
    else if (url->flags & QRK_URL_FLAG_SPECIAL)
    {
        *origin = qrk_malloc(mctx, sizeof(qrk_html_origin_tuple_t));
        if (!*origin)
            return 1;
        if (qrk_html_origin_tuple_init(*origin, mctx))
        {
fail_special:
            qrk_free(mctx, *origin);
            *origin = NULL;
            return 1;
        }
        if (qrk_url_host_clone(&(*origin)->host, &url->host))
            goto fail_special;
        if (!qrk_str_push_back(&(*origin)->scheme, url->scheme.base, url->scheme.len))
        {
            qrk_url_host_free(&(*origin)->host);
            goto fail_special;
        }
        (*origin)->port = url->port;
    }
    else if (url->scheme.len == 4 && !memcmp(url->scheme.base, "blob", 4))
    {
        // TODO: If url’s blob URL entry is non-null, then return url’s blob URL entry’s environment’s origin.
        qrk_str_t path;

        if (!qrk_str_malloc(&path, mctx, 1))
            return 1;

        if (qrk_url_serialize_path(url, &path))
        {
            qrk_str_free(&path);
            return 1;
        }

        qrk_url_parser_t parser;

        if (qrk_url_parser_init(&parser, mctx))
        {
            qrk_str_free(&path);
            return 1;
        }

        qrk_url_t *_url = NULL;

        if (qrk_url_parse_basic(&parser, (qrk_rbuf_t *) &path, NULL, &_url, QRK_URL_PARSER_STATE_NO_OVERRIDE))
        {
            if (_url != NULL)
            {
                qrk_url_free(_url);
                qrk_free(mctx, _url);
            }
            qrk_str_free(&path);
            *origin = NULL;
            return 0;
        }

        int r = qrk_url_origin(_url, origin, mctx);

        if (r)
        {
            qrk_url_free(_url);
            qrk_free(mctx, _url);
            qrk_str_free(&path);
            return 1;
        }

        qrk_url_free(_url);
        qrk_free(mctx, _url);
        qrk_str_free(&path);
    }
    else
    {
        *origin = NULL;
    }

    return 0;
}

int qrk_url_parser_init (qrk_url_parser_t *parser, qrk_malloc_ctx_t *ctx)
{
    memset(parser, 0, sizeof(qrk_url_parser_t));

    parser->m_ctx = ctx;

    return 0;
}

void qrk_url_free (qrk_url_t *url)
{
    qrk_str_free(&url->scheme);
    qrk_str_free(&url->username);
    qrk_str_free(&url->password);
    qrk_str_free(&url->query);
    qrk_str_free(&url->fragment);

    qrk_url_host_free(&url->host);

    for (size_t i = 0; i < url->path.nmemb; i++)
        qrk_str_free((qrk_str_t *)(url->path.base + (i * url->path.memb_size)));

    qrk_buf_free(&url->path);
}

int qrk_url_form_urlencoded_parse (qrk_rbuf_t *src, qrk_buf_t *list, qrk_malloc_ctx_t *mctx)
{
    if (!qrk_buf_malloc(list, mctx, sizeof(qrk_kv_t), 1))
        return 1;

    char *start = src->base;
    int is_value = 0;

    char *c = src->base;
    char *end = src->base + src->len;

    while (c <= end)
    {
        const char ch = *c;

        if (ch == '&' || c >= end)
        {
            if (is_value)
            {
                // get last element in the list
                qrk_kv_t *kv = qrk_buf_get(list, list->nmemb - 1);
                // set the value
                qrk_rbuf_t value = {
                        .base = start,
                        .len = c - start
                };
                if (qrk_url_percent_decode2(&kv->value, &value, 1))
                    return 1;
            }
            else
            {
                qrk_kv_t kv;
                if (!qrk_str_malloc(&kv.key, mctx, c - start))
                    return 1;
                if (!qrk_str_malloc(&kv.value, mctx, 64))
                {
                    qrk_str_free(&kv.key);
                    return 1;
                }

                qrk_rbuf_t key = {
                        .base = start,
                        .len = c - start
                };

                if (qrk_url_percent_decode2(&kv.key, &key, 1))
                {
                    qrk_str_free(&kv.key);
                    qrk_str_free(&kv.value);
                    return 1;
                }
                if (!qrk_buf_push_back1(list, &kv))
                {
                    qrk_str_free(&kv.key);
                    qrk_str_free(&kv.value);
                    return 1;
                }
            }

            is_value = 0;
            start = c + 1;
        }

        if (c >= end)
            break;

        if (ch == '=' && !is_value)
        {
            is_value = 1;
            qrk_kv_t kv;
            if (!qrk_str_malloc(&kv.key, mctx, c - start))
                return 1;
            if (!qrk_str_malloc(&kv.value, mctx, 64))
            {
                qrk_str_free(&kv.key);
                return 1;
            }

            qrk_rbuf_t key = {
                .base = start,
                .len = c - start
            };

            if (qrk_url_percent_decode2(&kv.key, &key, 1))
            {
                qrk_str_free(&kv.key);
                qrk_str_free(&kv.value);
                return 1;
            }
            if (!qrk_buf_push_back1(list, &kv))
            {
                qrk_str_free(&kv.key);
                qrk_str_free(&kv.value);
                return 1;
            }
            start = c + 1;
        }

        c++;
    }

    return 0;
}
