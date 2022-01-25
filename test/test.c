#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsmn.h"
#include "jsmn-find.h"

char *
cog_load_whole_file_fp(FILE *fp, size_t *len)
{
    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *str = malloc(fsize + 1);

    str[fsize] = '\0';
    fsize = fread(str, 1, fsize, fp);
    if (!fsize) {
        free(str);
        str = NULL;
    }

    if (len) *len = fsize;

    return str;
}

char *
cog_load_whole_file(const char filename[], size_t *len)
{
    FILE *fp = fopen(filename, "rb");
    char *str = cog_load_whole_file_fp(fp, len);
    fclose(fp);
    return str;
}

int
main(int argc, char *argv[])
{
    char *path[] = { "a", "b", "c" };

    if (argc == 1) return EXIT_FAILURE;

    size_t len;
    char *json = cog_load_whole_file(argv[1], &len);

    jsmnfind *handle = jsmnfind_init();
    int ret;

    ret = jsmnfind_start(handle, json, len);
    printf("Exit with %d\n", ret);

    jsmntok_t *tok = jsmnfind_find(handle, path, sizeof(path) / sizeof(char *));
    fprintf(stderr, "Found: %.*s\n", tok->end - tok->start, json + tok->start);

    jsmnfind_cleanup(handle);
    free(json);

    return EXIT_SUCCESS;
}
