/**
 * @implements index.h
 */

/* set log level for prints in this file */
#define LOG_LEVEL LOG_LEVEL_DEBUG

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
#include "parser.h"

typedef struct tnode tnode_t;
struct tnode {
    // tnode_color_t color;
    void *elem;
    tnode_t *parent;
    tnode_t *left;
    tnode_t *right;
};

struct set {
    tnode_t *root;
    cmp_fn cmpfn;
    size_t length;
};


// Own doc_info struct //

set_t *ptree_operation(index_t *index, p_node_t *node, char* errmsg);


/**
 * You may utilize this for lists of query results, or write your own comparison function.
 */
ATTR_MAYBE_UNUSED
int compare_results_by_score(query_result_t *a, query_result_t *b) {
    if (a->score > b->score) {
        return -1;
    }
    if (a->score < b->score) {
        return 1;
    }
    return 0;
}

/**
 * @brief debug / helper to print a list of strings with a description.
 * Can safely be removed, but could be useful for debugging/development.
 *
 * Remove this function from your finished program once you are done
 */
ATTR_MAYBE_UNUSED
static void print_list_of_strings(const char *descr, list_t *tokens) {
    if (LOG_LEVEL <= LOG_LEVEL_INFO) {
        return;
    }
    list_iter_t *tokens_iter = list_createiter(tokens);
    if (!tokens_iter) {
        /* this is not a critical function, so just print an error and return. */
        pr_error("Failed to create iterator\n");
        return;
    }

    pr_info("\n%s:", descr);
    while (list_hasnext(tokens_iter)) {
        char *token = (char *) list_next(tokens_iter);
        pr_info("\"%s\", ", token);
    }
    pr_info("\n");

    list_destroyiter(tokens_iter);
}

index_t *index_create() {
    index_t *index = calloc(1, sizeof(index_t));
    if (index == NULL) {
        pr_error("Failed to allocate memory for index\n");
        return NULL;
    }

    /**
     * TODO: Allocate, initialize and set up nescessary structures
     */


    // ERLIGN IMPLEMENTASJON NEDENFOR

    index->hashmap = map_create((cmp_fn) strcmp, hash_string_fnv1a64);


    return index;
}

void index_destroy(index_t *index) {
    // during development, you can use the following macro to silence "unused variable" errors.

    /**
     * TODO: Free all memory associated with the index
     */
    free(index->hashmap);
    free(index);
}
int cmp_doc_query(void *a, void *b){
    query_result_t *aq = a;
    query_result_t *bq = b;
    return strcmp(aq->doc_name, bq->doc_name);
}

query_result_t *create_query(char *doc_name, int freq){
    query_result_t *query = calloc(1, sizeof(query_result_t));  
    if (!query){
        pr_error("Calloc of doc_i error, terminate indexing");
        return NULL;
    }
    query->doc_name = doc_name; // Adding the doc name 
    query->score = freq;

    return query;
}

int index_document(index_t *index, char *doc_name, list_t *terms) {
    
    /**
     * TODO: Process document, enabling the terms and subsequent document to be found by index_query
     *
     * Note: doc_name and the list of terms is now owned by the index. See the docstring.
     */

    // lager iternode av første instans av terms listen
    if (terms == NULL || doc_name == NULL){
        pr_error("terms or doc_name is null, terminate indexing");
        return -1;
    }
    index->num_docs++; //one for each document

    
    // Struct to put in as "val" in a node in linked-list. 
    // The linked-list is the "val" in a "entry" node inside "mnode"
    // "mnode" are the buckets of a hashmap

    //  MÅ GJØRES OM, MÅ HA EN HASHMAP ELLER LENKET LISTE I VAL FOR ENTRY
    //  så lenge det er noe i første noden, så fortsetter den gjennom listen

    list_iter_t *list_iter = list_createiter(terms); // A checker to see if there is a next node in the list
    while(list_hasnext(list_iter) != 0){ // If there is a node with a term
        list_next(list_iter);
        
        char *curr_term = list_popfirst(terms); // get the first node term item from the "terms" list
        
        entry_t *term_entry = map_get(index->hashmap, curr_term); // checking if the word is in the hashmap

        query_result_t *query_term = create_query(doc_name, 1); // making query to insertn in term entry

        // if nothing in the entry, inserting new term to hashmap 
        if (term_entry == NULL){
            set_t *term_set = set_create((cmp_fn) cmp_doc_query); // making a  set for popped term
            set_insert(term_set, query_term); // Adding document
            map_insert(index->hashmap, curr_term, term_set); // adding linked list as value to the popped "term" as key to hashmap
        }
        else{ // term_entry != NULL: The word/term exist, therefore, a set for doc_names is there as well.
            set_t *term_set = term_entry->val; // Accessing the document set of current term  
            if (set_get(term_set, query_term) != NULL){ // If document name is in the set, adds "score" to it for frequency
                query_result_t *query = set_get(term_set, query_term);
                query->score++;
            }
            else{
                set_insert(term_set, query_term); // if the document exist in term, then do nothing
            }
        }
    }
    return 0; // or -x on error
}

