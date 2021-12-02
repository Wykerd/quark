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

// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

static const uint8_t utf8d[] = {
        // The first part of the table maps bytes to character classes that
        // to reduce the size of the transition table and create bitmasks.
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
        10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

        // The second part is a transition table that maps a combination
        // of a state of the automaton and a character class to a state.
        0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
        12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
        12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
        12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
        12,36,12,12,12,12,12,12,12,12,12,12,
};

uint32_t qrk_utf8_decode(uint32_t* state, uint32_t* codep, uint32_t byte) {
    uint32_t type = utf8d[byte];

    *codep = (*state != UTF8_ACCEPT) ?
             (byte & 0x3fu) | (*codep << 6) :
             (0xff >> type) & (byte);

    *state = utf8d[256 + *state + type];
    return *state;
}

int qrk_utf8_isascii (const char *s)
{
    uint32_t codepoint;
    uint32_t state = 0;

    for (; *s; ++s)
        if (!qrk_utf8_decode(&state, &codepoint, *s))
            if (codepoint > 127)
                return 1;

    if (state != UTF8_ACCEPT)
        return 2;

    return 0;
}

int qrk_infra_split_strict_static (qrk_rbuf_t *string, uint32_t delimiter, size_t *tokens, size_t max_tokens)
{
    const char *s = string->base;
    const char *end = string->base + string->len;
    uint32_t codepoint;
    uint32_t state = 0;
    int token = 0;

    for (; *s; ++s)
        if (s >= end)
        {
            break;
        }
        else if (!qrk_utf8_decode(&state, &codepoint, *s))
            if (codepoint == delimiter)
            {
                if (++token > max_tokens)
                    return -1;
                tokens[token - 1] = s - string->base; // index of delimiter
            }

    if (++token > max_tokens)
        return -1;
    tokens[token - 1] = -1;

    if (state != UTF8_ACCEPT)
        return -1;

    return token;
}
