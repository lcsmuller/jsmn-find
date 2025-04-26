#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
check_not_corrupted(void)
{
    const char json[] =
        "{\"status\":\"enabled\",\"timestamp\":\"2023-08-24T07:59:03Z\"}\n";
    jsmn_parser parser;
    jsmntok_t *toks = NULL;
    unsigned num_tokens = 0;
    long ret;

    jsmn_init(&parser);

    ret = jsmn_parse_auto(&parser, json, strlen(json), &toks, &num_tokens);
    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmn_parse_auto(&parser, json, sizeof(json) - 1, &toks,
                                     &num_tokens),
               0);

    PASS();
}

SUITE(fn__jsmn_load_auto)
{
    RUN_TEST(check_not_corrupted);
}

TEST
check_load_dynamic_pairs(void)
{
    const char js_small[] = "{\"foo\":[true]}";
    const char js_large[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";
    jsmnf_loader loader;
    jsmnf_table *table = NULL;
    const jsmnf_pair *f;
    size_t num_pairs = 0;
    long ret;

    jsmnf_init(&loader);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load_auto(&loader, js_small, sizeof(js_small) - 1,
                                     &table, &num_pairs),
               0);

    ASSERT_NEQ(NULL, f = jsmnf_find(loader.root, "foo", 3));
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "0", 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0,
               f->k->end - f->k->start);
    ASSERT_STRN_EQ("true", js_small + f->v->start, f->v->end - f->v->start);

    jsmnf_init(&loader);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load_auto(&loader, js_large, sizeof(js_large) - 1,
                                     &table, &num_pairs),
               0);

    ASSERT_NEQ(NULL, f = jsmnf_find(loader.root, "foo", 3));
    ASSERT_STRN_EQ("foo", js_large + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "bar", 3));
    ASSERT_STRN_EQ("bar", js_large + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "baz", 3));
    ASSERT_STRN_EQ("baz", js_large + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "0", 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0,
               f->k->end - f->k->start);
    ASSERT_STRN_EQ("true", js_large + f->v->start, f->v->end - f->v->start);

    free(table);

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
    jsmnf_table *table = NULL;
    const jsmnf_pair *f;
    size_t num_pairs = 0;
    unsigned num_tokens = 0;
    long ret;

    jsmn_init(&parser);
    jsmnf_init(&loader);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmn_parse_auto(&parser, js_small, sizeof(js_small) - 1,
                                     &toks, &num_tokens),
               0);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load_auto(&loader, js_small, sizeof(js_small) - 1,
                                     &table, &num_pairs),
               0);

    ASSERT_NEQ(NULL, f = jsmnf_find(loader.root, "foo", 3));
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "0", 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0,
               f->k->end - f->k->start);
    ASSERT_STRN_EQ("true", js_small + f->v->start, f->v->end - f->v->start);

    jsmn_init(&parser);
    jsmnf_init(&loader);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmn_parse_auto(&parser, js_large, sizeof(js_large) - 1,
                                     &toks, &num_tokens),
               0);
    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load_auto(&loader, js_large, sizeof(js_large) - 1,
                                     &table, &num_pairs),
               0);

    ASSERT_NEQ(NULL, f = jsmnf_find(loader.root, "foo", 3));
    ASSERT_STRN_EQ("foo", js_large + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "bar", 3));
    ASSERT_STRN_EQ("bar", js_large + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "baz", 3));
    ASSERT_STRN_EQ("baz", js_large + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "0", 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0,
               f->k->end - f->k->start);
    ASSERT_STRN_EQ("true", js_large + f->v->start, f->v->end - f->v->start);

    free(toks);
    free(table);

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
    jsmnf_loader loader;
    jsmnf_table table[64];
    const jsmnf_pair *f;
    long ret;

    jsmnf_init(&loader);

    /* not enough pairs should return JSMN_ERROR_NOMEM */
    ASSERT_EQm(print_jsmnerr(ret), JSMN_ERROR_NOMEM,
               ret = jsmnf_load(&loader, js, sizeof(js) - 1, table, 1));
    /* simulate realloc */
    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load(&loader, js, sizeof(js) - 1, table,
                                sizeof(table) / sizeof *table),
               0);

    /* check if searching still works */
    ASSERT_NEQ(NULL, f = jsmnf_find(loader.root, "foo", 3));
    ASSERT_STRN_EQ("foo", js + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "bar", 3));
    ASSERT_STRN_EQ("bar", js + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "baz", 3));
    ASSERT_STRN_EQ("baz", js + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "0", 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0,
               f->k->end - f->k->start);
    ASSERT_STRN_EQ("true", js + f->v->start, f->v->end - f->v->start);

    PASS();
}

