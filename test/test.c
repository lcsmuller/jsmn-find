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
    jsmnf *root, *f;
    int ret;

    root = jsmnf_init();

    ret = jsmnf_start(root, JSON, sizeof(JSON) - 1);
    printf("Exit with %d\n", ret);

    f = jsmnf_find_path(root, path, sizeof(path) / sizeof(char *));
    if (f)
        fprintf(stderr, "Found: %.*s\n", f->val->end - f->val->start,
                JSON + f->val->start);
    else
        fprintf(stderr, "Couldn't locate field\n");

    f = jsmnf_find(root, "foo", sizeof("foo") - 1);
    f = jsmnf_find(f, "bar", sizeof("bar") - 1);
    f = jsmnf_find(f, "baz", sizeof("baz") - 1);
    f = jsmnf_find(f, "0", sizeof("0") - 1);
    if (f)
        fprintf(stderr, "Found: %.*s\n", f->val->end - f->val->start,
                JSON + f->val->start);
    else
        fprintf(stderr, "Couldn't locate field\n");

    jsmnf_cleanup(root);

    return EXIT_SUCCESS;
}
