#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define JSMN_STRICT
#include "jsmn_1.1.0.h"
#include "jsmn-find.h"
#include "greatest.h"

#define TODO 0

const char *
print_jsmnerr(enum jsmnerr code)
{
    switch (code) {
    case JSMN_ERROR_NOMEM:
        return "JSMN_ERROR_NOMEM: Not enough tokens were provided";
    case JSMN_ERROR_INVAL:
        return "JSMN_ERROR_INVAL: Invalid character inside a JSON string";
    case JSMN_ERROR_PART:
        return "JSMN_ERROR_PART: The string is not a full JSON packet, more "
               "bytes expected";
    default:
        if (code == 0) return "Ouput string is empty";
        if (code > 0) return "No error encountered";
    }
    return "Unknown error";
}

TEST
check_unescaping(char *(*accept)[3])
{
    char buf[256] = "";
    long ret;

    ASSERT_GTEm(print_jsmnerr(ret),
                ret = jsmnf_unescape(buf, sizeof(buf), (*accept)[1],
                                     strlen((*accept)[1])),
                0);

    ASSERT_STR_EQm((*accept)[2], (*accept)[0], buf);

    PASS();
}

TEST
check_unescaping_not_enough_buffer_memory(void)
{
    char src[] = "\\u00e1\\u00e9\\u00ed\\u00f3\\u00fa\\u00c1\\u00c9\\u00cd\\u0"
                 "0d3\\u0000";
    char buf[32];
    long ret;

    ASSERT_EQm(print_jsmnerr(ret), JSMN_ERROR_NOMEM,
               ret = jsmnf_unescape(buf, 4, src, sizeof(src)));
    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_unescape(buf, sizeof(buf), src, sizeof(src)), 0);

    PASS();
}

SUITE(fn__jsmnf_unescape)
{
    /* [0]: expected out ; [1]: input string ; [2]: test meaning */
    char *accept[][3] = {
        { "\\\"", "\\\\\\\"", "escaped quote" },
        { "\\", "\\\\", "escaped escape" },
        { "☺☺", "\\u263A\\u263A", "unicode sequence" },
        { "abc", "abc", "bytes ascii" },
#if TODO
        /* TODO: support octal */
        { "Ã¿", "\303\277", "multi octal sequence" },
        { "ÿ", "\377", "octal sequence" },
        /* TODO: unescape commas recursively */
        { "\"x\"\"x\"", "\"x\"\"x\"", "triple double quotes" },
        { "\007\b\f\n\r\t\013'\"\\?", "\\a\\b\\f\\n\\r\\t\\v\\'\\\"\\\\\\?",
          "legal escapes" },
#endif
    };
    size_t i;

    for (i = 0; i < sizeof(accept) / sizeof *accept; ++i)
        RUN_TEST1(check_unescaping, accept + i);

    RUN_TEST(check_unescaping_not_enough_buffer_memory);
}

TEST
check_load_without_enough_pairs(void)
{
    const char json[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";
    jsmn_parser parser;
    jsmntok_t toks[64];
    jsmnf_loader loader;
    jsmnf_pair pairs[64], *f;
    long ret;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, json, sizeof(json) - 1, toks,
               sizeof(toks) / sizeof *toks);

    /* not enough pairs should return JSMN_ERROR_NOMEM */
    ASSERT_EQm(print_jsmnerr(ret), JSMN_ERROR_NOMEM,
               ret = jsmnf_load(&loader, json, toks, parser.toknext, pairs,
                                parser.toknext));
    /* simulate realloc */
    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load(&loader, json, toks, parser.toknext, pairs,
                                sizeof(pairs) / sizeof *pairs),
               0);

    /* check if searching still works */
    ASSERT_NEQ(NULL, f = jsmnf_find(pairs, "foo", sizeof("foo") - 1));
    ASSERT_STRN_EQ("foo", f->key.contents, f->key.length);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "bar", sizeof("bar") - 1));
    ASSERT_STRN_EQ("bar", f->key.contents, f->key.length);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "baz", sizeof("baz") - 1));
    ASSERT_STRN_EQ("baz", f->key.contents, f->key.length);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "0", sizeof("0") - 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0, f->key.length);
    ASSERT_STRN_EQ("true", f->value.contents, f->value.length);

    PASS();
}

TEST
check_load_array(void)
{
    const char json[] = "[1, [1, 2, [1, 2, 3, [true]]]]";
    jsmn_parser parser;
    jsmntok_t toks[64];
    jsmnf_loader loader;
    jsmnf_pair pairs[64], *f;
    long ret;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, json, sizeof(json) - 1, toks,
               sizeof(toks) / sizeof *toks);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load(&loader, json, toks, parser.toknext, pairs,
                                sizeof(pairs) / sizeof *pairs),
               0);

    ASSERT_EQ(2, pairs->length);
    f = &pairs->buckets[1];
    ASSERT_EQ(3, f->length);
    f = &f->buckets[2];
    ASSERT_EQ(4, f->length);
    f = &f->buckets[3];
    ASSERT_EQ(1, f->length);

    PASS();
}

SUITE(fn__jsmnf_load)
{
    RUN_TEST(check_load_without_enough_pairs);
    RUN_TEST(check_load_array);
}

