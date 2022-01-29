JSMN-FIND
=========

jsmn-find is an add-on for the popular minimalistic JSON parser 
[jsmn](https://github.com/zserge/jsmn).

Dependencies
------------

* [jsmn](https://github.com/zserge/jsmn) - Minimalistic JSON parser
* [uthash](https://github.com/troydhanson/uthash) - Macro-based hashtable

Design
------

jsmn-find organizes jsmn's JSON tokens under a hashtable, so they can be
searched for.

Usage
-----

Download `jsmn-find.h` and the [dependencies](#dependencies) should be visible to your linker search path:

```c
#include "jsmn.h"
#include "jsmn-find.h"

...
jsmnfind *root = jsmnfind_init();

r = jsmnfind_start(root, json, strlen(json));

// assume the JSON : { "foo": { "bar": [ 1, 2, 3 ] } }
jsmnfind *f = jsmnfind_find(root, "foo", strlen("foo"));
// Found: { "bar" : [ 1, 2, 3 ] }
printf("Found: %.*s\n", f->val->end - f->val->start, json + f->val->start);
...
// assume the JSON : [ 1, 2, [ 1, [ { "b":true } ] ] ]
char *path[] = { "2", "1", "0", "b" };
jsmnfind *f = jsmnfind_find_path(root, path, sizeof(path) / sizeof(char *));
// Found: true
printf("Found: %.*s\n", f->val->end - f->val->start, json + f->val->start);
...
jsmnfind_cleanup(root); // don't forget to cleanup jsmnfind when you're done
```

jsmn-find is single-header and should be compatible with jsmn additional macros for more complex uses cases. `#define JSMN_STATIC` hides all jsmn-find API symbols by making them static. Also, if you want to include `jsmn-find.h` from multiple C files, to avoid duplication of symbols you may define `JSMN_HEADER` macro.

```c
/* In every .c file that uses jsmn include only declarations: */
#define JSMN_HEADER
#include "jsmn.h"
#include "jsmn-find.h"

/* Additionally, create one jsmn-find.c file for jsmn-find implementation: */
#include "jsmn.h"
#include "jsmn-find.h"
```

API
---

* `jsmnfind_init()` - initialize a jsmnfind root
* `jsmnfind_start()` - populate jsmnfind root with JSMN tokens
* `jsmnfind_cleanup()` - cleanup jsmnfind root resources
* `jsmnfind_find()` - locate a top JSMN token by its key
* `jsmnfind_find_path()` - locate a JSMN token by its key path

More Documentation
------------------

Read jsmn documentation for additional information:
[jsmn web page](http://zserge.com/jsmn.html)

Other Info
----------

This software is distributed under [MIT license](www.opensource.org/licenses/mit-license.php),
so feel free to integrate it in your commercial products.

