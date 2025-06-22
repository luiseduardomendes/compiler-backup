#ifndef ILOC_H
#define ILOC_H
#include <stdio.h>

typedef struct iloc_instr {
    char *label;      // Optional: label for this instruction (or NULL)
    char *opcode;     // e.g., "add", "loadI"
    char *arg1;
    char *arg2;
    char *result;
    struct iloc_instr *next;
} iloc_instr_t;

typedef struct iloc_list {
    iloc_instr_t *head;
    iloc_instr_t *tail;
} iloc_list_t;

// Functions to create and append instructions
iloc_instr_t* make_iloc(const char *label, const char *opcode, const char *arg1, const char *arg2, const char *result);
iloc_list_t* new_iloc_list();
void append_iloc(iloc_list_t *list, iloc_instr_t *instr);
iloc_list_t* concat_iloc(iloc_list_t *a, iloc_list_t *b);
void free_iloc_list(iloc_list_t *list); // Add this 
iloc_list_t* copy_iloc_list(const iloc_list_t* src);

void print_iloc_code(FILE *stream, iloc_list_t *list);

#endif