TEST
check_find_nested(void)
{
    const char json[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";
    jsmn_parser parser;
    jsmntok_t toks[64];
    jsmnf_loader loader;
    jsmnf_pair pairs[64], *f;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, json, sizeof(json) - 1, toks,
               sizeof(toks) / sizeof *toks);

    jsmnf_load(&loader, json, toks, parser.toknext, pairs,
               sizeof(pairs) / sizeof *pairs);

    ASSERT_NEQ(NULL, f = jsmnf_find(pairs, "foo", sizeof("foo") - 1));
    ASSERT_STRN_EQ("foo", f->key.contents, f->key.length);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "bar", sizeof("bar") - 1));
    ASSERT_STRN_EQ("bar", f->key.contents, f->key.length);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "baz", sizeof("baz") - 1));
    ASSERT_STRN_EQ("baz", f->key.contents, f->key.length);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "0", sizeof("0") - 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0, f->key.length);
    ASSERT_STRN_EQ("true", f->value.contents, f->value.length);

    PASS();
}

TEST
check_find_array(void)
{
    const char json[] = "[1, [1, 2, [1, 2, 3, [true]]]]";
    jsmn_parser parser;
    jsmntok_t toks[64];
    jsmnf_loader loader;
    jsmnf_pair pairs[64], *f;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, json, sizeof(json) - 1, toks,
               sizeof(toks) / sizeof *toks);

    jsmnf_load(&loader, json, toks, parser.toknext, pairs,
               sizeof(pairs) / sizeof *pairs);

    /* test direct search */
    f = &pairs->buckets[0];
    ASSERT_STRN_EQ("1", f->value.contents, f->value.length);
    f = &pairs->buckets[1];
    ASSERT_STRN_EQ("1", f->buckets[0].value.contents,
                   f->buckets[0].value.length);
    ASSERT_STRN_EQ("2", f->buckets[1].value.contents,
                   f->buckets[1].value.length);
    f = &f->buckets[2];
    ASSERT_STRN_EQ("1", f->buckets[0].value.contents,
                   f->buckets[0].value.length);
    ASSERT_STRN_EQ("2", f->buckets[1].value.contents,
                   f->buckets[1].value.length);
    ASSERT_STRN_EQ("3", f->buckets[2].value.contents,
                   f->buckets[2].value.length);
    f = &f->buckets[3];
    ASSERT_STRN_EQ("[true]", f->value.contents, f->value.length);

    /* test key search */
    ASSERT_NEQ(NULL, f = jsmnf_find(pairs, "0", 1));
    ASSERT_STRN_EQ("1", f->value.contents, f->value.length);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "1", 1));
    ASSERT_STRN_EQ("1", f->buckets[0].value.contents,
                   f->buckets[0].value.length);
    ASSERT_STRN_EQ("2", f->buckets[1].value.contents,
                   f->buckets[1].value.length);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "2", 1));
    ASSERT_STRN_EQ("1", f->buckets[0].value.contents,
                   f->buckets[0].value.length);
    ASSERT_STRN_EQ("2", f->buckets[1].value.contents,
                   f->buckets[1].value.length);
    ASSERT_STRN_EQ("3", f->buckets[2].value.contents,
                   f->buckets[2].value.length);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "3", 1));
    ASSERT_STRN_EQ("[true]", f->value.contents, f->value.length);

    PASS();
}

SUITE(fn__jsmnf_find)
{
    RUN_TEST(check_find_nested);
    RUN_TEST(check_find_array);
}

TEST
check_find_path_nested(void)
{
    const char json[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";
    char *path[] = { "foo", "bar", "baz", "0" };
    jsmn_parser parser;
    jsmntok_t toks[64];
    jsmnf_loader loader;
    jsmnf_pair pairs[64], *f;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, json, sizeof(json) - 1, toks,
               sizeof(toks) / sizeof *toks);

    jsmnf_load(&loader, json, toks, parser.toknext, pairs,
               sizeof(pairs) / sizeof *pairs);

    ASSERT_NEQ(NULL, f = jsmnf_find_path(pairs, path, 1));
    ASSERT_STRN_EQ(path[0], f->key.contents, f->key.length);
    ASSERT_NEQ(NULL, f = jsmnf_find_path(pairs, path, 2));
    ASSERT_STRN_EQ(path[1], f->key.contents, f->key.length);
    ASSERT_NEQ(NULL, f = jsmnf_find_path(pairs, path, 3));
    ASSERT_STRN_EQ(path[2], f->key.contents, f->key.length);
    ASSERT_NEQ(NULL, f = jsmnf_find_path(pairs, path, 4));
    ASSERT_EQm("Array elements shouldn't have a key", 0, f->key.length);
    ASSERT_STRN_EQ("true", f->value.contents, f->value.length);

    PASS();
}

SUITE(fn__jsmnf_find_path)
{
    RUN_TEST(check_find_path_nested);
}

GREATEST_MAIN_DEFS();

int
main(int argc, char *argv[])
{
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(fn__jsmnf_unescape);
    RUN_SUITE(fn__jsmnf_load);
    RUN_SUITE(fn__jsmnf_find);
    RUN_SUITE(fn__jsmnf_find_path);

    GREATEST_MAIN_END();
}
