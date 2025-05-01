/**
 * @implements parser.h
 */

 #include <stdlib.h>
 #include <stdio.h>
 #include <string.h>
 #include <limits.h> // for LINE_MAX

#include "parser.h"
#include "index.h"
#include "set.h"
#include "printing.h"
#include "defs.h"
#include "common.h"
#include "map.h"


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


p_tree_t *ptree_create(){ 
    p_tree_t *p_tree = calloc(1, sizeof(p_tree_t));
    return p_tree;
}


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


set_t *fetch_docs(index_t *index, p_node_t *node, char* errmsg){
    entry_t *entry_node = map_get(index->hashmap, node->item);
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


p_node_t *parse_query(list_iter_t *query_iter, char* errmsg){
    char *curr_token = list_next(query_iter); 

    // Edge case
    if (curr_token == NULL){ 
        snprintf(errmsg, LINE_MAX, "String does not exist in AST parsing - returning NULL \n");
        return NULL;
    }
    // Edge case
    if (query_iter->list->length == 1){ 
        return pnode_create(curr_token); 
    }

    // First if statement controls grouping in query, e.g. "(A && B)"
    if (strcmp(curr_token, "(") == 0) 
    { 
        // Recursively initiate itself for handling inside grouping 
        p_node_t *term_node = parse_query(query_iter, errmsg); 
        char *next_token = list_next(query_iter); 
        if (next_token == NULL || strcmp(next_token, ")") == 0){ // If null or a ")", end of parsing.
            return term_node;
        }
        else if (strcmp(next_token, ")") != 0){  // This checks if there is more after one grouping, e.g. ( "((A && B) && C)" 
            p_node_t *root_node = pnode_create(next_token);
            root_node->left = term_node; 
            root_node->right = parse_query(query_iter, errmsg); // Parsing the right node for a new word or a new query
            return root_node;
        }
    }
    // Else statement controls inside grouping in query
    else if (strcmp(curr_token, "(") != 1) { 
        p_node_t *leaf_node = pnode_create(curr_token); // Creating word node (always a word). 
        char *next_token = list_next(query_iter); 
        if (next_token == NULL || strcmp(next_token, ")") == 0){ // End of grouping
            return leaf_node;
        }
        p_node_t *parent_node = pnode_create(next_token); // Always an operator ("&&", "&!", "||"). 
        parent_node->left = leaf_node; 
        parent_node->right = parse_query(query_iter, errmsg); 
        return parent_node; 
        }
        
    snprintf(errmsg, LINE_MAX, "Something went wrong in making Parser Tree - returning NULL\n");
    return NULL;
    }




void ptree_parsing(p_tree_t *p_tree, list_iter_t *query_iter, char* errmsg){
    p_tree->root = parse_query(query_iter, errmsg);
    list_destroyiter(query_iter);
    UNUSED(errmsg);
    }