#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#define JSMN_STRICT
#include "jsmn_1.1.0.h"
#include "jsmn-find.h"
#include "greatest.h"

static char **g_files;
static char **g_suffixes;
static int g_n_files;

enum action {
    ACTION_NONE = 0,
    ACTION_ACCEPT = 1 << 0,
    ACTION_REJECT = 1 << 1
};

struct context {
    char *str;
    size_t len;
    enum action expected;
};

static char *
load_whole_file(char *filename, size_t *p_fsize)
{
    size_t fsize;
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
check_parser(const struct context *const cxt)
{
    static char errbuf[0x1000];
    const size_t jsonlen =
        (cxt->len < sizeof(errbuf) - 1) ? cxt->len : sizeof(errbuf) - 1;
    enum action action;
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {
        sprintf(errbuf, "%.*s", (int)sizeof(errbuf) - 1, strerror(errno));
        SKIPm(errbuf);
    }
    if (0 == pid) { /* child process */
        jsmnf_table table[256];
        jsmnf_loader loader;

        jsmnf_init(&loader);
        _exit((jsmnf_load(&loader, cxt->str, cxt->len, table,
                          sizeof(table) / sizeof *table)
               > 0)
                  ? EXIT_SUCCESS
                  : EXIT_FAILURE);
    }

    wait(&status);
    if (!WIFEXITED(status)) { /* child process crashed */
        sprintf(errbuf, "Process crashed, JSON: %.*s", (int)jsonlen, cxt->str);
        FAILm(errbuf);
    }
    action =
        WEXITSTATUS(status) == EXIT_SUCCESS ? ACTION_ACCEPT : ACTION_REJECT;
    if (cxt->expected != ACTION_NONE && !(action & cxt->expected)) {
        sprintf(errbuf, "expected: %d | got: %d\nJSON: %.*s", cxt->expected,
                action, (int)jsonlen, cxt->str);
        FAILm(errbuf);
    }
    PASS();
}

SUITE(json_parsing)
{
    struct context cxt = { 0 };
    int i;

    for (i = 0; i < g_n_files; ++i) {
        cxt.str = load_whole_file(g_files[i], &cxt.len);

        if (g_suffixes[i][1] != '_') {
            fprintf(stderr,
                    "Error: File '%s' not conforming to "
                    "github.com/nst/JSONTestSuite",
                    g_files[i]);
            exit(EXIT_FAILURE);
        }

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
    assert(g_n_files != 0 && "Missing JSON files");

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

    GREATEST_MAIN_END();
}
