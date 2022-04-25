# JSMN-FIND

jsmn-find is a ANSI C add-on for the minimalistic JSON tokenizer
[jsmn](https://github.com/zserge/jsmn), it offers a familiar and
zero-allocation design. Its serializer counterpart counterpart can be found
at [json-build](https://github.com/lcsmuller/json-build).

## Dependencies

* [jsmn](https://github.com/zserge/jsmn) - Minimalistic JSON parser

## Included Dependencies

* chash - Type-safe, stackful hashtable made by [antropez](https://github.com/antropez)

## Design

jsmn-find organizes jsmn's JSON tokens under a hashtable, so that they can be
searched for in linear time.

## Usage

Download `jsmn-find.h` and the [dependencies](#dependencies) should be visible 
to your linker search path:

`load`
```c
#include "jsmn.h"
#include "jsmn-find.h"

...
jsmn_parser parser;
jsmntok_t tokens[256];

jsmn_init(&parser);

r = jsmn_parse(&parser, json, strlen(json), tokens, 256);
if (r < 0) error();

// populate jsmnf_pairs with the jsmn tokens
jsmnf_loader loader;
jsmnf_pairs pairs[256];

jsmnf_init(&loader);
r = jsmnf_load(&loader, json, tokens, parser.toknext, pairs, 256);
if (r < 0) error();
```

`find (key search for objects and arrays)`
```c
jsmnf_pair *f;

// assume the JSON : { "foo": { "bar": [ true, null, null ] } }
if ((f = jsmnf_find(pairs, json, "foo", strlen("foo")))) {
    // Found: { "bar" : [ true, null, null ] }
    printf("Found: %.*s\n", f->value.end - f->value.start, json + f->value.start);
    if ((f = jsmnf_find(f, json, "bar", 3))) {
        // Found: [ true, null, null ]
        printf("Found: %.*s\n", f->value.end - f->value.start, json + f->value.start);
        if ((f = jsmn_find(f, json, "0", 1))) {
            // Found: true
            printf("Found: %.*s\n", f->value.end - f->value.start, json + f->value.start);
        }
    }
}
```

`find (index search arrays)`
```c
jsmnf_pair *f;

// assume the JSON : [ null, [ true, null, null ] ]
f = &pairs->buckets[1];
// Found: [ true, null, null ]
printf("Found: %.*s\n", f->value.end - f->value.start, json + f->value.start);
f = &f->buckets[0];
// Found: true
printf("Found: %.*s\n", f->value.end - f->value.start, json + f->value.start);
```

`find path (key search for objects and arrays)`
```c
char *path[] = { "2", "1", "0", "b" };
jsmnf_pair *f;

// assume the JSON : [ 1, 2, [ 1, [ { "b":true } ] ] ]
if ((f = jsmnf_find_path(pairs, path, sizeof(path) / sizeof *path))) {
    // Found: true
    printf("Found: %.*s\n", f->value.end - f->value.start, json + f->value.start);
}
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

## API

* `jsmnf_init()` - initialize a `jsmnf_loader`
* `jsmnf_load()` - populate `jsmnf_pair` array with JSMN tokens
* `jsmnf_find()` - locate a `jsmnf_pair` by its associated key
* `jsmnf_find_path()` - locate a `jsmnf_pair` by its full key path

### Misc

* `jsmn_parse_auto()` - `jsmn_parse()` counterpart that allocates the necessary amount of tokens
* `jsmnf_load_auto()` - `jsmn_load()` counterpart that allocates the necessary amount of pairs
* `jsmnf_unescape()` - unescape a Unicode string

## Other Info

This software is distributed under [MIT license](www.opensource.org/licenses/mit-license.php),
so feel free to integrate it in your commercial products.
