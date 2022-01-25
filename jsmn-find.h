#ifndef JSMN_FIND_H
#define JSMN_FIND_H

#ifndef JSMN_H
#error "jsmn-find.h should be included after jsmn.h"
#endif

#ifdef JSMN_HEADER
typedef struct jsmnfind jsmnfind;
#else
#include <stdio.h>
#include <stdlib.h>
#include "uthash.h"

struct jsmnpair {
    jsmntok_t *key;
    jsmntok_t *value;
    int idx;
    UT_hash_handle hh;
    struct jsmnpair *head;
};

typedef struct jsmnfind {
    size_t real_ntoks;
    size_t ntoks;
    jsmntok_t *toks;
    struct jsmnpair *root;
} jsmnfind;
#endif /* JSMN_HEADER */

JSMN_API jsmnfind *jsmnfind_init(void);
JSMN_API void jsmnfind_cleanup(jsmnfind *handle);
JSMN_API int jsmnfind_start(jsmnfind *handle, const char json[], size_t size);
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
    if (pair->value
        && (JSMN_OBJECT == pair->value->type
            || JSMN_ARRAY == pair->value->type))
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
    handle->root = _jsmnpair_init();
    if (!handle->root) {
        free(handle);
        free(handle->toks);
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
                curr->value = tok + 1 + offset;

                ret =
                    _jsmnfind_get_pairs(js, curr->value, ntoks - offset, curr);
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
            curr->value = tok + 1 + offset;

            ret = _jsmnfind_get_pairs(js, curr->value, ntoks - offset, curr);
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

    /* Prepare parser */
    jsmn_init(&parser);

    while (1) {
        ret = jsmn_parse(&parser, js, size, handle->toks, handle->real_ntoks);

        if (ret >= 0) {
            handle->ntoks = parser.toknext;
            handle->root->value = &handle->toks[0];

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
        if (JSMN_OBJECT == iter->value->type) {
            HASH_FIND_STR(iter->head, path[i], found);
        }
        else if (JSMN_ARRAY == iter->value->type) {
            char *endptr;
            int idx = (int)strtol(path[i], &endptr, 10);

            if (endptr == path[i]) return NULL;

            HASH_FIND_INT(iter->head, &idx, found);
        }

        iter = found;
    }

    return found ? found->value : NULL;
}
#endif /* JSMN_HEADER */

#endif /* JSMN_FIND_H */
