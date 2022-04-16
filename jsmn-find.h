#ifndef JSMN_FIND_H
#define JSMN_FIND_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef JSMN_H
#error "jsmn-find.h should be included after jsmn.h"
#endif

/** @brief Internally used sized-buffer */
struct _jsmnf_szbuf {
    /** buffer's contents */
    const char *contents;
    /** buffer's length */
    int length;
};

/** @brief JSON object */
typedef struct jsmnf_pair {
    /** amount of children currently filled in */
    int length;
    /** children threshold capacity */
    int capacity;
    /** this pair's children */
    struct jsmnf_pair *buckets;

    /** the key of the pair */
    struct _jsmnf_szbuf key;
    /** the value of the pair */
    struct _jsmnf_szbuf value;
    /** current state of this pair */
    int state;

    /** JSON type @see `jsmntype_t` at jsmn.h */
    jsmntype_t type;
} jsmnf_pair;

/** @brief Bucket (@ref jsmnf_pair) loader, keeps track of pair array
 *      position */
typedef struct jsmnf_loader {
    /** next pair to allocate */
    unsigned int pairnext;
} jsmnf_loader;

/**
 * @brief Initialize a @ref jsmnf_loader
 *
 * @param loader jsmnf_loader to be initialized
 */
JSMN_API void jsmnf_init(jsmnf_loader *loader);

/**
 * @brief Populate the @ref jsmnf_pair pairs with jsmn tokens
 *
 * @param loader the @ref jsmnf_loader initialized with jsmnf_init()
 * @param js the raw JSON string
 * @param tokens jsmn tokens initialized with jsmn_parse()
 * @param num_tokens amount of tokens initialized with jsmn_parse()
 * @param pairs jsmnf_pair pairs array
 * @param num_pairs maximum amount of pairs provided
 * @return a `enum jsmnerr` value for error or the number of tokens found
 */
JSMN_API int jsmnf_load(jsmnf_loader *loader,
                        const char js[],
                        const jsmntok_t tokens[],
                        unsigned int num_tokens,
                        jsmnf_pair pairs[],
                        unsigned int num_pairs);

/**
 * @brief Find a @ref jsmnf_pair token by its associated key
 *
 * @param head a @ref jsmnf_pair object or array loaded at jsmnf_start()
 * @param key the key too be matched
 * @param length length of the key too be matched
 * @return the @ref jsmnf_pair `head`'s field matched to `key`, or NULL if
 * not encountered
 */
JSMN_API jsmnf_pair *jsmnf_find(jsmnf_pair *head,
                                const char key[],
                                int length);

/**
 * @brief Find a @ref jsmnf_pair token by its full key path
 *
 * @param head a @ref jsmnf_pair object or array loaded at jsmnf_start()
 * @param path an array of key path strings, from least to highest depth
 * @param depth the depth level of the last `path` key
 * @return the @ref jsmnf_pair `head`'s field matched to `path`, or NULL if
 * not encountered
 */
JSMN_API jsmnf_pair *jsmnf_find_path(jsmnf_pair *head,
                                     char *const path[],
                                     int depth);
/**
 * @brief Utility function for unescaping a Unicode string
 *
 * @param p_dest destination buffer
 * @param src source string to be unescaped
 * @param length source string length
 * @return length of unescaped string if successful, 0 otherwise
 */
JSMN_API size_t jsmnf_unescape(char **p_dest, const char src[], size_t length);

#ifndef JSMN_HEADER

#include "chash.h"

#define _jsmnf_key_hash(key, hash)                                            \
    5031;                                                                     \
    do {                                                                      \
        int __CHASH_HINDEX;                                                   \
        for (__CHASH_HINDEX = 0; __CHASH_HINDEX < (key).length;               \
             ++__CHASH_HINDEX) {                                              \
            (hash) =                                                          \
                (((hash) << 1) + (hash)) + (key).contents[__CHASH_HINDEX];    \
        }                                                                     \
    } while (0)