// ---------------- below are Parser Tree AST functions ---------------- 

p_tree_t *ptree_create(){ // Make AST parser tree
    p_tree_t *p_tree = calloc(1, sizeof(p_tree));
    return p_tree;
}

p_node_t *pnode_create(char *item){ // Creating node for parser-tree
    p_node_t *p_node = calloc(1, sizeof(p_node_t));
    p_node->token_type = calloc(1, sizeof(p_type_t));
    // p_node->item = NULL; // Er denne nødvendig?
    if (item != NULL){
        if (strcmp(item, "&&") == 0){
            p_node->token_type->AND = true;
        }
        else if (strcmp(item, "||") == 0){
            p_node->token_type->OR = true;
        }
        else if (strcmp(item, "&!") == 0){
            p_node->token_type->ANDNOT = true;
        }
        else{ 
            p_node->token_type->WORD = true;
        }
        p_node->item = item;   
    return p_node;
    }
    else{
        pr_error("Not a word, aborting indexing");
        return NULL;
    }
}

/* Fetching docs on the word */
set_t *fetch_docs(index_t *index, p_node_t *node, char* errmsg){
    entry_t *entry_node = map_get(index->hashmap,node->item);
    if (entry_node == NULL){
        errmsg = "Word does not exist - Returning NULL \n";
        return NULL;
    }
    /* Creating new set, so old set in index wont be altered */
    set_t *set_word = entry_node->val;  

    // Funker ikke så bra, klarer ikke å compare
    set_iter_t *set_iter = set_createiter(set_word);
    set_t *new_set = set_create((cmp_fn) cmp_doc_query);
    while(set_hasnext(set_iter) != 0){
        query_result_t *doc = set_next(set_iter);
        char *new_name;
        new_name = malloc(sizeof(char)*strlen(doc->doc_name)+1);
        strcpy(new_name, doc->doc_name);
        // int new_score;
        // new_score = (int)doc->score;
        query_result_t *new_doc = create_query(new_name, 0);
        set_insert(new_set, new_doc);
    }
    UNUSED(errmsg);
    return new_set;
}



/* Return intersection "&&" on set from left and right node */
set_t *ptree_intersection(index_t *index, p_node_t *left, p_node_t *right, char* errmsg){
    set_t *set_left = ptree_operation(index, left, errmsg);
    set_t *set_right = ptree_operation(index, right, errmsg);
    if (set_left == NULL || set_right == NULL){
        errmsg = "Getting set of one word went wrong - Returning NULL \n"; 
        return NULL;
    } 
    set_t *result = set_intersection(set_left, set_right);
    UNUSED(errmsg);
    return result;
}

/* Return union "||" on set from left and right node 
* Getting score from each doc in both sets to add as score to next set.
* Score starts as frequency of each word to document, and set operations adds unto the score. 
*/
set_t *ptree_union(index_t *index, p_node_t *left, p_node_t *right, char* errmsg){
    set_t *set_left = ptree_operation(index, left, errmsg);
    set_t *set_right = ptree_operation(index, right, errmsg);
    if (set_left == NULL || set_right == NULL){
        errmsg = "Getting set of one word went wrong - Returning NULL \n"; 
        return NULL;
    } 
    set_t *result = set_union(set_left, set_right);



    UNUSED(errmsg);
    return result;
}
/* Return difference "&!" on set from left and right node */
set_t *ptree_difference(index_t *index, p_node_t *left, p_node_t *right, char* errmsg){
    set_t *set_left = ptree_operation(index, left, errmsg);
    set_t *set_right = ptree_operation(index, right, errmsg);
    if (set_left == NULL || set_right == NULL){
        errmsg = "Getting set of one word went wrong - Returning NULL \n"; 
        return NULL;
    } 
    set_t *result = set_difference(set_left, set_right);
    UNUSED(errmsg);
    return result;
}
  
/* 
* Recursive set operation on nodes. Checks if node is a word or an operator (&&, &! or ||)
* Recursively goes through left and right nodes until leaves are met. 
* Return set operation on left and right nodes
*/
set_t *ptree_operation(index_t *index, p_node_t *node, char *errmsg){
    p_type_t *type = node->token_type;

    if (type->WORD == 1){ 
        set_t *result = fetch_docs(index, node, errmsg);
        return result;
    }
    if (type->AND == 1)
    {
        set_t *result = ptree_intersection(index, node->left, node->right, errmsg);
        return result;
    }
    else if (type->ANDNOT == 1)
    {
        set_t *result = ptree_difference(index, node->left, node->right, errmsg);
        return result;
    }
    else if (type->OR == 1)
    {
        set_t *result = ptree_union(index, node->left, node->right, errmsg);
        return result;
    }   
    errmsg = "Failure on fetching sets\n";
    return NULL;
}