TEST
check_load_basic(void)
{
    const char js[] = "{\"a\":1,\"c\":{}}";

    jsmnf_loader loader;
    jsmnf_table table[16];
    long ret;

    jsmnf_init(&loader);

    /* not enough pairs should return JSMN_ERROR_NOMEM */
    ASSERT_EQm(print_jsmnerr(ret), JSMN_ERROR_NOMEM,
               ret = jsmnf_load(&loader, js, sizeof(js) - 1, table,
                                sizeof(table) / sizeof *table / 2));
    /* simulate realloc */
    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load(&loader, js, sizeof(js) - 1, table,
                                sizeof(table) / sizeof *table),
               0);

    PASS();
}

TEST
check_load_array(void)
{
    const char js[] = "[1, [1, 2, [1, 2, 3, [true]]]]";
    jsmnf_loader loader;
    jsmnf_table table[64];
    const jsmnf_pair *f;
    long ret;

    jsmnf_init(&loader);

    ASSERT_GTm(print_jsmnerr(ret),
               ret = jsmnf_load(&loader, js, sizeof(js) - 1, table,
                                sizeof(table) / sizeof *table),
               0);

    ASSERT_EQ(2, loader.root->length);
    f = &loader.root->fields[1];
    ASSERT_EQ(3, f->length);
    f = &f->fields[2];
    ASSERT_EQ(4, f->length);
    f = &f->fields[3];
    ASSERT_EQ(1, f->length);

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
    jsmnf_loader loader;
    jsmnf_table table[64];
    const jsmnf_pair *f;

    jsmnf_init(&loader);
    jsmnf_load(&loader, js, sizeof(js) - 1, table,
               sizeof(table) / sizeof *table);

    ASSERT_NEQ(NULL, f = jsmnf_find(loader.root, "foo", 3));
    ASSERT_STRN_EQ("foo", js + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "bar", 3));
    ASSERT_STRN_EQ("bar", js + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "baz", 3));
    ASSERT_STRN_EQ("baz", js + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "0", 1));
    ASSERT_EQm("Array elements shouldn't have a key", 0,
               f->k->end - f->k->start);
    ASSERT_STRN_EQ("true", js + f->v->start, f->v->end - f->v->start);

    PASS();
}

TEST
check_find_array(void)
{
    const char js[] = "[1, [1, 2, [1, 2, 3, [true]]]]";
    jsmnf_loader loader;
    jsmnf_table table[64];
    const jsmnf_pair *f;

    jsmnf_init(&loader);
    jsmnf_load(&loader, js, sizeof(js) - 1, table,
               sizeof(table) / sizeof *table);

    /* test direct search */
    f = &loader.root->fields[0];
    ASSERT_STRN_EQ("1", js + f->v->start, f->v->end - f->v->start);
    f = &loader.root->fields[1];
    ASSERT_STRN_EQ("1", js + f->fields[0].v->start,
                   f->fields[0].v->end - f->fields[0].v->start);
    ASSERT_STRN_EQ("2", js + f->fields[1].v->start,
                   f->fields[1].v->end - f->fields[1].v->start);
    f = &f->fields[2];
    ASSERT_STRN_EQ("1", js + f->fields[0].v->start,
                   f->fields[0].v->end - f->fields[0].v->start);
    ASSERT_STRN_EQ("2", js + f->fields[1].v->start,
                   f->fields[1].v->end - f->fields[1].v->start);
    ASSERT_STRN_EQ("3", js + f->fields[2].v->start,
                   f->fields[2].v->end - f->fields[2].v->start);
    f = &f->fields[3];
    ASSERT_STRN_EQ("[true]", js + f->v->start, f->v->end - f->v->start);

    /* test key search */
    ASSERT_NEQ(NULL, f = jsmnf_find(loader.root, "0", 1));
    ASSERT_STRN_EQ("1", js + f->v->start, f->v->end - f->v->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(loader.root, "1", 1));
    ASSERT_STRN_EQ("1", js + f->fields[0].v->start,
                   f->fields[0].v->end - f->fields[0].v->start);
    ASSERT_STRN_EQ("2", js + f->fields[1].v->start,
                   f->fields[1].v->end - f->fields[1].v->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "2", 1));
    ASSERT_STRN_EQ("1", js + f->fields[0].v->start,
                   f->fields[0].v->end - f->fields[0].v->start);
    ASSERT_STRN_EQ("2", js + f->fields[1].v->start,
                   f->fields[1].v->end - f->fields[1].v->start);
    ASSERT_STRN_EQ("3", js + f->fields[2].v->start,
                   f->fields[2].v->end - f->fields[2].v->start);
    ASSERT_NEQ(NULL, f = jsmnf_find(f, "3", 1));
    ASSERT_STRN_EQ("[true]", js + f->v->start, f->v->end - f->v->start);

    PASS();
}

