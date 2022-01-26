#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "jsmn.h"
#include "jsmn-find.h"

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

int
main(int argc, char *argv[])
{
    char *path[] = { "1", "0", "a" };
    jsmnfind *handle;
    jsmntok_t *tok;
    char *json;
    long len;
    int ret;

    if (argc == 1) return EXIT_FAILURE;

    json = load_whole_file(argv[1], &len);

    handle = jsmnfind_init();

    ret = jsmnfind_start(handle, json, len);
    printf("Exit with %d\n", ret);

    tok = jsmnfind_find(handle, path, sizeof(path) / sizeof(char *));
    if (tok)
        fprintf(stderr, "Found: %.*s\n", tok->end - tok->start,
                json + tok->start);
    else
        fprintf(stderr, "Couldn't locate field\n");

    jsmnfind_cleanup(handle);
    free(json);

    return EXIT_SUCCESS;
}
