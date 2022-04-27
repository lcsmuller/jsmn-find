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
check_load_dynamic_pairs(void)
{
    const char js_small[] = "{\"foo\":[true]}";
    const char js_large[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";
    jsmn_parser parser;
    jsmntok_t toks[64];
    jsmnf_loader loader;
    jsmnf_pair *pairs = NULL, *f;
    unsigned num_pairs = 0;
    long ret;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, js_small, sizeof(js_small) - 1, toks,
               sizeof(toks) / sizeof *toks);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load_auto(&loader, js_small, toks, parser.toknext,
                                     &pairs, &num_pairs),
               0);

    ASSERT_NEQ(NULL, f = jsmnf_find(pairs, js_small, "foo", 3));
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js_small, "0", 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0, f->k.len);
    ASSERT_STRN_EQ("true", js_small + f->v.pos, f->v.len);

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, js_large, sizeof(js_large) - 1, toks,
               sizeof(toks) / sizeof *toks);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load_auto(&loader, js_large, toks, parser.toknext,
                                     &pairs, &num_pairs),
               0);

    ASSERT_NEQ(NULL, f = jsmnf_find(pairs, js_large, "foo", 3));
    ASSERT_STRN_EQ("foo", js_large + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js_large, "bar", 3));
    ASSERT_STRN_EQ("bar", js_large + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js_large, "baz", 3));
    ASSERT_STRN_EQ("baz", js_large + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js_large, "0", 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0, f->k.len);
    ASSERT_STRN_EQ("true", js_large + f->v.pos, f->v.len);

    free(pairs);

    PASS();
}

TEST
check_load_dynamic_pairs_and_tokens(void)
{
    const char js_small[] = "{\"foo\":[true]}";
    const char js_large[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";
    jsmn_parser parser;
    jsmntok_t *toks = NULL;
    jsmnf_loader loader;
    jsmnf_pair *pairs = NULL, *f;
    unsigned num_tokens = 0, num_pairs = 0;
    long ret;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmn_parse_auto(&parser, js_small, sizeof(js_small) - 1,
                                     &toks, &num_tokens),
               0);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load_auto(&loader, js_small, toks, num_tokens,
                                     &pairs, &num_pairs),
               0);

    ASSERT_NEQ(NULL, f = jsmnf_find(pairs, js_small, "foo", 3));
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js_small, "0", 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0, f->k.len);
    ASSERT_STRN_EQ("true", js_small + f->v.pos, f->v.len);

    jsmn_init(&parser);
    jsmnf_init(&loader);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmn_parse_auto(&parser, js_large, sizeof(js_large) - 1,
                                     &toks, &num_tokens),
               0);
    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load_auto(&loader, js_large, toks, parser.toknext,
                                     &pairs, &num_pairs),
               0);

    ASSERT_NEQ(NULL, f = jsmnf_find(pairs, js_large, "foo", 3));
    ASSERT_STRN_EQ("foo", js_large + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js_large, "bar", 3));
    ASSERT_STRN_EQ("bar", js_large + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js_large, "baz", 3));
    ASSERT_STRN_EQ("baz", js_large + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js_large, "0", 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0, f->k.len);
    ASSERT_STRN_EQ("true", js_large + f->v.pos, f->v.len);

    free(toks);
    free(pairs);

    PASS();
}

SUITE(fn__jsmnf_load_auto)
{
    RUN_TEST(check_load_dynamic_pairs);
    RUN_TEST(check_load_dynamic_pairs_and_tokens);
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
check_load_not_enough_pairs_for_tokens(void)
{
    const char js[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";
    jsmn_parser parser;
    jsmntok_t toks[64];
    jsmnf_loader loader;
    jsmnf_pair pairs[64], *f;
    long ret;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, js, sizeof(js) - 1, toks, sizeof(toks) / sizeof *toks);

    /* not enough pairs should return JSMN_ERROR_NOMEM */
    ASSERT_EQm(print_jsmnerr(ret), JSMN_ERROR_NOMEM,
               ret = jsmnf_load(&loader, js, toks, parser.toknext, pairs,
                                parser.toknext));
    /* simulate realloc */
    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load(&loader, js, toks, parser.toknext, pairs,
                                sizeof(pairs) / sizeof *pairs),
               0);

    /* check if searching still works */
    ASSERT_NEQ(NULL, f = jsmnf_find(pairs, js, "foo", 3));
    ASSERT_STRN_EQ("foo", js + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js, "bar", 3));
    ASSERT_STRN_EQ("bar", js + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js, "baz", 3));
    ASSERT_STRN_EQ("baz", js + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js, "0", 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0, f->k.len);
    ASSERT_STRN_EQ("true", js + f->v.pos, f->v.len);

    PASS();
}