#define OBJ1_NEST                                                             \
    "{ \"username\": null, \"avatar\": null, \"avatar_decoration\": null, "   \
    "\"bar\": null, \"foo\": 1 }"
#define OBJ1 "{ \"foo\": " OBJ1_NEST " }"
#define OBJ2 "{ \"foo\": null }"
TEST
check_iterate_over_object_elements_in_array(void)
{
    const char js[] = "[ " OBJ1 " , " OBJ2 " ]";
    jsmnf_loader loader;
    jsmnf_table table[128];
    const jsmnf_pair *f1, *f2;

    jsmnf_init(&loader);
    jsmnf_load(&loader, js, sizeof(js) - 1, table,
               sizeof(table) / sizeof *table);

    f1 = &loader.root->fields[0];
    ASSERT_STRN_EQ(OBJ1, js + f1->v->start, f1->v->end - f1->v->start);
    ASSERT_NEQ(NULL, f1 = jsmnf_find(f1, "foo", 3));
    ASSERT_STRN_EQ(OBJ1_NEST, js + f1->v->start, f1->v->end - f1->v->start);
    ASSERT_NEQ(NULL, f2 = jsmnf_find(f1, "bar", 3));
    ASSERT_NEQ(NULL, f2 = jsmnf_find(f1, "avatar", 6));
    ASSERT_STRN_EQ("null", js + f2->v->start, f2->v->end - f2->v->start);

    f1 = &loader.root->fields[1];
    ASSERT_STRN_EQ(OBJ2, js + f1->v->start, f1->v->end - f1->v->start);
    ASSERT_NEQ(NULL, f1 = jsmnf_find(f1, "foo", 3));
    ASSERT_STRN_EQ("null", js + f1->v->start, f1->v->end - f1->v->start);

    PASS();
}
#undef OBJ1_NEST
#undef OBJ1
#undef OBJ2

TEST
check_find_string_elements_in_array(void)
{
    const char js[] = "{\"modules\":[\"foo\",\"bar\",\"baz\",\"tuna\"]}";
    jsmnf_loader loader;
    jsmnf_table table[64];
    const jsmnf_pair *f1, *f2;

    jsmnf_init(&loader);
    jsmnf_load(&loader, js, sizeof(js) - 1, table,
               sizeof(table) / sizeof *table);

    ASSERT_NEQ(NULL, f1 = jsmnf_find(loader.root, "modules", 7));

    /* test direct search */
    f2 = &f1->fields[0];
    ASSERT_STRN_EQ("foo", js + f2->v->start, f2->v->end - f2->v->start);
    f2 = &f1->fields[1];
    ASSERT_STRN_EQ("bar", js + f2->v->start, f2->v->end - f2->v->start);
    f2 = &f1->fields[2];
    ASSERT_STRN_EQ("baz", js + f2->v->start, f2->v->end - f2->v->start);
    f2 = &f1->fields[3];
    ASSERT_STRN_EQ("tuna", js + f2->v->start, f2->v->end - f2->v->start);

    PASS();
}

SUITE(fn__jsmnf_find)
{
    RUN_TEST(check_find_nested);
    RUN_TEST(check_find_array);
    RUN_TEST(check_find_string_elements_in_array);
    RUN_TEST(check_iterate_over_object_elements_in_array);
}

TEST
check_find_path_nested(void)
{
    const char js[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";
    char *path[] = { "foo", "bar", "baz", "0" };
    jsmnf_loader loader;
    jsmnf_table table[64];
    const jsmnf_pair *f;

    jsmnf_init(&loader);
    jsmnf_load(&loader, js, sizeof(js) - 1, table,
               sizeof(table) / sizeof *table);

    ASSERT_NEQ(NULL, f = jsmnf_find_path(loader.root, path, 1));
    ASSERT_STRN_EQ(path[0], js + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find_path(loader.root, path, 2));
    ASSERT_STRN_EQ(path[1], js + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find_path(loader.root, path, 3));
    ASSERT_STRN_EQ(path[2], js + f->k->start, f->k->end - f->k->start);
    ASSERT_NEQ(NULL, f = jsmnf_find_path(loader.root, path, 4));
    ASSERT_EQm("Array elements shouldn't have a key", 0,
               f->k->end - f->k->start);
    ASSERT_STRN_EQ("true", js + f->v->start, f->v->end - f->v->start);

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
