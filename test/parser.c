#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#define JSMN_STRICT
#include "jsmn.h"
#include "jsmn-find.h"
#include "greatest.h"

static char **g_files;
static char **g_suffixes;
static int g_n_files;

enum action {
    ACTION_NONE = 0,
    ACTION_ACCEPT = 1 << 0,
    ACTION_REJECT = 1 << 2
};

struct context {
    char *str;
    long len;
    enum action expected;
};

static char *
load_whole_file(char *filename, long *p_fsize)
{
    long fsize;
    char *str;
    FILE *f;

    f = fopen(filename, "rb");
    assert(NULL != f && "Couldn't open file");

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    str = malloc(fsize + 1);

    str[fsize] = '\0';
    fread(str, 1, fsize, f);

    fclose(f);

    if (p_fsize) *p_fsize = fsize;

    return str;
}

TEST
check_parser(struct context *cxt)
{
    static char errbuf[2048];

    int jsonlen = (cxt->len < (int)sizeof(errbuf) - 1)
                      ? cxt->len
                      : (int)sizeof(errbuf) - 1;
    enum action action;
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) {
        sprintf(errbuf, "%.*s", (int)sizeof(errbuf) - 1, strerror(errno));
        SKIPm(errbuf);
    }

    if (0 == pid) { /* child process */
        jsmnf *root = jsmnf_init();
        int ret;

        ret = jsmnf_start(root, cxt->str, cxt->len);

        jsmnf_cleanup(root);

        _exit(ret >= 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    wait(&status);
    if (!WIFEXITED(status)) { /* child process crashed */
        sprintf(errbuf, "Process crashed, JSON: %.*s", jsonlen, cxt->str);
        FAILm(errbuf);
    }

    if (EXIT_SUCCESS == WEXITSTATUS(status))
        action = ACTION_ACCEPT;
    else
        action = ACTION_REJECT;

    if (!cxt->expected || action & cxt->expected) PASS();

    sprintf(errbuf, "JSON: %.*s", jsonlen, cxt->str);
    FAILm(errbuf);
}

SUITE(json_parsing)
{
    struct context cxt = { 0 };
    int i;

    for (i = 0; i < g_n_files; ++i) {
        cxt.str = load_whole_file(g_files[i], &cxt.len);

        switch (g_suffixes[i][0]) {
        case 'y':
            cxt.expected = ACTION_ACCEPT;
            break;
        case 'n':
            cxt.expected = ACTION_REJECT;
            break;
        case 'i':
            cxt.expected = ACTION_ACCEPT | ACTION_REJECT;
            break;
        default:
            fprintf(stderr,
                    "Error: File '%s' not conforming to "
                    "github.com/nst/JSONTestSuite",
                    g_files[i]);
            exit(EXIT_FAILURE);
        }

        greatest_set_test_suffix(g_suffixes[i]);
        RUN_TEST1(check_parser, &cxt);

        free(cxt.str);
    }
}

SUITE(json_transform)
{
    struct context cxt = { 0 };
    int i;

    cxt.expected = ACTION_NONE;

    for (i = 0; i < g_n_files; ++i) {
        cxt.str = load_whole_file(g_files[i], &cxt.len);

        greatest_set_test_suffix(g_suffixes[i]);
        RUN_TEST1(check_parser, &cxt);

        free(cxt.str);
    }
}

TEST
check_unescaping(void)
{
    char *pairs[][2] = {
        { "áéíóú", "\\u00e1\\u00e9\\u00ed\\u00f3\\u00fa" },
        { "\"quote\"", "\"quote\"" },
        { "😊", "\\ud83d\\ude0a" },
        { "müller", "m\\u00fcller" },
        { "müller", "m\\u00fcller" },
    };
    size_t i;

    for (i = 0; i < sizeof(pairs) / sizeof *pairs; ++i) {
        char *str = NULL;

        jsmnf_unescape(&str, pairs[i][1], strlen(pairs[i][1]));

        ASSERT(str != NULL);
        ASSERT_STR_EQ(pairs[i][0], str);
    }

    PASS();
}

SUITE(json_unescape)
{
    RUN_TEST(check_unescaping);
}

GREATEST_MAIN_DEFS();

int
main(int argc, char *argv[])
{
    char *start, *end;
    int i;

    GREATEST_MAIN_BEGIN();

    for (i = 0; i < argc; ++i) {
        /* we're assuming the files are after the "--" arg */
        if (0 == strcmp("--", argv[i]) && (i + 1 < argc)) {
            g_files = argv + (i + 1);
            g_n_files = argc - (i + 1);
            break;
        }
    }

    if (g_n_files) {
        /* create test suffixes for easy identification */
        g_suffixes = malloc(g_n_files * sizeof(char *));
        for (i = 0; i < g_n_files; ++i) {
            size_t size;

            if ((start = strchr(g_files[i], '/')))
                ++start;
            else
                start = g_files[i];
            end = strrchr(start, '.');

            size = end ? (size_t)(end - start) : strlen(start);

            g_suffixes[i] = malloc(size + 1);
            memcpy(g_suffixes[i], start, size);
            g_suffixes[i][size] = '\0';
        }

        RUN_SUITE(json_parsing);
        RUN_SUITE(json_transform);
    }
    RUN_SUITE(json_unescape);

    GREATEST_MAIN_END();
}