TEST
check_load_basic(void)
{
    const char js[] = "{\"a\":1,\"c\":{}}";

    jsmn_parser parser;
    jsmntok_t toks[32];
    jsmnf_loader loader;
    jsmnf_pair pairs[16];
    long ret;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, js, sizeof(js) - 1, toks, sizeof(toks) / sizeof *toks);

    /* not enough pairs should return JSMN_ERROR_NOMEM */
    ASSERT_EQm(print_jsmnerr(ret), JSMN_ERROR_NOMEM,
               ret = jsmnf_load(&loader, js, toks, parser.toknext, pairs,
                                sizeof(pairs) / sizeof *pairs / 2));
    /* simulate realloc */
    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load(&loader, js, toks, parser.toknext, pairs,
                                sizeof(pairs) / sizeof *pairs),
               0);

    PASS();
}

TEST
check_load_array(void)
{
    const char js[] = "[1, [1, 2, [1, 2, 3, [true]]]]";
    jsmn_parser parser;
    jsmntok_t toks[64];
    jsmnf_loader loader;
    jsmnf_pair pairs[64], *f;
    long ret;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, js, sizeof(js) - 1, toks, sizeof(toks) / sizeof *toks);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load(&loader, js, toks, parser.toknext, pairs,
                                sizeof(pairs) / sizeof *pairs),
               0);

    ASSERT_EQ(2, pairs->size);
    f = &pairs->fields[1];
    ASSERT_EQ(3, f->size);
    f = &f->fields[2];
    ASSERT_EQ(4, f->size);
    f = &f->fields[3];
    ASSERT_EQ(1, f->size);

    PASS();
}

SUITE(fn__jsmnf_load)
{
    RUN_TEST(check_load_not_enough_pairs_for_tokens);
    RUN_TEST(check_load_basic);
    RUN_TEST(check_load_array);
}

TEST
check_find_nested(void)
{
    const char js[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";
    jsmn_parser parser;
    jsmntok_t toks[64];
    jsmnf_loader loader;
    jsmnf_pair pairs[64], *f;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, js, sizeof(js) - 1, toks, sizeof(toks) / sizeof *toks);

    jsmnf_load(&loader, js, toks, parser.toknext, pairs,
               sizeof(pairs) / sizeof *pairs);

    ASSERT_NEQ(NULL, f = jsmnf_find(pairs, js, "foo", 3));
    ASSERT_STRN_EQ("foo", js + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js, "bar", 3));
    ASSERT_STRN_EQ("bar", js + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js, "baz", 3));
    ASSERT_STRN_EQ("baz", js + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js, "0", 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0, f->k.len);
    ASSERT_STRN_EQ("true", js + f->v.pos, f->v.len);

    PASS();
}

