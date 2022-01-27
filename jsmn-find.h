#ifndef JSMN_FIND_H
#define JSMN_FIND_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef JSMN_H
#error "jsmn-find.h should be included after jsmn.h"
#endif

#ifdef JSMN_HEADER
typedef struct jsmnfind jsmnfind;
#else
#include <stdio.h>
#include <stdlib.h>
#include "uthash.h"

/** @brief store key/value jsmn tokens in a hashtable */
struct jsmnpair {
    /** the key of the pair (null if root) */
    jsmntok_t *key;
    /** the value of the pair (null if unexistent) */
    jsmntok_t *val;
    /** the positional index of the pair */
    int idx;
    /** this structure inner fields root */
    struct jsmnpair *head;
    /** make this structure inner fields hashable */
    UT_hash_handle hh;
};

/** @brief store key/value pairs */
typedef struct jsmnfind {
    /** tokens storage cap */
    size_t real_ntoks;
    /** amount of tokens currently stored */
    size_t ntoks;
    /** the jsmn tokens array */
    jsmntok_t *toks;
    /** the key/value pair root */
    struct jsmnpair *root;
} jsmnfind;
#endif /* JSMN_HEADER */

/**
 * @brief Initialize a @ref jsmnfind handle
 *
 * @return a @ref jsmnfind handle that should be cleanup up with
 * jsmnfind_cleanup()
 */
JSMN_API jsmnfind *jsmnfind_init(void);

/** @brief Cleanup a @ref jsmnfind handle */
JSMN_API void jsmnfind_cleanup(jsmnfind *handle);

/**
 * @brief Populate the @ref jsmnfind structure with jsmn tokens
 *
 * @param handle the @ref jsmnfind structure initialized with jsmnfind_init()
 * @param json the raw JSON string
 * @param size the raw JSON length
 * @return a negative number for error, or the number of tokens found
 */
JSMN_API int jsmnfind_start(jsmnfind *handle, const char json[], size_t size);

/**
 * @brief Find a value `jsmntok_t` by its key path
 *
 * @param handle the @ref jsmnfind structure initialized with jsmnfind_init()
 * @param path an array of key path strings, from least to highest depth
 * @param depth the depth level of the last `path` key
 * @return the jsmn value matched to `path`
 */
JSMN_API jsmntok_t *jsmnfind_find(jsmnfind *handle, char *path[], int depth);

#ifndef JSMN_HEADER
static struct jsmnpair *
_jsmnpair_init(void)
{
    struct jsmnpair *pair = calloc(1, sizeof *pair);
    if (!pair) return NULL;

    return pair;
}

static void
_jsmnpair_cleanup(struct jsmnpair *pair)
{
    if (pair->val
        && (JSMN_OBJECT == pair->val->type || JSMN_ARRAY == pair->val->type))
    {
        struct jsmnpair *curr, *tmp;

        HASH_ITER(hh, pair->head, curr, tmp)
        {
            HASH_DEL(pair->head, curr);
            _jsmnpair_cleanup(curr);
        }
    }
    free(pair);
}

jsmnfind *
jsmnfind_init(void)
{
    jsmnfind *handle = calloc(1, sizeof *handle);
    if (!handle) return NULL;

    handle->real_ntoks = 128;
    handle->toks = malloc(handle->real_ntoks * sizeof *handle->toks);
    if (!handle->toks) {
        free(handle);
        return NULL;
    }
    return handle;
}

void
jsmnfind_cleanup(jsmnfind *handle)
{
    _jsmnpair_cleanup(handle->root);
    free(handle->toks);
    free(handle);
}

static int
_jsmnfind_get_pairs(const char js[],
                    jsmntok_t *tok,
                    size_t ntoks,
                    struct jsmnpair *parent)
{
    int offset = 0;

    if (!ntoks) return 0;

    switch (tok->type) {
    case JSMN_OBJECT: {
        struct jsmnpair *curr;
        int ret;
        int i;

        for (i = 0; i < tok->size; ++i) {
            curr = _jsmnpair_init();
            curr->idx = i;
            curr->key = tok + 1 + offset;

            ret = _jsmnfind_get_pairs(js, curr->key, ntoks - offset, curr);
            if (ret < 0) return ret;

            offset += ret;

            if (curr->key->size > 0) {
                curr->val = tok + 1 + offset;

                ret = _jsmnfind_get_pairs(js, curr->val, ntoks - offset, curr);
                if (ret < 0) return ret;

                offset += ret;
            }
            HASH_ADD_KEYPTR(hh, parent->head, js + curr->key->start,
                            curr->key->end - curr->key->start, curr);
        }
    } break;
    case JSMN_ARRAY: {
        struct jsmnpair *curr;
        int ret;
        int i;

        for (i = 0; i < tok->size; ++i) {
            curr = _jsmnpair_init();
            curr->idx = i;
            curr->val = tok + 1 + offset;

            ret = _jsmnfind_get_pairs(js, curr->val, ntoks - offset, curr);
            if (ret < 0) return ret;

            offset += ret;

            HASH_ADD_INT(parent->head, idx, curr);
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
    return offset + 1;
}

int
jsmnfind_start(jsmnfind *handle, const char js[], size_t size)
{
    jsmn_parser parser;
    int ret;

    /* cleanup existing pairs for reuse of handle */
    if (handle->root) _jsmnpair_cleanup(handle->root);

    handle->root = _jsmnpair_init();
    if (!handle->root) return JSMN_ERROR_NOMEM;

    /* Prepare parser */
    jsmn_init(&parser);

    while (1) {
        ret = jsmn_parse(&parser, js, size, handle->toks, handle->real_ntoks);

        if (ret >= 0) {
            handle->ntoks = parser.toknext;
            handle->root->val = &handle->toks[0];

            ret = _jsmnfind_get_pairs(js, handle->toks, handle->ntoks,
                                      handle->root);

            break;
        }
        else {
            if (ret != JSMN_ERROR_NOMEM) {
                break;
            }
            else {
                size_t new_ntoks = handle->real_ntoks * 2;
                void *tmp;

                tmp = realloc(handle->toks, new_ntoks * sizeof *handle->toks);
                if (!tmp) return JSMN_ERROR_NOMEM;

                handle->real_ntoks = new_ntoks;
                handle->toks = tmp;
            }
        }
    }

    return ret;
}

jsmntok_t *
jsmnfind_find(jsmnfind *handle, char *path[], int depth)
{
    struct jsmnpair *iter = handle->root, *found = NULL;
    int i;

    for (i = 0; i < depth; ++i) {
        if (!iter->val) continue;

        if (JSMN_OBJECT == iter->val->type) {
            HASH_FIND_STR(iter->head, path[i], found);
        }
        else if (JSMN_ARRAY == iter->val->type) {
            char *endptr;
            int idx = (int)strtol(path[i], &endptr, 10);

            if (endptr == path[i]) return NULL;

            HASH_FIND_INT(iter->head, &idx, found);
        }

        iter = found;
    }

    return found ? found->val : NULL;
}
#endif /* JSMN_HEADER */

#ifdef __cplusplus
}
#endif

#endif /* JSMN_FIND_H */
