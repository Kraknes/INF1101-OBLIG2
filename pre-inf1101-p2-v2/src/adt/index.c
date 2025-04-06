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

// Made new function to work with new struct
lnode_t *list_contains_doc(list_t *list, doc_i *doc){
    lnode_t *node = list->leftmost;

    while (node != NULL) {
        doc_i *curr_doc = node->item; // First doc in the list
        if (list->cmpfn(doc->docID, curr_doc->docID) == 0) { // Compares the doc in list to arg doc.
            return node;
        }
        node = node->right;
    }

    return NULL;
}

doc_i *create_doc(char *doc_name, int freq){
    doc_i *doc = calloc(1, sizeof(doc));  
    if (!doc){
        pr_error("Calloc of doc_i error, terminate indexing");
        PANIC("AAAAAAAAH");
    }
    doc->docID = doc_name; // Adding the doc name 
    doc->freq = freq;

    return doc;
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
    while (terms) // FUNKER IKKE HELT
    {   
        list_iter_t *list_iter = list_createiter(terms); // A checker to see if there is a next node in the list
        if (list_hasnext(list_iter) != 0){ // If there is a node with a term
        void *curr_term = list_popfirst(terms); // get the first node term item from the "terms" list

        entry_t *term_entry = map_get(index->hashmap, curr_term); // checking if the word is in the hashmap

        doc_i *doc = create_doc(doc_name, 1); // creating doc struct for insert in term linked list

        // if nothing in the entry, inserting 
        if (term_entry == NULL){
            list_t *doc_list = list_create((cmp_fn) strcmp); // making a linkedlist for unique docIDs to the specific popped "term"
            list_addfirst(doc_list, doc); // Adding document info to doc_list
            map_insert(index->hashmap, curr_term, doc_list); // adding linked list as value to the popped "term" as key to hashmap
        }
        else{ // term_entry != NULL: The word/term exist, therefore, a linked list for doc_ids is there as well.
    
            list_t *doc_list = term_entry->val; // Accessing the document list of current term  
            lnode_t *doc_node = list_contains_doc(doc_list, doc); // Traversing/iterating through the document list. If it exist, change the freq, else, add in.
           
            if (doc_node == NULL) { // Checking if the doc is in the list -> = null means not in list, = node means yes
                list_addfirst(doc_list, doc); // adding the new document into the list
            }
            else{ // If the document does exist, we have to change the freq of that word. 
                doc_i *term_doc = doc_node->item; // the returned node has the document of interest
                term_doc->freq++; // changes the freq of the document
            }
            
        }
    }
    else{
        break; // viktig! ellers går dne i en evig loop
    }
    
    }
    return 0; // or -x on error
}




query_result_t *create_query_result(){
    query_result_t *query_result = calloc(1, sizeof(query_result_t));
    return query_result;
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

     // TROR FUNKSJONEN FÅR INN EN SORTERT LENKET LISTE OVER INPUT. DENNE SKAL DA PROSESSERE  
    // TOKENS ER HVERT ORD DELT OPP, VI SLIPPER Å GJØRE DET SELV. BLIR GJORT AV PRECODE (HELDIGIVIS)
    // SKAL RETURNERE EN LISTE AV QUERY_STRUCTS MED SCORE I DESCENDING ORDER, BRUK FREQ I DOKUMENT FOR DET

    // p_tree *AST = p_tree_create((cmp_fn) strcmp); // Creating abstract syntaxt tree

    list_iter_t *q_iter = list_createiter(query_tokens); // Create iter for query_token LList

    while (list_hasnext(q_iter) != 0){ // Will go on until all tokens are processed
        char *curr_token = list_next(q_iter); // current token from LList

        entry_t *token_entry = map_get(index->hashmap, curr_token); // Accesing token entry
        list_sort_doc(token_entry->val);
        list_iter_t *t_entry_iter = list_createiter(token_entry->val); //creating iter from token entry LL

        doc_i *top_doc = calloc(1, sizeof(doc_i));
        top_doc->freq = 0;

        while (list_hasnext(t_entry_iter) != 0){
            doc_i *curr_doc = list_next(t_entry_iter);
            printf("%s\n", curr_doc->docID); // Dette funker bra
            printf("%d\n", curr_doc->freq);

            if (curr_doc->freq > top_doc->freq){ // Finding the best freq document
                top_doc->docID = curr_doc->docID;
                top_doc->freq = curr_doc->freq;
            }
            
        }
        printf("topdoc name: %s\n", top_doc->docID); 
        printf("topdoc freq: %d\n", top_doc->freq);

    }

    UNUSED(errmsg);
    return query_tokens;
    // return NULL; // TODO: return list of query_result_t objects instead
}

void index_stat(index_t *index, size_t *n_docs, size_t *n_terms) {
    /**
     * TODO: fix this
     */
    // UNUSED(index);
    *n_docs = index->num_docs; // See index->num_docs 
    *n_terms = index->hashmap->length; // return map->length
}
