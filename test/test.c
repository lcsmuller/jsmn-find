#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "jsmn.h"
#include "jsmn-find.h"

const char JSON[] = "{\"foo\":{\"bar\":{\"baz\":[true]}}}";

int
main(void)
{
    char *path[] = { "foo", "bar", "baz", "0" };
    jsmnfind *root, *f;
    int ret;

    root = jsmnfind_init();

    ret = jsmnfind_start(root, JSON, sizeof(JSON)-1);
    printf("Exit with %d\n", ret);

    f = jsmnfind_find_path(root, path, sizeof(path) / sizeof(char *));
    if (f)
        fprintf(stderr, "Found: %.*s\n", f->val->end - f->val->start,
                JSON + f->val->start);
    else
        fprintf(stderr, "Couldn't locate field\n");

    f = jsmnfind_find(root, "foo");
    f = jsmnfind_find(f, "bar");
    f = jsmnfind_find(f, "baz");
    f = jsmnfind_find(f, "0");
    if (f)
        fprintf(stderr, "Found: %.*s\n", f->val->end - f->val->start,
                JSON + f->val->start);
    else
        fprintf(stderr, "Couldn't locate field\n");

    jsmnfind_cleanup(root);

    return EXIT_SUCCESS;
}
