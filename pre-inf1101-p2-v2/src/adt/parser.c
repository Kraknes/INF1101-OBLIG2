#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h> // for LINE_MAX
#include <stddef.h> // for size_t

#include "printing.h"
#include "index.h"
// #include "defs.h"
// #include "common.h"
// #include "list.h"
// #include "map.h"
// #include "set.h"
// #include "parser.h"


p_tree *p_tree_create(cmp_fn cmpfn) {
    p_tree *ast = calloc(1, sizeof(p_tree));
    if (!ast){
        PANIC("No memory created for p_tree");
    }
    ast->cmpfn = cmpfn;
    return ast;
}

