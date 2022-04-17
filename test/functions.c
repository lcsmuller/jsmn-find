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
        if (code == 0) return "Empty string";
        if (code > 0) return "No error encountered";
        return "Unknown error";
    }
}

TEST
check_unescaping(void *p_test)
{
    char *(*test)[3] = p_test;
    char buf[256] = "";
    long ret;

    ASSERT_GTEm(
        print_jsmnerr(ret),
        ret = jsmnf_unescape(buf, sizeof(buf), (*test)[1], strlen((*test)[1])),
        0);

    ASSERT_STR_EQm((*test)[2], (*test)[0], buf);

    PASS();
}

TEST
check_unescaping_not_enough_buffer_memory(void)
{
    char src[] =
        "\\u00e1\\u00e9\\u00ed\\u00f3\\u00fa\\u00c1\\u00c9\\u00cd\\u00d3\\u0000";
    char buf[32];
    long ret;

    ASSERT_EQm(print_jsmnerr(ret), JSMN_ERROR_NOMEM,
               ret = jsmnf_unescape(buf, 4, src, sizeof(src)));
    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_unescape(buf, sizeof(buf), src, sizeof(src)), 0);

    PASS();
}

SUITE(jsmn_unescape)
{
    /* [0]: expected out ; [1]: input string ; [2]: test meaning */
    char *accept[][3] = {
        { "\\\"", "\\\\\\\"", "escaped escape" },
        { "\\", "\\\\", "escaped escape" },
        { "☺☺", "\\u263A\\u263A", "unicode sequence" },
        { "abc", "abc", "bytes ascii" },
#if TODO
        /* support octal */
        { "Ã¿", "\303\277", "multi octal sequence" },
        { "ÿ", "\377", "octal sequence" },
        /* unescape commas recursively */
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
