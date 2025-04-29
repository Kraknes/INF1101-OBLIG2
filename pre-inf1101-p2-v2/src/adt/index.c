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
    index->hashmap = map_create((cmp_fn) strcmp, hash_string_fnv1a64);
    return index;
}

void index_destroy(index_t *index) {
    map_iter_t *map_iter = map_createiter(index->hashmap);
    if (map_hasnext(map_iter) != 0){

        entry_t *entry = map_next(map_iter);
        if (entry != NULL){
            set_destroy(entry->val, (free_fn) free);
        }
    }
    map_destroyiter(map_iter);
    free(index->hashmap);
    free(index);
}

// 

/* Own compare function for query_structs */
int cmp_doc_query(void *a, void *b){
    query_result_t *aq = a;
    query_result_t *bq = b;
    return strcmp(aq->doc_name, bq->doc_name);
}

/* Creating query_result_t struct */
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
    if (terms == NULL || doc_name == NULL){
        pr_error("terms or doc_name is null, terminate indexing");
        return -1;
    }
    index->num_docs++; // Counts every document

    list_iter_t *list_iter = list_createiter(terms); // A checker to see if there is a next node in the list
    while(list_hasnext(list_iter) != 0){ // If there is a node with a term
        
        char *curr_term = list_next(list_iter); // get the first node term item from the "terms" list
        
        entry_t *term_entry = map_get(index->hashmap, curr_term); // checking if the word is in the hashmap

        query_result_t *query_term = create_query(doc_name, 1); // making query to insert in term entry

        if (term_entry == NULL) // if nothing in the entry, inserting new term to hashmap 
        {
            set_t *term_set = set_create((cmp_fn) cmp_doc_query); // making a  set for popped term
            set_insert(term_set, query_term); // Adding document
            map_insert(index->hashmap, curr_term, term_set); // adding linked list as value to the popped "term" as key to hashmap
        }
        else // term_entry != NULL: The word/term exist, therefore, a set for doc_names is there as well.
        { 
            set_t *term_set = term_entry->val; // Accessing the document set of current term  
            if (set_get(term_set, query_term) != NULL){ // If document name is in the set, adds "score" to it for frequency
                query_result_t *query = set_get(term_set, query_term);
                query->score++;
                free(query_term);
            }
            else{
                set_insert(term_set, query_term); // if the document does not exist in term, add in the document with 1 in score
            }
        }
    }
    free(terms);
    list_destroyiter(list_iter);
    return 0; // or -x on error
}

// ---------------- below are Parser Tree AST functions ---------------- ¨

