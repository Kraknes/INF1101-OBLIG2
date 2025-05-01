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

int index_document(index_t *index, char *doc_name, list_t *terms) {
    if (terms == NULL || doc_name == NULL){
        pr_error("terms or doc_name is null, terminate indexing");
        return -1;
    }
    index->num_docs++; // Counts every document

    list_iter_t *list_iter = list_createiter(terms); 
    // While loop for putting in word as key to hashmap, and set document name as items in a red-black tree ADT
    while(list_hasnext(list_iter) != 0){ 
        
        char *curr_term = list_next(list_iter); 
        
        entry_t *term_entry = map_get(index->hashmap, curr_term);

        query_result_t *query_term = create_query(doc_name, 1); 

        // if nothing in the entry, inserting new term to hashmap 
        if (term_entry == NULL) 
        {
            set_t *term_set = set_create((cmp_fn) cmp_doc_query); 
            set_insert(term_set, query_term);
            map_insert(index->hashmap, curr_term, term_set); 
        }
        // term_entry != NULL: The word/term exist, therefore, a set for doc_names is there as well.
        else 
        { 
            set_t *term_set = term_entry->val; 
            if (set_get(term_set, query_term) != NULL){ 
                query_result_t *query = set_get(term_set, query_term);
                query->score++;
                free(query_term);
            }
             // if the document does not exist in term, add in the document with 1 in score
            else{
                set_insert(term_set, query_term);
            }
        }
    }
    free(terms);
    list_destroyiter(list_iter);
    return 0; // or -x on error
}

/* 
* Linked list iterator to fetch all the words in the AST.
* Will be used to acquire frequency of word to calculate score
 */
void word_list_parser(list_t *word_list, p_node_t *root, index_t *index) {
    if (root == NULL) {
        return;
    }
    if (root->token_type->WORD == 1) {
        if (map_get(index->hashmap, root->item) != NULL) { // Avoid segmentation fault if word does not exist in the inverted index
            list_addfirst(word_list, root->item);
        }
    } else {
        word_list_parser(word_list, root->left, index);
        word_list_parser(word_list, root->right, index);
    }
}

/* 
* Function to calculate score for each term.
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
    
    list_iter_t *query_iter = list_createiter(query_tokens); // Iterator for query_tokens
 
    ptree_parsing(p_tree, query_iter, errmsg); // Parses query tokens to AST tree
 
    list_t *word_query_list = list_create((cmp_fn) strcmp); // for easy score handling afterwards
    word_list_parser(word_query_list, p_tree->root, index); // Adding words from token query to linked list, will be used for calculating score 

    set_t *result = ptree_operation(index, p_tree->root, errmsg); // Creating result set from Parser AST tree
    if (result == NULL){
        return NULL;
    }
    
    if (score_calculation(index, word_query_list, result, errmsg) != 1){ // Adds score to each document in result set from Parser AST tree
        snprintf(errmsg, LINE_MAX, "Problems in score calculation \n");
        return NULL;
    }
    
    set_iter_t *result_iter = set_createiter(result);
    if (result_iter == NULL){
        snprintf(errmsg, LINE_MAX, "Problems in creating set iter\n");
        return NULL;
    }

    list_t *result_list = list_create((cmp_fn) compare_results_by_score); // Creating list of documents to be returned
    while (set_hasnext(result_iter) != 0) // Goes through all documents in set, and put in the result_list to be returned
    {
        query_result_t *query_result = set_next(result_iter); 
        list_addfirst(result_list, query_result);
    }
    set_destroyiter(result_iter);

    list_sort(result_list); // Sorts list after score by using compare_results_by_score func. 

    UNUSED(errmsg);
    return result_list;
    
}

void index_stat(index_t *index, size_t *n_docs, size_t *n_terms) {
    *n_docs = index->num_docs; // See index->num_docs 
    *n_terms = index->hashmap->length; // return map->length
}
