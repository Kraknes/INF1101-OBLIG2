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


// Own doc_info struct //



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
    UNUSED(index);
    
    /**
     * TODO: Free all memory associated with the index
     */
    free(index->hashmap);
    free(index);
}

// // Made new function to work with new struct
// lnode_t *list_contains_doc(list_t *list, doc_i *doc){
//     lnode_t *node = list->leftmost;

//     while (node != NULL) {
//         doc_i *curr_doc = node->item; // First doc in the list
//         if (list->cmpfn(doc->docID, curr_doc->docID) == 0) { // Compares the doc in list to arg doc.
//             return node;
//         }
//         node = node->right;
//     }

//     return NULL;
// }

query_result_t *create_query(char *doc_name, int freq){
    query_result_t *query = calloc(1, sizeof(query_result_t));  
    if (!query){
        pr_error("Calloc of doc_i error, terminate indexing");
        PANIC("AAAAAAAAH");
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

//  HVIS Æ HAR SKJØNT RIKTIG, SÅ FÅR FUNSKJONEN ANVNET PÅ DOUMENTET OG EN LENKET LISTE OVER ALLE ORD I DOKUMENTET. 
// FINN UT HVORDAN INDEX SKAL BRUKE DEN INFORMASJONEN VIDERE
// PLAN: 
// 1. LAGER ITERATOR AV LISTEN, VIL GÅ GJENNOM LISTEN AV ALLE ORD OG LEGGE TIL I HASHMAP
// 2. LAGER EGEN STRUCT (WORD_S) FOR WORD INFO = DOC_NR OG FREQ I DOKUMENTET. BLIR LAGT TIL SOM "VALUE" I MNODE I HASHMAP
// 3. 

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
                set_t *term_set = set_create((cmp_fn) strcmp); // making a  set for popped term
                set_insert(term_set, query_term); // Adding document
                map_insert(index->hashmap, curr_term, term_set); // adding linked list as value to the popped "term" as key to hashmap
            }
            else{ // term_entry != NULL: The word/term exist, therefore, a set for doc_names is there as well.
                set_t *term_set = term_entry->val; // Accessing the document set of current term  
                if (set_get_doc(term_set, query_term->doc_name) != NULL){ // If document name is in the set, adds "score" to it for frequency
                    query_result_t *query = set_get_doc(term_set, query_term->doc_name);
                    query->score++;
                }
                else{
                    set_insert(term_set, query_term); // if the document exist in term, then do nothing
                }
            }
        }
    return 0; // or -x on error
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

     // return NULL; // TODO: return list of query_result_t objects instead

     // TROR FUNKSJONEN FÅR INN EN SORTERT LENKET LISTE OVER INPUT. DENNE SKAL DA PROSESSERE  
    // TOKENS ER HVERT ORD DELT OPP, VI SLIPPER Å GJØRE DET SELV. BLIR GJORT AV PRECODE (HELDIGIVIS)
    // SKAL RETURNERE EN LISTE AV QUERY_STRUCTS MED SCORE I DESCENDING ORDER, BRUK FREQ I DOKUMENT FOR DET

    list_iter_t *q_iter = list_createiter(query_tokens); // Create iter for query_token LList

    list_t *test_list = list_create((cmp_fn) strcmp);


    while (list_hasnext(q_iter) != 0){ // Will go on until all tokens are processed
         // Going to next node;
        char *curr_token = list_next(q_iter); // current token from LList
        list_addlast(test_list, curr_token);
        printf("Adding token to list\n");
    }
    // Frigjøre query_tokens?
    printf("Done with token querys\n");
    entry_t *a = map_get(index->hashmap,test_list->leftmost->item);
    entry_t *b = map_get(index->hashmap,test_list->rightmost->item);
    set_t *as = a->val;
    printf("done as. set length: %ld\n", set_length(as));
    set_t *bs = b->val;
    printf("done bs. set length: %ld\n", set_length(bs));


    list_iter_t *test_iter = list_createiter(test_list);
    printf("hey1\n");
    while (list_hasnext(test_iter) != 0)
    {
        printf("hey2\n");
        char *d = list_next(test_iter);
        printf("%s\n", (char*)d);
        if (strcmp(d,"&&") == 0){
            set_t *c = set_intersection(as,bs);
            printf("c length is: %lu\n", set_length(c));

            set_iter_t *set_iter = set_createiter(c);
            query_result_t *query_result = set_next(set_iter); // <-- funker bra
            printf("%s\n", query_result->doc_name);
            query_result = set_next(set_iter); // <-- funker bra
            printf("%s\n", query_result->doc_name);
        }
        else if (strcmp(d, "||")==0) {
            set_t *c = set_union(as,bs);
            printf("c length is: %lu\n", set_length(c));

            set_iter_t *set_iter = set_createiter(c);
            query_result_t *query_result = set_next(set_iter); // <-- funker bra
            printf("%s\n", query_result->doc_name);
            query_result = set_next(set_iter); // <-- funker bra
            printf("%s\n", query_result->doc_name);
        }
        

            
            
        
            // printf("%s\n", (char*)set_next(set_iter));
            // printf("%s\n", (char*)set_next(set_iter));
            // printf("%s\n", (char*)set_next(set_iter));
            // printf("%s\n", (char*)set_next(set_iter));
        
    }
    

    
    // p_tree *AST = p_tree_create((cmp_fn) strcmp); // Creating abstract syntaxt tree
    return query_tokens;
    UNUSED(index);
    UNUSED(query_tokens);
    UNUSED(errmsg);
    // return query_tokens;
    
}

void index_stat(index_t *index, size_t *n_docs, size_t *n_terms) {
    /**
     * TODO: fix this
     */
    *n_docs = index->num_docs; // See index->num_docs 
    *n_terms = index->hashmap->length; // return map->length
}
