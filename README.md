JSMN-FIND
=========

jsmn-find is an add-on for the popular minimalistic JSON parser 
[jsmn](https://github.com/zserge/jsmn).

Included Dependencies
---------------------

[jsmn](https://github.com/zserge/jsmn) - Minimalistic JSON parser
[uthash](https://github.com/troydhanson/uthash) - Macro-based hashtable

Design
------

jsmn-find organizes jsmn's JSON tokens under a hashtable, so they can be
searched for.

Usage
-----

Download `jsmn-find.h` and the [dependencies](#included-dependencies) should be visible to your linker search path:

```c
#include "jsmn.h"
#include "jsmn-find.h"

...
jsmnfind *f = jsmn_init();

r = jsmnfind_start(f, json, strlen(json)); // "s" is the char array holding the json content

// assume the JSON : { "foo": { "bar": [ 1, 2, 3] } }
char *path[] = { "foo", "bar", "1" };
jsmntok_t *tok = jsmnfind_find(f, path, sizeof(path)/ sizeof(char *));
printf("Found: %.*s\n", tok->end - tok->start, json + tok->start); // Found: 2
```

jsmn-find is single-header and should be compatible with jsmn additional macros for more complex uses cases. `#define JSMN_STATIC` hides all jsmn-find API symbols by making them static. Also, if you want to include `jsmn-find.h` from multiplce C files, to avoid duplication of symbols you may define `JSMN_HEADER` macro.

```c
/* In every .c file that uses jsmn include only declarations: */
#define JSMN_HEADER
#include "jsmn.h"
#include "jsmn-find.h"

/* Additionally, create one jsmn-find.c file for jsmn-find implementation: */
#include "jsmn.h"
#include "jsmn-find.h"
```

More Documentation
------------------

Read jsmn documentation for additional information:
[jsmn web page](http://zserge.com/jsmn.html)

Other Info
----------

This software is distributed under MIT license, so feel free to integrate it in your commercial products.