TEST
check_find_array(void)
{
    const char js[] = "[1, [1, 2, [1, 2, 3, [true]]]]";
    jsmn_parser parser;
    jsmntok_t toks[64];
    jsmnf_loader loader;
    jsmnf_pair pairs[64], *f;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, js, sizeof(js) - 1, toks, sizeof(toks) / sizeof *toks);

    jsmnf_load(&loader, js, toks, parser.toknext, pairs,
               sizeof(pairs) / sizeof *pairs);

    /* test direct search */
    f = &pairs->fields[0];
    ASSERT_STRN_EQ("1", js + f->v.pos, f->v.len);
    f = &pairs->fields[1];
    ASSERT_STRN_EQ("1", js + f->fields[0].v.pos, f->fields[0].v.len);
    ASSERT_STRN_EQ("2", js + f->fields[1].v.pos, f->fields[1].v.len);
    f = &f->fields[2];
    ASSERT_STRN_EQ("1", js + f->fields[0].v.pos, f->fields[0].v.len);
    ASSERT_STRN_EQ("2", js + f->fields[1].v.pos, f->fields[1].v.len);
    ASSERT_STRN_EQ("3", js + f->fields[2].v.pos, f->fields[2].v.len);
    f = &f->fields[3];
    ASSERT_STRN_EQ("[true]", js + f->v.pos, f->v.len);

    /* test key search */
    ASSERT_NEQ(NULL, f = jsmnf_find(pairs, js, "0", 1));
    ASSERT_STRN_EQ("1", js + f->v.pos, f->v.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(pairs, js, "1", 1));
    ASSERT_STRN_EQ("1", js + f->fields[0].v.pos, f->fields[0].v.len);
    ASSERT_STRN_EQ("2", js + f->fields[1].v.pos, f->fields[1].v.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js, "2", 1));
    ASSERT_STRN_EQ("1", js + f->fields[0].v.pos, f->fields[0].v.len);
    ASSERT_STRN_EQ("2", js + f->fields[1].v.pos, f->fields[1].v.len);
    ASSERT_STRN_EQ("3", js + f->fields[2].v.pos, f->fields[2].v.len);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, js, "3", 1));
    ASSERT_STRN_EQ("[true]", js + f->v.pos, f->v.len);

    PASS();
}

TEST
check_find_string_elements_in_array(void)
{
    const char js[] = "{\"modules\":[\"foo\",\"bar\",\"baz\",\"tuna\"]}";
    jsmn_parser parser;
    jsmntok_t toks[64];
    jsmnf_loader loader;
    jsmnf_pair pairs[64], *f1, *f2;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, js, sizeof(js) - 1, toks, sizeof(toks) / sizeof *toks);

    jsmnf_load(&loader, js, toks, parser.toknext, pairs,
               sizeof(pairs) / sizeof *pairs);

    ASSERT_NEQ(NULL, f1 = jsmnf_find(pairs, js, "modules", 7));

    /* test direct search */
    f2 = &f1->fields[0];
    ASSERT_STRN_EQ("foo", js + f2->v.pos, f2->v.len);
    f2 = &f1->fields[1];
    ASSERT_STRN_EQ("bar", js + f2->v.pos, f2->v.len);
    f2 = &f1->fields[2];
    ASSERT_STRN_EQ("baz", js + f2->v.pos, f2->v.len);
    f2 = &f1->fields[3];
    ASSERT_STRN_EQ("tuna", js + f2->v.pos, f2->v.len);

    PASS();
}

SUITE(fn__jsmnf_find)
{
    RUN_TEST(check_find_nested);
    RUN_TEST(check_find_array);
    RUN_TEST(check_find_string_elements_in_array);
}

TEST
check_find_path_nested(void)
{
    const char js[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";
    char *path[] = { "foo", "bar", "baz", "0" };
    jsmn_parser parser;
    jsmntok_t toks[64];
    jsmnf_loader loader;
    jsmnf_pair pairs[64], *f;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    jsmn_parse(&parser, js, sizeof(js) - 1, toks, sizeof(toks) / sizeof *toks);

    jsmnf_load(&loader, js, toks, parser.toknext, pairs,
               sizeof(pairs) / sizeof *pairs);

    ASSERT_NEQ(NULL, f = jsmnf_find_path(pairs, js, path, 1));
    ASSERT_STRN_EQ(path[0], js + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find_path(pairs, js, path, 2));
    ASSERT_STRN_EQ(path[1], js + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find_path(pairs, js, path, 3));
    ASSERT_STRN_EQ(path[2], js + f->k.pos, f->k.len);
    ASSERT_NEQ(NULL, f = jsmnf_find_path(pairs, js, path, 4));
    ASSERT_EQm("Array elements shouldn't have a key", 0, f->k.len);
    ASSERT_STRN_EQ("true", js + f->v.pos, f->v.len);

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

    RUN_SUITE(fn__jsmnf_load_auto);
    RUN_SUITE(fn__jsmnf_unescape);
    RUN_SUITE(fn__jsmnf_load);
    RUN_SUITE(fn__jsmnf_find);
    RUN_SUITE(fn__jsmnf_find_path);

    GREATEST_MAIN_END();
}
