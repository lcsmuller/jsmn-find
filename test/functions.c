#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define JSMN_STRICT
#include "jsmn_1.1.0.h"
#include "jsmn-find.h"
#include "greatest.h"

TEST
check_unescaping(void *p_pair)
{
    char *(*pair)[2] = p_pair;
    char *str = NULL;

    ASSERT(jsmnf_unescape(&str, (*pair)[1], strlen((*pair)[1])) != 0);
    ASSERT_NEQ(NULL, str);
    ASSERT_STR_EQ((*pair)[0], str);

    free(str);

    PASS();
}

/* unescape only the first 5 characters */
TEST
check_unescaping_with_threshold_length(void)
{
#define EXPECTED "áéíóú"
    char *pair[1][2] = {
        { EXPECTED "\\u00c1\\u00c9\\u00cd\\u00d3\\u00da",
          "\\u00e1\\u00e9\\u00ed\\u00f3\\u00fa\\u00c1\\u00c9\\u00cd\\u00d3\\u0"
          "0da" },
    };
    char *str = NULL;

    ASSERT(
        jsmnf_unescape(&str, (*pair)[1], strlen("\\u0000") * strlen(EXPECTED))
        != 0);
    ASSERT_NEQ(NULL, str);
    ASSERT_STRN_EQ((*pair)[0], str, strlen(EXPECTED));

    free(str);

    PASS();
#undef EXPECTED
}

SUITE(jsmn_unescape)
{
    char *pairs[][2] = {
        { "áéíóú", "\\u00e1\\u00e9\\u00ed\\u00f3\\u00fa" },
        { "\"quote\"", "\"quote\"" },
        { "😊", "\\ud83d\\ude0a" },
        { "müller", "m\\u00fcller" },
        { "[\"áéíóú\",\"\"quote\"\",\"😊\",\"müller\"]",
          "[\"\\u00e1\\u00e9\\u00ed\\u00f3\\u00fa\",\"\\\"quote\\\"\","
          "\"\\ud83d\\ude0a\",\"m\\u00fcller\"]" },
    };
    size_t i;

    for (i = 0; i < sizeof(pairs) / sizeof *pairs; ++i)
        RUN_TEST1(check_unescaping, pairs + i);
    RUN_TEST(check_unescaping_with_threshold_length);
}

TEST
check_find(void)
{
    const char JSON[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";
    jsmn_parser parser;
    jsmntok_t toks[256];
    jsmnf_loader loader;
    jsmnf_pair pairs[256], *f;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    ASSERT(jsmn_parse(&parser, JSON, sizeof(JSON) - 1, toks,
                      sizeof(toks) / sizeof *toks)
           >= 0);
    ASSERT(jsmnf_load(&loader, JSON, toks, parser.toknext, pairs,
                      sizeof(pairs) / sizeof *pairs)
           >= 0);

    ASSERT((f = jsmnf_find(pairs, "foo", sizeof("foo") - 1)) != NULL);
    ASSERT_STRN_EQ("foo", f->key.contents, f->key.length);
    ASSERT((f = jsmnf_find(f, "bar", sizeof("bar") - 1)) != NULL);
    ASSERT_STRN_EQ("bar", f->key.contents, f->key.length);
    ASSERT((f = jsmnf_find(f, "baz", sizeof("baz") - 1)) != NULL);
    ASSERT_STRN_EQ("baz", f->key.contents, f->key.length);
    ASSERT((f = jsmnf_find(f, "0", sizeof("0") - 1)) != NULL);
    ASSERT_EQ(0, f->key.length);
    ASSERT_STRN_EQ("true", f->value.contents, f->value.length);

    PASS();
}

TEST
check_find_path(void)
{
    const char JSON[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";
    char *path1[] = { "foo" };
    char *path2[] = { "foo", "bar" };
    char *path3[] = { "foo", "bar", "baz" };
    char *path4[] = { "foo", "bar", "baz", "0" };
    jsmn_parser parser;
    jsmntok_t toks[256];
    jsmnf_loader loader;
    jsmnf_pair pairs[256], *f;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    ASSERT(jsmn_parse(&parser, JSON, sizeof(JSON) - 1, toks,
                      sizeof(toks) / sizeof *toks)
           >= 0);
    ASSERT(jsmnf_load(&loader, JSON, toks, parser.toknext, pairs,
                      sizeof(pairs) / sizeof *pairs)
           >= 0);

    ASSERT_NEQ(NULL, f = jsmnf_find_path(pairs, path1,
                                         sizeof(path1) / sizeof *path1));
    ASSERT_STRN_EQ(path1[0], f->key.contents, f->key.length);
    ASSERT_NEQ(NULL, f = jsmnf_find_path(pairs, path2,
                                         sizeof(path2) / sizeof *path2));
    ASSERT_STRN_EQ(path2[1], f->key.contents, f->key.length);
    ASSERT_NEQ(NULL, f = jsmnf_find_path(pairs, path3,
                                         sizeof(path3) / sizeof *path3));
    ASSERT_STRN_EQ(path3[2], f->key.contents, f->key.length);
    ASSERT_NEQ(NULL, f = jsmnf_find_path(pairs, path4,
                                         sizeof(path4) / sizeof *path4));
    ASSERT_EQ(0, f->key.length);
    ASSERT_STRN_EQ("true", f->value.contents, f->value.length);

    PASS();
}

SUITE(jsmn_find)
{
    RUN_TEST(check_find);
    RUN_TEST(check_find_path);
}

GREATEST_MAIN_DEFS();

int
main(int argc, char *argv[])
{
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(jsmn_unescape);
    RUN_SUITE(jsmn_find);

    GREATEST_MAIN_END();
}
