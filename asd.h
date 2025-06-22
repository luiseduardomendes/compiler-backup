#ifndef ASD_H
#define ASD_H

#include "type.h"
#include "iloc.h"

typedef struct asd_tree {
    char *label;
    type_t type;
    int number_of_children;
    struct asd_tree **children;

    // For code generation
    iloc_list_t *code; // Pointer to ILOC code for this node
    char *place;       // Register/temp holding the result
} asd_tree_t;

// Updated constructor to match new fields
asd_tree_t *asd_new(const char *label, type_t type, iloc_list_t *code, char *place);
void asd_free(asd_tree_t *tree);
void asd_add_child(asd_tree_t *tree, asd_tree_t *child);
void asd_print(asd_tree_t *tree);
void asd_print_graphviz(asd_tree_t *tree);
void print_code(iloc_list_t *code);

#endif // ASD_H
