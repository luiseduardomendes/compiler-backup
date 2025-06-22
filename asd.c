#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "asd.h"
#include "code.h"
#include "iloc.h"

asd_tree_t *asd_new(const char *label, type_t type, iloc_list_t *code, char *place)
{
    asd_tree_t *ret = calloc(1, sizeof(asd_tree_t));
    if (ret != NULL) {
        ret->label = strdup(label);
        ret->number_of_children = 0;
        ret->type = type;
        ret->code = code;
        ret->place = place ? strdup(place) : NULL;
        ret->children = NULL;
    }
    return ret;
}

void asd_free(asd_tree_t *tree)
{
    if (tree != NULL) {
        int i;
        for (i = 0; i < tree->number_of_children; i++) {
            asd_free(tree->children[i]);
        }
        tree->number_of_children = 0;
        free(tree->children);
        free(tree->label);
        if (tree->place) free(tree->place);
        if (tree->code) free_iloc_list(tree->code);
        free(tree);
        tree = NULL;
    } else {
        printf("Erro: %s recebeu parâmetro tree = %p.\n", __FUNCTION__, tree);
    }
}

void asd_add_child(asd_tree_t *tree, asd_tree_t *child)
{
    if (tree != NULL && child != NULL) {
        tree->number_of_children++;
        tree->children = realloc(tree->children, tree->number_of_children * sizeof(asd_tree_t*));
        tree->children[tree->number_of_children-1] = child;
    } else {
        printf("Erro: %s recebeu parâmetro tree = %p / %p.\n", __FUNCTION__, tree, child);
    }
}

static void _asd_print (FILE *foutput, asd_tree_t *tree, int profundidade)
{
    int i;
    if (tree != NULL) {
        fprintf(foutput, "%d%*s: Nó '%s' tem %d filhos:\n", profundidade, profundidade*2, "", tree->label, tree->number_of_children);
        for (i = 0; i < tree->number_of_children; i++) {
            _asd_print(foutput, tree->children[i], profundidade+1);
        }
    } else {
        printf("Erro: %s recebeu parâmetro tree = %p.\n", __FUNCTION__, tree);
    }
}

void asd_print(asd_tree_t *tree)
{
    FILE *foutput = stderr;
    if (tree != NULL) {
        _asd_print(foutput, tree, 0);
    } else {
        printf("Erro: %s recebeu parâmetro tree = %p.\n", __FUNCTION__, tree);
    }
}

static void _asd_print_graphviz (FILE *foutput, asd_tree_t *tree)
{
    int i;
    if (tree != NULL) {
        fprintf(foutput, "  %ld [ label=\"%s\" ];\n", (long)tree, tree->label);
        for (i = 0; i < tree->number_of_children; i++) {
            fprintf(foutput, "  %ld -> %ld;\n", (long)tree, (long)tree->children[i]);
            _asd_print_graphviz(foutput, tree->children[i]);
        }
    } else {
        printf("Erro: %s recebeu parâmetro tree = %p.\n", __FUNCTION__, tree);
    }
}

void asd_print_graphviz(asd_tree_t *tree)
{
    FILE *foutput = stdout;
    if (tree != NULL) {
        fprintf(foutput, "digraph grafo {\n");
        _asd_print_graphviz(foutput, tree);
        fprintf(foutput, "}\n");
    } else {
        printf("Erro: %s recebeu parâmetro tree = %p.\n", __FUNCTION__, tree);
    }
}

void print_code(iloc_list_t *code) {
    if (code == NULL || code->head == NULL) {
        printf("No ILOC code to print.\n");
        return;
    }
    iloc_instr_t *current = code->head;
    while (current != NULL) {
        printf("%s: %s %s, %s -> %s\n", current->label ? current->label : " ", current->opcode, 
               current->arg1 ? current->arg1 : " ", 
               current->arg2 ? current->arg2 : " ", 
               current->result ? current->result : " ");
        current = current->next;
    }
}