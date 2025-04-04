#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h> // for LINE_MAX

#include "printing.h"
#include "index.h"
#include "defs.h"
#include "common.h"
#include "list.h"
#include "map.h"
#include "set.h"


typedef struct parser_node node;
struct parser_node{
    char item;
    node *left;
    node *right;
};

struct parser_tree{
    node *root;
};
