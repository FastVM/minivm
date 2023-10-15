If you're interested in contributing to this code this is where you'd learn.
Essentially this is the builtin linker, it can convert TB modules, external
objects and libraries into executables. Let's walk through making a dummy
linker.

First, create a new source file (dummy.c) in the linker directory. We're gonna
populate it with our dummy vtable (this is how the generic linker piece calls
into the format-specific details):

```
// in dummy.c
#define NL_STRING_MAP_IMPL
#include "linker.h"

static void append_object(TB_Linker* l, TB_ObjectFile* obj) {
    // implement this
}

static void append_library(TB_Linker* l, TB_Slice ar_file) {
    // implement this
}

static void append_module(TB_Linker* l, TB_Module* m) {
    // implement this
}

static TB_Exports export(TB_Linker* l) {
    // implement this
    return (TB_Exports){ 0 };
}

TB_LinkerVtbl tb__linker_dummy = {
    .append_object  = append_object,
    .append_library = append_library,
    .append_module  = append_module,
    .export         = export
};
```

We can start with stubbing out `header`
