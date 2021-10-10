#include <quark/std/utils.h>
#include <string.h>

#include <quark/no_malloc.h>

/**
 * Credit:
 * Markus Kuhn <http://www.cl.cam.ac.uk/~mgk25/> -- 2005-03-30
 * License: http://www.cl.cam.ac.uk/~mgk25/short-license.html
 * Available: https://www.cl.cam.ac.uk/~mgk25/ucs/utf8_check.c
 */
const char *qrk_utf8_check(const char *s, size_t len)
{
    size_t i = 0;
    while (i < len)
    {
        size_t j = i + 8;

        if (j <= len)
        {
            //
            // Read 8 bytes and check if they are ASCII.
            //
            uint64_t chunk;
            memcpy(&chunk, s + i, 8);

            if ((chunk & 0x8080808080808080) == 0x00)
            {
                i = j;
                continue;
            }
        }

        while ((s[i] & 0x80) == 0x00)
        { // 0xxxxxxx
            if (++i == len)
            {
                return NULL;
            }
        }

        if ((s[i] & 0xe0) == 0xc0)
        {
            /* 110XXXXx 10xxxxxx */
            if (i + 1 == len ||
                (s[i + 1] & 0xc0) != 0x80 ||
                (s[i] & 0xfe) == 0xc0) /* overlong? */
                return s + 1;
            else
                i += 2;
        }
        else if ((s[i] & 0xf0) == 0xe0)
        {
            /* 1110XXXX 10Xxxxxx 10xxxxxx */
            if (i + 2 >= len ||
                (s[i + 1] & 0xc0) != 0x80 ||
                (s[i + 2] & 0xc0) != 0x80 ||
                (s[i] == 0xe0 && (s[i + 1] & 0xe0) == 0x80) || /* overlong? */
                (s[i] == 0xed && (s[i + 1] & 0xe0) == 0xa0) || /* surrogate? */
                (s[i] == 0xef && s[i + 1] == 0xbf &&
                    (s[i + 2] & 0xfe) == 0xbe)) /* U+FFFE or U+FFFF? */
                return s + i;
            else
                i += 3;
        }
        else if ((s[i] & 0xf8) == 0xf0)
        {
            /* 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx */
            if (i + 3 >= len ||
                (s[i + 1] & 0xc0) != 0x80 ||
                (s[i + 2] & 0xc0) != 0x80 ||
                (s[i + 3] & 0xc0) != 0x80 ||
                (s[i] == 0xf0 && (s[i + 1] & 0xf0) == 0x80) ||    /* overlong? */
                (s[i] == 0xf4 && s[i + 1] > 0x8f) || s[i] > 0xf4) /* > U+10FFFF? */
                return s + i;
            else
                i += 4;
        }
        else
            return s + i;
    }

    return NULL;
}
