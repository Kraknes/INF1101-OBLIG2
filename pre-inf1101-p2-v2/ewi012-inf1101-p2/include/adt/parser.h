#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>


typedef struct index index_t;
typedef struct set set_t;
typedef struct entry entry_t;
typedef struct set_iter set_iter_t;
typedef struct list_iter list_iter_t;
typedef struct query_result query_result_t;

// Our types
typedef struct parser_tree p_tree_t;
typedef struct parser_node p_node_t;
typedef struct parser_type p_type_t;

/**
 * Parser tree AST. Root is the top node of the tree.
 */
struct parser_tree {
    p_node_t *root;
};

/**
 * Parser node for parser tree AST. 
 * Token type is a boolean indicator for what term type the word is.
 * Item is the word itself.
 * Left and right are the left and right child nodes of the tree.
 */
struct parser_node{
    p_type_t *token_type;
    char *item;
    struct parser_node *left;
    struct parser_node *right;
};

/**
 * Boolean indicator for what term type the word is in the parser node.
 * AND, OR, ANDNOT are operators. WORD is a word.
 * The boolean values are set as 1 or 0, whereas 1 is true and 0 is false. 
 */
struct parser_type{
    bool AND;
    bool OR;
    bool ANDNOT;
    bool WORD;
};


/*  
* Creating AST parser tree for list of query terms
*/
p_tree_t *ptree_create();

/* 
Creating node for AST tree. 
Checks the term if it is an operator or a word. 
Sets it as token_type 
*/
p_node_t *pnode_create(char *item);

/* 
* Fetching the set of document of word from reversed index. 
* Making a deep-copy of set to avoid deleting of original set. 
*/
set_t *fetch_docs(index_t *index, p_node_t *node, char* errmsg);

/* 
Return intersection "&&" on set from left and right node 
*/
set_t *ptree_intersection(index_t *index, p_node_t *left, p_node_t *right, char* errmsg);

/* 
Return union "||" on set from left and right node 
*/
set_t *ptree_union(index_t *index, p_node_t *left, p_node_t *right, char* errmsg);

/* 
Return difference "&!" on set from left and right node 
*/
set_t *ptree_difference(index_t *index, p_node_t *left, p_node_t *right, char* errmsg);

/* 
* Recursive set operation on nodes. Checks if node is a word or an operator (&&, &! or ||)
* Recursively goes through left and right nodes until leaves are met. 
* Return set operation on left and right nodes
*/
set_t *ptree_operation(index_t *index, p_node_t *node, char *errmsg);

/* 
* One sole Recursive query token parser function to AST tree. 
* Fetches first word of the linked list, and detects if its a start of a query "(". 
* The functions recursively initiate itself for each term.
* The returned node is the root of a subtree.
* Returns the final root for the whole AST tree by recursively uses the same function for every query
* 
 */
p_node_t *parse_query(list_iter_t *query_iter, char* errmsg);

/** 
 * Parent p_tree parsing function, initiates the parsing of the query tokens to AST tree.
 * */ 
void ptree_parsing(p_tree_t *p_tree, list_iter_t *query_iter, char* errmsg);

/* Own compare function for query_structs */
int cmp_doc_query(void *a, void *b);

/** 
 *  Creates a query_result struct.
 *  document name and frequency as arguments
 * */ 
query_result_t *create_query(char *doc_name, int freq);



#endif /* PARSER_H */