p_node_t *parse_query(list_iter_t *query_iter){
    char *curr_token = list_next(query_iter); // gets the next wor
    if (curr_token == NULL){ // if end of string
        pr_error("String does not exist - Aborts \n");
        return NULL;
    }
    if (query_iter->list->length == 1){
        return pnode_create(curr_token); // if sample has only one word without "()"
    }
    if (strcmp(curr_token, "(") == 0) // start of a query
    { 
        p_node_t *return_node = parse_query(query_iter); // parsere ny query
        char *next_token = list_next(query_iter); // sjekker om det er noe mer
        if (next_token == NULL){
            return return_node;
        }
        else if (strcmp(next_token, ")") != 0){
            p_node_t *root_node = pnode_create(next_token);
            root_node->left = return_node;
            root_node->right = parse_query(query_iter);
            return root_node;

        }
        else if (strcmp(next_token, ")") == 0)
        {
            return return_node;
        }
    }
    else{ // This happens after a "(", meaning a query happens.
        p_node_t *leaf_node = pnode_create(curr_token); // Is always a leaf_node (unless of a single term search, is then the only node)
        char *next_token = list_next(query_iter); // Checking next token, either a operator ("&&", "&!", "||") or a ")".
        if (strcmp(next_token, ")") == 0 || next_token == NULL){ // If ")", end of query, and is returned as a right node
            return leaf_node;
        }
        p_node_t *root_node = pnode_create(next_token);
        root_node->left = leaf_node;
        root_node->right = parse_query(query_iter);
        return root_node;
        }
    pr_error("Something went wrong in making Parser Tree\n");
    return NULL;
    }



    
void ptree_parsing(p_tree_t *p_tree, list_iter_t *query_iter){
    p_tree->root = parse_query(query_iter);
    list_destroyiter(query_iter);
    }

// ---------------- Parser Tree AST functions STOPS HERE---------------- 


void word_list_parser(list_t *word_list, p_node_t *root){
    if (root->token_type->WORD == 1){
        list_addfirst(word_list, root->item);
    }
    else{
        word_list_parser(word_list, root->left);
        word_list_parser(word_list, root->right);
    }
}

list_t *index_query(index_t *index, list_t *query_tokens, char *errmsg) {
    print_list_of_strings("query", query_tokens); // remove this if you like - no thank you :)

    /**
     * TODO: perform the search, and return:
     * query is invalid => write reasoning to errmsg and return NULL
     * query is valid   => return list with any results (empty if none)
     *
     * Tip: `snprintf(errmsg, LINE_MAX, "...")` is a handy way to write to the error message buffer as you
     * would do with a typical `printf`. `snprintf` does not print anything, rather writing your message to
     * the buffer.
     */

     // TODO: return list of query_result_t objects instead

    
    p_tree_t *p_tree = ptree_create(); // Creating Parsing AST tree
    
    list_iter_t *new_iter = list_createiter(query_tokens); // Iterator for query_tokens
 
    ptree_parsing(p_tree, new_iter); // Parses query tokens to AST tree
 
    list_t *word_list = list_create((cmp_fn) strcmp); // for easy score handling aferwards
    word_list_parser(word_list, p_tree->root); // Adding words from token query to linked list

    set_t *result = ptree_operation(index, p_tree->root, errmsg); // Creating result set from Parser AST tree
    if (result == NULL || set_length(result) == 0){
        errmsg = "No documents could be fetched by input\n";
        return NULL;
    }

     // Kan lage egen funskjon for denne
    list_iter_t *word_iter = list_createiter(word_list);
    while (list_hasnext(word_iter) != 0)
    {
        char * word = list_next(word_iter);
        entry_t *entry_word = map_get(index->hashmap, word);
        set_t *set_word = entry_word->val;
        set_iter_t *result_iter = set_createiter(result);
        while (set_hasnext(result_iter) != 0){
            query_result_t *result_doc = set_next(result_iter);
            query_result_t *found_doc = set_get(set_word, result_doc);
            if (found_doc != NULL){
                result_doc->score += found_doc->score;
            }
        }
        set_destroyiter(result_iter);
    }
    list_destroyiter(word_iter);
    

   
    printf("Number of documents for term is: %lu\n", set_length(result));
    set_iter_t *set_iter = set_createiter(result);
    list_t *result_list = list_create((cmp_fn) compare_results_by_score);

    while (set_hasnext(set_iter) != 0)
    {
        query_result_t *query_result = set_next(set_iter); // <-- funker bra

        printf("Name: %s has score of: %d\n", query_result->doc_name, (int)query_result->score);
        list_addfirst(result_list, query_result);
    }
    list_sort(result_list);



    UNUSED(errmsg);
    // p_tree *AST = p_tree_create((cmp_fn) strcmp); // Creating abstract syntaxt tree
    return result_list;
    // return query_tokens;
    
}

void index_stat(index_t *index, size_t *n_docs, size_t *n_terms) {
    /**
     * TODO: fix this
     */
    *n_docs = index->num_docs; // See index->num_docs 
    *n_terms = index->hashmap->length; // return map->length
}