/*  Creating AST parser tree for list of query terms*/
p_tree_t *ptree_create(){ 
    p_tree_t *p_tree = calloc(1, sizeof(p_tree));
    return p_tree;
}
/* Creating node for AST tree. Checks the term if it is an operator or a word. Sets it as token_type */
p_node_t *pnode_create(char *item){
    p_node_t *p_node = calloc(1, sizeof(p_node_t));
    p_node->token_type = calloc(1, sizeof(p_type_t));
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

/* 
* Fetching the set of document of word from reversed index. Making a deep-copy of set to avoid deleting of original set. 
*/
set_t *fetch_docs(index_t *index, p_node_t *node, char* errmsg){
    entry_t *entry_node = map_get(index->hashmap,node->item);
    if (entry_node == NULL){
        snprintf(errmsg, LINE_MAX, "Word does not exist - Returning NULL \n");
        return NULL;
    }
    set_t *set_word = entry_node->val;  

     /* Creating a deep-copy set, so old set in index wont be altered/deleted*/
    set_t *new_set = set_create((cmp_fn) cmp_doc_query);
    set_iter_t *set_iter = set_createiter(set_word);
    if (set_iter == NULL){
         snprintf(errmsg, LINE_MAX, "Could not make set iterator - Returning NULL \n");
         return NULL;
    }
    while(set_hasnext(set_iter) != 0){
        query_result_t *doc = set_next(set_iter);
        char *new_name;
        new_name = malloc(sizeof(char)*strlen(doc->doc_name)+1);
        strcpy(new_name, doc->doc_name);
        query_result_t *new_doc = create_query(new_name, 0);
        set_insert(new_set, new_doc);
    }
    UNUSED(errmsg);
    set_destroyiter(set_iter);
    return new_set;
}



/* Return intersection "&&" on set from left and right node */
set_t *ptree_intersection(index_t *index, p_node_t *left, p_node_t *right, char* errmsg){
    set_t *set_left = ptree_operation(index, left, errmsg);
    set_t *set_right = ptree_operation(index, right, errmsg);
    if (set_left == NULL || set_right == NULL){
        snprintf(errmsg, LINE_MAX, " Getting set word went wrong - Returning NULL \n");
        return NULL;
    } 
    set_t *result = set_intersection(set_left, set_right);
    UNUSED(errmsg);
    return result;
}

/* Return union "||" on set from left and right node */
set_t *ptree_union(index_t *index, p_node_t *left, p_node_t *right, char* errmsg){
    set_t *set_left = ptree_operation(index, left, errmsg);
    set_t *set_right = ptree_operation(index, right, errmsg);
    if (set_left == NULL || set_right == NULL){
        snprintf(errmsg, LINE_MAX, " Getting set word went wrong - Returning NULL \n");
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
    if (set_left == NULL){
        return set_right;
    } 
    if (set_right == NULL){
        return set_left;
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
        if (result == NULL){
            printf("%s\n", node->item);
            snprintf(errmsg, LINE_MAX, "The above word does not exist, try another word. Aborts");
            return NULL;
        }
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
    snprintf(errmsg, LINE_MAX, "Could not iterate through AST tree - returning NULL \n");
    return NULL;
}

/* 
* One sole Recursive query token parser function to AST tree. 
* Fetches first word of the linked list, and detects if its a start of a query "(". 
* The functions recursively initiate itself for each term.
* The returned node is the root of a subtree.
* Returns the final root for the whole AST tree by recursively uses the same function for every query
* 
 */
p_node_t *parse_query(list_iter_t *query_iter, char* errmsg){
    char *curr_token = list_next(query_iter); // Fetches the next word from the query list

    if (curr_token == NULL){ // If no next word, then the input is not correct. 
        snprintf(errmsg, LINE_MAX, "String does not exist in AST parsing - returning NULL \n");
        return NULL;
    }
    if (query_iter->list->length == 1){ // If sample has only one word without "()", for edge cases. 
        return pnode_create(curr_token); 
    }

    if (strcmp(curr_token, "(") == 0) // Start of a query, e.g --> "(A && B)"
    { 
        p_node_t *parent1_node = parse_query(query_iter, errmsg); // Recursively initiate same function parse tokens in query. Next term can be word or a new parantheese. Returns a root node of a subtree 
        char *next_token = list_next(query_iter); // Getting the next token 
        if (next_token == NULL || strcmp(next_token, ")") == 0){ // If null or a ")", end of parsing. The returned node is the root of a subtree, or AST tree. 
            return parent1_node;
        }
        else if (strcmp(next_token, ")") != 0){  // This checks if there is more after a finished query ")" symbol e.g.  ((A && B) &! C)
            p_node_t *parent2_node = pnode_create(next_token); // Meaning the first query is the left node, and the newly created node the root node for the subtree/tree. 
            parent2_node->left = parent1_node; 
            parent2_node->right = parse_query(query_iter, errmsg); // Parsing the right node for a new word or a new query
            return parent2_node;
        }
    }
    else { // This happens after a "(", meaning a query happens. The next word after a "(" is a always a word, e.g --> "(A && B)"
        p_node_t *leaf_node = pnode_create(curr_token); // Creating word node (always a word). 
        char *next_token = list_next(query_iter); // Getting next token, can either be a operator ("&&", "&!", "||") or a ")".
        if (next_token == NULL || strcmp(next_token, ")") == 0){ // If ")", end of query, and is returned as a right node to a subtree. The subtree is then finished. Next_token == null is for bad inputs e.g: "(a && b" 
            return leaf_node;
        }
        p_node_t *parent_node = pnode_create(next_token); // If not ")", then the next term/word is an operator ("&&", "&!", "||"). 
        parent_node->left = leaf_node; // Operator  becomes the root, and the previous node will be the left node
        parent_node->right = parse_query(query_iter, errmsg); // Parsing the next word, can either be a word (end of query), or a new query ("(")
        return parent_node; // Subtree is then returned
        }
        
    snprintf(errmsg, LINE_MAX, "Something went wrong in making Parser Tree - returning NULL\n");
    return NULL;
    }



/* 
* Parent p_tree parsing function
*/
void ptree_parsing(p_tree_t *p_tree, list_iter_t *query_iter, char* errmsg){
    p_tree->root = parse_query(query_iter, errmsg);
    list_destroyiter(query_iter);
    UNUSED(errmsg);
    }

// ---------------- Parser Tree AST functions STOPS HERE---------------- 

/* 
* Linked list iterator to fetch all the words in the AST.
* Will be used to acquire frequency of word to calculate score
 */
void word_list_parser(list_t *word_list, p_node_t *root, index_t *index){
    if (root->token_type->WORD == 1){
        if (map_get(index->hashmap, root->item) != NULL){ // In case word does not exist in inverted index, voids segmentation fault
            list_addfirst(word_list, root->item);
        }
    }
    else{
        word_list_parser(word_list, root->left, index);
        word_list_parser(word_list, root->right, index);
    }
}

/* 
* Function to calculate score for each term.ANSI_CLEAR_TERM
* Fetches word from word_list and gets frequency of each word in the document of interest. 
* Frequency of all words are summed together as a final score for each document. 
* E.g, all operations on terms do the same calculations.
*/
int score_calculation(index_t *index,  list_t *word_list, set_t *result, char *errmsg){
    list_iter_t *word_iter = list_createiter(word_list);
    if (word_iter == NULL){
        snprintf(errmsg, LINE_MAX, "Couldn't create list iter \n");
        return 0;
    }
    while (list_hasnext(word_iter) != 0)
    {
        char * word = list_next(word_iter);
        entry_t *entry_word = map_get(index->hashmap, word);
        set_t *set_word = entry_word->val;
        set_iter_t *result_iter = set_createiter(result);
        if (word_iter == NULL){
            snprintf(errmsg, LINE_MAX, "Couldn't create set iter \n");
            return 0;
        }
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
    return 1;
}

list_t *index_query(index_t *index, list_t *query_tokens, char *errmsg) {
    print_list_of_strings("query", query_tokens); // remove this if you like

    p_tree_t *p_tree = ptree_create(); // Creating Parsing AST tree
    
    list_iter_t *new_iter = list_createiter(query_tokens); // Iterator for query_tokens
 
    ptree_parsing(p_tree, new_iter, errmsg); // Parses query tokens to AST tree
 
    list_t *word_list = list_create((cmp_fn) strcmp); // for easy score handling afterwards
    word_list_parser(word_list, p_tree->root, index); // Adding words from token query to linked list, will be used for calculating score 

    set_t *result = ptree_operation(index, p_tree->root, errmsg); // Creating result set from Parser AST tree
    if (result == NULL || set_length(result) == 0){
        return NULL;
    }
    
    if (score_calculation(index, word_list, result, errmsg) != 1){ // Adds score to each document in result set from Parser AST tree
        snprintf(errmsg, LINE_MAX, "Problems in score calculation \n");
        return NULL;
    }
    
    set_iter_t *set_iter = set_createiter(result);
    if (result == NULL || set_length(result) == 0){
        snprintf(errmsg, LINE_MAX, "Problems in creating set iter\n");
        return NULL;
    }
    list_t *result_list = list_create((cmp_fn) compare_results_by_score); // Creating list of documents to be returned
    while (set_hasnext(set_iter) != 0) // Goes through all documents in set, and put in the result_list to be returned
    {
        query_result_t *query_result = set_next(set_iter); 
        list_addfirst(result_list, query_result);
    }
    set_destroyiter(set_iter);

    list_sort(result_list); // Sorts list after score by using compare_results_by_score func. 

    UNUSED(errmsg);
    return result_list;
    
}

void index_stat(index_t *index, size_t *n_docs, size_t *n_terms) {
    *n_docs = index->num_docs; // See index->num_docs 
    *n_terms = index->hashmap->length; // return map->length
}