/* compare jsmnf keys */
#define _jsmnf_key_compare(cmp_a, cmp_b)                                      \
    (!strncmp((cmp_a).contents, (cmp_b).contents, (cmp_b).length))

#define _JSMNF_TABLE_HEAP   0
#define _JSMNF_TABLE_BUCKET struct jsmnf_pair
#define _JSMNF_TABLE_FREE_KEY(key)
#define _JSMNF_TABLE_HASH(key, hash) _jsmnf_key_hash(key, hash)
#define _JSMNF_TABLE_FREE_VALUE(value)
#define _JSMNF_TABLE_COMPARE(cmp_a, cmp_b) _jsmnf_key_compare(cmp_a, cmp_b)

JSMN_API void
jsmnf_init(jsmnf_loader *loader)
{
    loader->pairnext = 0;
}

static int
_jsmnf_get_pairs(struct jsmnf_loader *loader,
                 struct jsmnf_pair *curr,
                 const char js[],
                 const struct jsmntok *tok,
                 unsigned int num_tokens,
                 struct jsmnf_pair *pairs,
                 unsigned int num_pairs)
{
    int offset = 0;

    if (!num_tokens) return 0;

    switch (tok->type) {
    case JSMN_OBJECT: {
        int pairstart = loader->pairnext;
        int ret;

        if (tok->size > (int)(num_pairs - pairstart)
            || (loader->pairnext + 1 + (tok->size * 1.3))
                   > (num_pairs - pairstart))
        {
            return JSMN_ERROR_NOMEM;
        }

        loader->pairnext += 1 + (tok->size * 1.3);

        (void)chash_init_stack(curr, pairs + pairstart,
                               loader->pairnext - pairstart, _JSMNF_TABLE);

        while (curr->length < tok->size) {
            const struct jsmntok *_key = tok + 1 + offset;
            struct jsmnf_pair *found = NULL;
            struct _jsmnf_szbuf key, value = { 0 };

            key.contents = js + _key->start;
            key.length = _key->end - _key->start;

            /* skip Key token */
            offset += 1;

            /* key->length > 0 means we're dealing with an Object or Array */
            if (_key->size > 0) {
                const struct jsmntok *_value = tok + 1 + offset;

                value.contents = js + _value->start;
                value.length = _value->end - _value->start;

                chash_assign(curr, key, value, _JSMNF_TABLE);
                (void)chash_lookup_bucket(curr, key, found, _JSMNF_TABLE);

                ret = _jsmnf_get_pairs(loader, found, js, _value,
                                       num_tokens - offset, pairs, num_pairs);
                if (ret < 0) return ret;

                offset += ret;
            }
            else {
                chash_assign(curr, key, value, _JSMNF_TABLE);
                (void)chash_lookup_bucket(curr, key, found, _JSMNF_TABLE);
            }
        }
    } break;
    case JSMN_ARRAY: {
        int pairstart = loader->pairnext;
        int ret;

        if (tok->size > (int)(num_pairs - pairstart)
            || (loader->pairnext + 1 + (tok->size * 1.3))
                   > (num_pairs - pairstart))
            return JSMN_ERROR_NOMEM;

        loader->pairnext += 1 + (tok->size * 1.3);

        (void)chash_init_stack(curr, pairs + pairstart,
                               loader->pairnext - pairstart, NULL);

        for (; curr->length < tok->size; ++curr->length) {
            const struct jsmntok *_value = tok + 1 + offset;
            struct jsmnf_pair *pair = curr->buckets + curr->length;
            struct _jsmnf_szbuf value;

            value.contents = js + _value->start;
            value.length = _value->end - _value->end;

            ret = _jsmnf_get_pairs(loader, pair, js, _value,
                                   num_tokens - offset, pairs, num_pairs);
            if (ret < 0) return ret;

            offset += ret;

            /* assign array element */
            pair->value = value;
            pair->state = CHASH_FILLED;
            /* unused for array elements */
            pair->key.contents = NULL;
            pair->key.length = 0;
        }
    } break;
    case JSMN_STRING:
    case JSMN_PRIMITIVE:
        break;
    case JSMN_UNDEFINED:
    default:
        fprintf(stderr, "Unexpected key: %.*s\n", tok->end - tok->start,
                js + tok->start);
        return -1;
    }

    curr->type = tok->type;

    return offset + 1;
}

