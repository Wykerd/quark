#include <quark/url/url.h>
#include <quark/std/utils.h>
#include <ctype.h>
#include <stdlib.h>
#include <unicode/utypes.h>
#include <unicode/uidna.h>
#include <math.h>
#include <uv.h>

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

void qrk_url_percent_decode (qrk_str_t *dest, qrk_rbuf_t *src)
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
            qrk_str_push_back(dest, &ch, 1);
        }
        else
            qrk_str_push_back(dest, &src->base[i], 1);
    }
}

void qrk_url_percent_encode (qrk_str_t *dest, qrk_rbuf_t *src, const uint8_t *set)
{
    for (size_t i = 0; i < src->len; i++)
    {
        if (set[src->base[i]])
            qrk_str_push_back(dest, PERCENT_ENCODING + src->base[i] * 4, 3);
        else
            qrk_str_push_back(dest, &src->base[i], 1);
    }
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

    int output_length = uidna_nameToASCII(idna, (const UChar *) src->base, src->len, (UChar *) dest->base, dest->size, &info, &err);

    if (err == U_BUFFER_OVERFLOW_ERROR)
    {
        err = U_ZERO_ERROR;
        qrk_str_resize(dest, output_length);
        output_length = uidna_nameToASCII(idna, (const UChar *) src->base, src->len, (UChar *) dest->base, dest->size, &info, &err);
    };

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
    };

    if (U_FAILURE(err))
        dest->len = 0;
    else
        dest->len = output_length;

    uidna_close(idna);

    return 0;
}

int qrk_url_domain_validate (qrk_rbuf_t *domain)
{
    /*
    int rc;
    char *p;

    rc = idn2_to_ascii_8z(domain->base, &p, IDN2_NONTRANSITIONAL | IDN2_BIDI | IDN2_CONTEXTJ);
    if (rc != IDNA_SUCCESS)
        return rc;

    idn2_free(p);

    rc = idn2_to_unicode_8z8z(domain->base, &p, IDN2_NONTRANSITIONAL | IDN2_BIDI | IDN2_CONTEXTJ | IDN2_USE_STD3_ASCII_RULES);
    if (rc != IDNA_SUCCESS)
        return rc;

    idn2_free(p);

    return 0; */
    return 1;
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
    const char *end = src->base + src->len;

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

    // XXX: we do not validate that %-encoded characters are valid.
    // XXX: we do not validate that characters are URL code points.
    for (size_t i = 0; i < src->len; i++)
        if (src->base[i] != '%' && FORBIDDEN_HOST_CODE_POINT[src->base[i]])
        {
            *validation_error = 1;
            return 1;
        }

    qrk_url_percent_encode(dest, src, C0_CONTROL_PERCENT_ENCODE_SET);

    return 0;
}

// TODO: good error codes
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
    };
    if (is_not_special)
    {
        dest->type = QRK_URL_HOST_OPAQUE;
        return qrk_url_host_opaque_parse(&dest->str, src, validation_error);
    }
    qrk_str_t domain;
    if (qrk_str_malloc(&domain, dest->str.m_ctx, src->len) == NULL)
        return 1;
    qrk_url_percent_decode(&domain, src);
    qrk_str_t asciiDomain;
    if (qrk_str_malloc(&asciiDomain, dest->str.m_ctx, domain.len) == NULL)
        goto fail1;
    if (qrk_url_domain_to_ascii(&asciiDomain, (qrk_rbuf_t *) &domain, 0))
        goto fail;
    for (size_t i = 0; i < asciiDomain.len; i++)
        if (FORBIDDEN_HOST_CODE_POINT[asciiDomain.base[i]])
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

    qrk_str_free(&asciiDomain);
    qrk_str_free(&domain);
    return 0;
fail:
    qrk_str_free(&asciiDomain);
fail1:
    qrk_str_free(&domain);
    return 1;
}
