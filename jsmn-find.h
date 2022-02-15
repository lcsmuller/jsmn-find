#ifndef JSMN_FIND_H
#define JSMN_FIND_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef JSMN_H
#error "jsmn-find.h should be included after jsmn.h"
#endif

#include "uthash.h"

/** @brief store key/value jsmn tokens in a hashtable */
typedef struct jsmnf {
    /** the key of the pair (null if root) */
    jsmntok_t *key;
    /** the value of the pair (null if unexistent) */
    jsmntok_t *val;
    /** the positional index of the pair */
    int idx;
    /** this structure fields */
    struct jsmnf *child;
    /** make this structure fields hashable */
    UT_hash_handle hh;
} jsmnf;

/**
 * @brief Initialize a @ref jsmnf root
 *
 * @return a @ref jsmnf root that should be cleanup up with
 *      jsmnf_cleanup()
 */
JSMN_API jsmnf *jsmnf_init(void);

/**
 * @brief Cleanup a @ref jsmnf handle
 *
 * @param root the @ref jsmnf root initialized with jsmnf_init()
 */
JSMN_API void jsmnf_cleanup(jsmnf *root);

/**
 * @brief Populate the @ref jsmnf root with jsmn tokens
 *
 * @param root the @ref jsmnf structure initialized with jsmnf_init()
 * @param json the raw JSON string
 * @param size the raw JSON length
 * @return a negative number for error, or the number of tokens found
 */
JSMN_API int jsmnf_start(jsmnf *root, const char json[], size_t size);

/**
 * @brief Find a value `jsmntok_t` by its key
 *
 * @param root the @ref jsmnf structure initialized with jsmnf_init()
 * @param key the key too be matched
 * @param size size of the key too be matched
 * @return the key/value pair matched to `key`
 */
JSMN_API jsmnf *jsmnf_find(jsmnf *root, const char key[], size_t size);

/**
 * @brief Find a value `jsmntok_t` by its key path
 *
 * @param root the @ref jsmnf structure initialized with jsmnf_init()
 * @param path an array of key path strings, from least to highest depth
 * @param depth the depth level of the last `path` key
 * @return the key/value pair matched to `path`
 */
JSMN_API jsmnf *jsmnf_find_path(jsmnf *root, char *const path[], int depth);

#ifndef JSMN_HEADER
#include <stdio.h>
#include <stdlib.h>

struct _jsmnroot {
    /**
     * the root jsmnf
     * @note `root` must be the first element so that `struct _jsmnroot` can be
     *      safely cast to `struct jsmnf` */
    jsmnf root;
    /** tokens storage cap */
    size_t real_ntoks;
    /** amount of tokens currently stored */
    size_t ntoks;
};

static jsmnf *
_jsmnf_init(void)
{
    return calloc(1, sizeof(jsmnf));
}

jsmnf *
jsmnf_init(void)
{
    struct _jsmnroot *r = calloc(1, sizeof *r);
    if (!r) return NULL;

    r->real_ntoks = 128;
    r->root.val = malloc(r->real_ntoks * sizeof *r->root.val);
    if (!r->root.val) {
        free(r);
        return NULL;
    }
    return &r->root;
}

static void
_jsmnf_cleanup(jsmnf *head)
{
    if (!head) return;

    if (JSMN_OBJECT == head->val->type || JSMN_ARRAY == head->val->type) {
        jsmnf *iter, *tmp;

        HASH_ITER(hh, head->child, iter, tmp)
        {
            HASH_DEL(head->child, iter);
            _jsmnf_cleanup(iter);
            free(iter);
        }
    }
}

void
jsmnf_cleanup(jsmnf *root)
{
    _jsmnf_cleanup(root);
    free(root->val);
    free(root);
}

static int
_jsmnf_get_pairs(const char js[], jsmntok_t *tok, size_t ntoks, jsmnf *head)
{
    int offset = 0;

    if (!ntoks) return 0;

    switch (tok->type) {
    case JSMN_OBJECT: {
        jsmnf *curr;
        int ret;
        int i;

        for (i = 0; i < tok->size; ++i) {
            curr = _jsmnf_init();
            curr->idx = i;
            curr->key = tok + 1 + offset;

            ret = _jsmnf_get_pairs(js, curr->key, ntoks - offset, curr);
            if (ret < 0) return ret;

            offset += ret;

            if (curr->key->size > 0) {
                curr->val = tok + 1 + offset;

                ret = _jsmnf_get_pairs(js, curr->val, ntoks - offset, curr);
                if (ret < 0) return ret;

                offset += ret;
            }

            HASH_ADD_KEYPTR(hh, head->child, js + curr->key->start,
                            curr->key->end - curr->key->start, curr);
        }
    } break;
    case JSMN_ARRAY: {
        jsmnf *curr;
        int ret;
        int i;

        for (i = 0; i < tok->size; ++i) {
            curr = _jsmnf_init();
            curr->idx = i;
            curr->val = tok + 1 + offset;

            ret = _jsmnf_get_pairs(js, curr->val, ntoks - offset, curr);
            if (ret < 0) return ret;

            offset += ret;

            HASH_ADD_INT(head->child, idx, curr);
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
jsmnf_start(jsmnf *root, const char js[], size_t size)
{
    struct _jsmnroot *r = (struct _jsmnroot *)root;
    jsmn_parser parser;
    int ret;

    /* Prepare parser */
    jsmn_init(&parser);
    while (1) {
        ret = jsmn_parse(&parser, js, size, root->val, r->real_ntoks);

        if (ret >= 0) {
            r->ntoks = parser.toknext;
            ret = _jsmnf_get_pairs(js, root->val, r->ntoks, root);
            break;
        }
        else {
            if (ret != JSMN_ERROR_NOMEM) {
                break;
            }
            else {
                size_t new_ntoks = r->real_ntoks * 2;
                void *tmp;

                tmp = realloc(root->val, new_ntoks * sizeof *root->val);
                if (!tmp) return JSMN_ERROR_NOMEM;

                r->real_ntoks = new_ntoks;
                root->val = tmp;
            }
        }
    }
    return ret;
}

jsmnf *
jsmnf_find(jsmnf *head, const char key[], size_t size)
{
    jsmnf *found = NULL;

    if (!key || !head) return NULL;

    if (JSMN_OBJECT == head->val->type) {
        HASH_FIND(hh, head->child, key, size, found);
    }
    else if (JSMN_ARRAY == head->val->type) {
        char *endptr;
        int idx = (int)strtol(key, &endptr, 10);

        if (endptr == key) return NULL;

        HASH_FIND_INT(head->child, &idx, found);
    }
    return found;
}

jsmnf *
jsmnf_find_path(jsmnf *head, char *const path[], int depth)
{
    jsmnf *iter = head, *found = NULL;
    int i;

    for (i = 0; i < depth; ++i) {
        if (!iter) continue;
        found = jsmnf_find(iter, path[i], strlen(path[i]));
        if (!found) break;
        iter = found;
    }
    return found;
}
#endif /* JSMN_HEADER */

#ifdef __cplusplus
}
#endif

#endif /* JSMN_FIND_H */