JSMN_API int
jsmnf_load(struct jsmnf_loader *loader,
           const char js[],
           const struct jsmntok tokens[],
           unsigned int num_tokens,
           struct jsmnf_pair pairs[],
           unsigned int num_pairs)
{
    if (!loader->pairnext) {
        pairs->value.contents = js + tokens->start;
        pairs->value.length = tokens->end - tokens->start;
    }
    return _jsmnf_get_pairs(loader, pairs, js, tokens, num_tokens, pairs,
                            num_pairs);
}

JSMN_API struct jsmnf_pair *
jsmnf_find(struct jsmnf_pair *head, const char key[], int length)
{
    struct jsmnf_pair *found = NULL;

    if (!key || !head) return NULL;

    if (JSMN_OBJECT == head->type) {
        struct _jsmnf_szbuf _key;
        int contains;
        _key.contents = key;
        _key.length = length;
        contains = chash_contains(head, _key, contains, _JSMNF_TABLE);
        if (contains) {
            (void)chash_lookup_bucket(head, _key, found, _JSMNF_TABLE);
        }
    }
    else if (JSMN_ARRAY == head->type) {
        char *endptr;
        int idx = (int)strtol(key, &endptr, 10);
        if (endptr != key) found = head->buckets + idx;
    }
    return found;
}

JSMN_API struct jsmnf_pair *
jsmnf_find_path(struct jsmnf_pair *head, char *const path[], int depth)
{
    struct jsmnf_pair *iter = head, *found = NULL;
    int i = 0;
    for (; i < depth; ++i) {
        if (!iter) continue;
        found = jsmnf_find(iter, path[i], strlen(path[i]));
        if (!found) break;
        iter = found;
    }
    return found;
}

static int
_jsmnf_read_4_digits(char **p_str, const char *src_end, unsigned *x)
{
    char *str = *p_str;
    char buf[5] = { 0 };
    unsigned v;
    int i;

    if (src_end - str < 4) return 0;

    for (i = 0; i < 4; i++) {
        buf[i] = str[i];
        if (('0' <= str[i] && str[i] <= '9')
            || ('A' <= str[i] && str[i] <= 'F')
            || ('a' <= str[i] && str[i] <= 'f'))
        {
            continue;
        }
        return 0;
    }

    sscanf(buf, "%x", &v);

    *x = v;
    *p_str = str + 4;

    return 1;
}

#define _JSMNF_UTF16_IS_FIRST_SURROGATE(c)                                    \
    (0xD800 <= (unsigned)c && (unsigned)c <= 0xDBFF)
#define _JSMNF_UTF16_IS_SECOND_SURROGATE(c)                                   \
    (0xDC00 <= (unsigned)c && (unsigned)c <= 0xDFFF)
#define _JSMNF_UTF16_JOIN_SURROGATE(c1, c2)                                   \
    (((((unsigned long)c1 & 0x3FF) << 10) | ((unsigned)c2 & 0x3FF)) + 0x10000)
#define _JSMNF_UTF8_IS_VALID(c)                                               \
    (((unsigned long)c <= 0x10FFFF)                                           \
     && ((unsigned long)c < 0xD800 || (unsigned long)c > 0xDFFF))
#define _JSMNF_UTF8_IS_TRAIL(c) (((unsigned char)c & 0xC0) == 0x80)
#define _JSMNF_UTF_ILLEGAL      0xFFFFFFFFu

