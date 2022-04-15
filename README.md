JSMN-FIND
=========

jsmn-find is an add-on for the popular minimalistic JSON parser 
[jsmn](https://github.com/zserge/jsmn), it strives to keep a familiar and
zero-allocation design.

Dependencies
------------

* [jsmn](https://github.com/zserge/jsmn) - Minimalistic JSON parser

Included Dependencies
---------------------

* chash - Type-safe, stackful hashtable made by [antropez](https://github.com/antropez)

Design
------

jsmn-find organizes jsmn's JSON tokens under a hashtable, so they can be
searched for.

Usage
-----

Download `jsmn-find.h` and the [dependencies](#dependencies) should be visible 
to your linker search path:

```c
#include "jsmn.h"
#include "jsmn-find.h"

...
// parse jsmn tokens first
jsmn_parser parser;
jsmntok_t tokens[256];

jsmn_init(&parser);

r = jsmn_parse(&parser, json, strlen(json), tokens, 256);
if (r <= 0) error();

// populate jsmnf_pairs with the jsmn tokens
jsmnf_loader loader;
jsmnf_pairs pairs[256];

jsmnf_init(&loader);

r = jsmnf_load(&loader, json, tokens, parser.toknext, pairs, 256);
if (r <= 0) error();

// assume the JSON : { "foo": { "bar": [ 1, 2, 3 ] } }
jsmnf_pair *f = jsmnf_find(pairs, "foo", strlen("foo"));
// Found: { "bar" : [ 1, 2, 3 ] }
printf("Found: %.*s\n", f->value.length, f->value.contents);
...
// assume the JSON : [ 1, 2, [ 1, [ { "b":true } ] ] ]
char *path[] = { "2", "1", "0", "b" };
jsmnf_pair *f = jsmnf_find_path(pairs, path, sizeof(path) / sizeof(char *));
// Found: true
printf("Found: %.*s\n", f->value.length, f->value.contents);
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

* `jsmnf_init()` - initialize a `jsmnf_loader`
* `jsmnf_load()` - populate `jsmnf_pair` array with JSMN tokens
* `jsmnf_find()` - locate a `jsmnf_pair` by its associated key
* `jsmnf_find_path()` - locate a `jsmnf_pair` by its full key path
* `jsmnf_unescape()` - unescape a Unicode string

More Documentation
------------------

Read jsmn documentation for additional information:
[jsmn web page](http://zserge.com/jsmn.html)

Other Info
----------

This software is distributed under [MIT license](www.opensource.org/licenses/mit-license.php),
so feel free to integrate it in your commercial products.
