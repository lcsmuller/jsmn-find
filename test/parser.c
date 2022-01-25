#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

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

static char *
load_whole_file(char *filename, long *p_fsize)
{
    FILE *f = fopen(filename, "rb");
    assert(NULL != f && "Couldn't open file");

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *str = malloc(fsize + 1);

    str[fsize] = '\0';
    fread(str, 1, fsize, f);

    fclose(f);

    if (p_fsize) *p_fsize = fsize;

    return str;
}

TEST
check_parser(char str[], long len, enum action expected)
{
    static char errbuf[2048];

    pid_t pid;

    pid = fork();
    if (pid < 0) {
        snprintf(errbuf, sizeof(errbuf), "%s", strerror(errno));
        SKIPm(errbuf);
    }

    if (0 == pid) { // child process
        jsmnfind *handle = jsmnfind_init();
        int ret;

        ret = jsmnfind_start(handle, str, len);

        jsmnfind_cleanup(handle);

        _exit(ret >= 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    int status;
    wait(&status);
    if (!WIFEXITED(status)) { // child process crashed
        snprintf(errbuf, sizeof(errbuf), "Process crashed, JSON: %.*s",
                 (int)len, str);
        FAILm(errbuf);
    }

    enum action action;
    if (EXIT_SUCCESS == WEXITSTATUS(status))
        action = ACTION_ACCEPT;
    else
        action = ACTION_REJECT;

    if (!expected || action & expected) PASS();

    snprintf(errbuf, sizeof(errbuf), "JSON: %.*s", (int)len, str);
    FAILm(errbuf);
}

SUITE(json_parsing)
{
    char *jsonstr;
    long jsonlen;

    for (int i = 0; i < g_n_files; ++i) {
        jsonstr = load_whole_file(g_files[i], &jsonlen);

        enum action expected;
        switch (g_suffixes[i][0]) {
        case 'y':
            expected = ACTION_ACCEPT;
            break;
        case 'n':
            expected = ACTION_REJECT;
            break;
        case 'i':
            expected = ACTION_ACCEPT | ACTION_REJECT;
            break;
        default:
            fprintf(stderr,
                    "Error: File '%s' not conforming to "
                    "github.com/nst/JSONTestSuite",
                    g_files[i]);
            exit(EXIT_FAILURE);
        }

        greatest_set_test_suffix(g_suffixes[i]);
        RUN_TESTp(check_parser, jsonstr, jsonlen, expected);

        free(jsonstr);
    }
}

SUITE(json_transform)
{
    char *jsonstr;
    long jsonlen;

    for (int i = 0; i < g_n_files; ++i) {
        jsonstr = load_whole_file(g_files[i], &jsonlen);

        greatest_set_test_suffix(g_suffixes[i]);
        RUN_TESTp(check_parser, jsonstr, jsonlen, ACTION_NONE);

        free(jsonstr);
    }
}

GREATEST_MAIN_DEFS();

int
main(int argc, char *argv[])
{
    GREATEST_MAIN_BEGIN();

    for (int i = 0; i < argc; ++i) {
        // we're assuming the files are after the "--" arg
        if (0 == strcmp("--", argv[i]) && (i + 1 < argc)) {
            g_files = argv + (i + 1);
            g_n_files = argc - (i + 1);
            break;
        }
    }
    assert(g_n_files != 0 && "Couldn't locate files");

    // create test suffixes for easy identification
    g_suffixes = malloc(g_n_files * sizeof(char *));
    char *start, *end;
    for (int i = 0; i < g_n_files; ++i) {
        if ((start = strchr(g_files[i], '/')))
            ++start;
        else
            start = g_files[i];
        end = strrchr(start, '.');

        size_t size = end ? (end - start) : strlen(start);
        g_suffixes[i] = malloc(size + 1);
        memcpy(g_suffixes[i], start, size);
        g_suffixes[i][size] = '\0';
    }

    RUN_SUITE(json_parsing);
    RUN_SUITE(json_transform);

    GREATEST_MAIN_END();
}