static int
_jsmnf_utf8_trail_length(unsigned char c)
{
    if (c < 128) return 0;
    if (c < 194) return -1;
    if (c < 224) return 1;
    if (c < 240) return 2;
    if (c <= 244) return 3;
    return -1;
}

static int
_jsmnf_utf8_width(unsigned long value)
{
    if (value <= 0x7F) return 1;
    if (value <= 0x7FF) return 2;
    if (value <= 0xFFFF) return 3;
    return 4;
}

/* See RFC 3629
   Based on: http://www.w3.org/International/questions/qa-forms-utf-8 */
static unsigned long
_jsmnf_utf8_next(char **p, char *e, int html)
{
    unsigned char lead, tmp;
    int trail_size;
    unsigned long c;

    if (*p == e) return _JSMNF_UTF_ILLEGAL;

    lead = **p;
    (*p)++;

    /* First byte is fully validated here */
    trail_size = _jsmnf_utf8_trail_length(lead);

    if (trail_size < 0) return _JSMNF_UTF_ILLEGAL;

    /* Ok as only ASCII may be of size = 0 also optimize for ASCII text */
    if (trail_size == 0) {
        if (!html || (lead >= 0x20 && lead != 0x7F) || lead == 0x9
            || lead == 0x0A || lead == 0x0D)
            return lead;
        return _JSMNF_UTF_ILLEGAL;
    }

    c = lead & ((1 << (6 - trail_size)) - 1);

    /* Read the rest */
    switch (trail_size) {
    case 3:
        if (*p == e) return _JSMNF_UTF_ILLEGAL;
        tmp = **p;
        (*p)++;
        if (!_JSMNF_UTF8_IS_TRAIL(tmp)) return _JSMNF_UTF_ILLEGAL;
        c = (c << 6) | (tmp & 0x3F);
    /* fall-through */
    case 2:
        if (*p == e) return _JSMNF_UTF_ILLEGAL;
        tmp = **p;
        (*p)++;
        if (!_JSMNF_UTF8_IS_TRAIL(tmp)) return _JSMNF_UTF_ILLEGAL;
        c = (c << 6) | (tmp & 0x3F);
    /* fall-through */
    case 1:
        if (*p == e) return _JSMNF_UTF_ILLEGAL;
        tmp = **p;
        (*p)++;
        if (!_JSMNF_UTF8_IS_TRAIL(tmp)) return _JSMNF_UTF_ILLEGAL;
        c = (c << 6) | (tmp & 0x3F);
    }

    /* Check code point validity: no surrogates and valid range */
    if (!_JSMNF_UTF8_IS_VALID(c)) return _JSMNF_UTF_ILLEGAL;

    /* make sure it is the most compact representation */
    if (_jsmnf_utf8_width(c) != trail_size + 1) return _JSMNF_UTF_ILLEGAL;

    if (html && c < 0xA0) return _JSMNF_UTF_ILLEGAL;
    return c;
}

static int
_jsmnf_utf8_validate(char *p, char *e)
{
    while (p != e)
        if (_jsmnf_utf8_next(&p, e, 0) == _JSMNF_UTF_ILLEGAL) return 0;
    return 1;
}

static void
_jsmnf_utf8_encode(unsigned long value,
                   char utf8_seq[4],
                   unsigned *utf8_seqlen)
{
    /* struct utf8_seq out={0}; */
    if (value <= 0x7F) {
        utf8_seq[0] = value;
        *utf8_seqlen = 1;
    }
    else if (value <= 0x7FF) {
        utf8_seq[0] = (value >> 6) | 0xC0;
        utf8_seq[1] = (value & 0x3F) | 0x80;
        *utf8_seqlen = 2;
    }
    else if (value <= 0xFFFF) {
        utf8_seq[0] = (value >> 12) | 0xE0;
        utf8_seq[1] = ((value >> 6) & 0x3F) | 0x80;
        utf8_seq[2] = (value & 0x3F) | 0x80;
        *utf8_seqlen = 3;
    }
    else {
        utf8_seq[0] = (value >> 18) | 0xF0;
        utf8_seq[1] = ((value >> 12) & 0x3F) | 0x80;
        utf8_seq[2] = ((value >> 6) & 0x3F) | 0x80;
        utf8_seq[3] = (value & 0x3F) | 0x80;
        *utf8_seqlen = 4;
    }
}

static char *
_jsmnf_utf8_append(unsigned long x, char *d)
{
    unsigned utf8_seqlen;
    char utf8_seq[4];
    unsigned i;

    _jsmnf_utf8_encode(x, utf8_seq, &utf8_seqlen);

    for (i = 0; i < utf8_seqlen; ++i)
        *d++ = utf8_seq[i];
    return d;
}

JSMN_API size_t
jsmnf_unescape(char **p_dest, const char src[], size_t size)
{
    enum { TESTING = 1, ALLOCATING, UNESCAPING } state = TESTING;

    char *src_start = (char *)src, *src_end = (char *)src + size;
    char *out_start = NULL, *d = NULL, *s = NULL;
    unsigned first_surrogate;
    int second_surrogate_expected;
    char c;

second_iter:
    first_surrogate = 0;
    second_surrogate_expected = 0;

    for (s = src_start; s < src_end;) {
        c = *s++;

        if (second_surrogate_expected && c != '\\') goto _err;
        if (0 <= c && c <= 0x1F) goto _err;

        if ('\\' == c) {
            /* break the while loop */
            if (TESTING == state) {
                state = ALLOCATING;
                break;
            }

            /* return if src is a well-formed json string */
            if (s == src_end) goto _err;

            c = *s++;

            if (second_surrogate_expected && c != 'u') goto _err;

            switch (c) {
            case '"':
            case '\\':
            case '/':
                *d++ = c;
                break;
            case 'b':
                *d++ = '\b';
                break;
            case 'f':
                *d++ = '\f';
                break;
            case 'n':
                *d++ = '\n';
                break;
            case 'r':
                *d++ = '\r';
                break;
            case 't':
                *d++ = '\t';
                break;
            case 'u': {
                unsigned x;

                if (!_jsmnf_read_4_digits(&s, src_end, &x)) goto _err;

                if (second_surrogate_expected) {
                    if (!_JSMNF_UTF16_IS_SECOND_SURROGATE(x)) goto _err;

                    d = _jsmnf_utf8_append(
                        _JSMNF_UTF16_JOIN_SURROGATE(first_surrogate, x), d);
                    second_surrogate_expected = 0;
                }
                else if (_JSMNF_UTF16_IS_FIRST_SURROGATE(x)) {
                    second_surrogate_expected = 1;
                    first_surrogate = x;
                }
                else {
                    d = _jsmnf_utf8_append(x, d);
                }
            } break;
            default:
                goto _err;
            }
        }
        else if (UNESCAPING == state) {
            *d++ = c;
        }
    }

    /** TODO: remove calloc() calls and return a JSMN_ERRO_NOMEM value
     *      instead */
    switch (state) {
    case UNESCAPING:
        if (!_jsmnf_utf8_validate(out_start, d)) goto _err;

        *p_dest = out_start;
        return d - out_start;
    case ALLOCATING:
        out_start = calloc(1, size);
        d = out_start;
        state = UNESCAPING;
        goto second_iter;
    case TESTING:
        *p_dest = calloc(1, size + 1);
        memcpy(*p_dest, src_start, size);
        (*p_dest)[size] = '\0';
        return size;
    default:
        break;
    }

_err:
    if (UNESCAPING == state) free(out_start);
    return 0;
}
#endif /* JSMN_HEADER */

#ifdef __cplusplus
}
#endif

#endif /* JSMN_FIND_H */
