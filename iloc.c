#include "iloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

iloc_instr_t* make_iloc(const char *label, const char *opcode, const char *arg1, const char *arg2, const char *result) {
    iloc_instr_t *instr = (iloc_instr_t*)malloc(sizeof(iloc_instr_t));
    instr->label  = label   ? strdup(label)  : NULL;
    instr->opcode = opcode  ? strdup(opcode) : NULL;
    instr->arg1   = arg1    ? strdup(arg1)   : NULL;
    instr->arg2   = arg2    ? strdup(arg2)   : NULL;
    instr->result = result  ? strdup(result) : NULL;
    instr->next   = NULL;
    return instr;
}

iloc_list_t* new_iloc_list() {
    iloc_list_t *list = (iloc_list_t*)malloc(sizeof(iloc_list_t));
    list->head = NULL;
    return list;
}

void append_iloc(iloc_list_t *list, iloc_instr_t *instr) {
    if (!list->head) {
        list->head = instr;
    } else {
        iloc_instr_t *curr = list->head;
        while (curr->next) {
            curr = curr->next;
        }
        curr->next = instr;
    }
}

// Helper: Deep copy a single instruction
static iloc_instr_t* copy_iloc_instr(const iloc_instr_t* instr) {
    if (!instr) return NULL;
    iloc_instr_t* copy = malloc(sizeof(iloc_instr_t));
    copy->label  = instr->label  ? strdup(instr->label)  : NULL;
    copy->opcode = instr->opcode ? strdup(instr->opcode) : NULL;
    copy->arg1   = instr->arg1   ? strdup(instr->arg1)   : NULL;
    copy->arg2   = instr->arg2   ? strdup(instr->arg2)   : NULL;
    copy->result = instr->result ? strdup(instr->result) : NULL;
    copy->next   = copy_iloc_instr(instr->next); // recursively copy the chain
    return copy;
}

// Helper: Deep copy an entire iloc_list_t
iloc_list_t* copy_iloc_list(const iloc_list_t* src) {
    iloc_list_t* copy = new_iloc_list();
    if (!src || !src->head) return copy;
    copy->head = copy_iloc_instr(src->head);
    return copy;
}

iloc_list_t* concat_iloc(iloc_list_t *a, iloc_list_t *b) {
    iloc_list_t* result = new_iloc_list();
    iloc_list_t* a_copy = copy_iloc_list(a);
    iloc_list_t* b_copy = copy_iloc_list(b);

    // Append all instructions from a_copy
    if (a_copy->head) {
        result->head = a_copy->head;
    }
    // Append all instructions from b_copy
    if (b_copy->head) {
        if (result->head) {
            iloc_instr_t *curr = result->head;
            while (curr->next) {
                curr = curr->next;
            }
            curr->next = b_copy->head;
        } else {
            result->head = b_copy->head;
        }
    }
    // Free the temporary lists (not their instructions)
    free(a_copy);
    free(b_copy);
    return result;
}

void free_iloc_list(iloc_list_t *list) {
    if (!list) return;
    iloc_instr_t *curr = list->head;
    while (curr) {
        iloc_instr_t *next = curr->next;
        if (curr->label)  free(curr->label);
        if (curr->opcode) free(curr->opcode);
        if (curr->arg1)   free(curr->arg1);
        if (curr->arg2)   free(curr->arg2);
        if (curr->result) free(curr->result);
        free(curr);
        curr = next;
    }
    free(list);
}

void print_iloc_code(FILE *stream, iloc_list_t *list) {
    if (!list || !list->head) {
        return;
    }

    iloc_instr_t *current = list->head;

    while (current != NULL) {
        // Print the label if it exists
        if (current->label) {
            fprintf(stream, "%s:", current->label);
        }

        // --- Handle Workaround Cases ---
        if(current->opcode == NULL){
            fprintf(stream, "nop\n");
        }else if (strcmp(current->opcode, "cbr") == 0) {
            // WORKAROUND: cbr cond_reg -> label1, label2
            // Mapping:   opcode arg1 -> arg2, result
            fprintf(stream, "%s %s -> %s, %s\n", current->opcode, current->arg1, current->arg2, current->result);
        } else if (strcmp(current->opcode, "storeAI") == 0 || strcmp(current->opcode, "storeAO") == 0) {
            // WORKAROUND: storeAI src_reg => base_reg, offset
            // Mapping:   opcode arg1 => arg2, result
            fprintf(stream, "%s %s => %s, %s\n", current->opcode, current->arg1, current->arg2, current->result);
        }
        
        // --- Handle Standard Control Flow (single arrow) ---
        else if (strncmp(current->opcode, "cmp_", 4) == 0) {
            // Standard: cmp_xx r1, r2 -> r3
            // Mapping: opcode arg1, arg2 -> result
            fprintf(stream, "%s %s, %s -> %s\n", current->opcode, current->arg1, current->arg2, current->result);
        } else if (strcmp(current->opcode, "jumpI") == 0 || strcmp(current->opcode, "jump") == 0) {
            // Standard: jumpI -> label
            // Mapping: opcode -> result
            fprintf(stream, "%s -> %s\n", current->opcode, current->result);
        } else if (strcmp(current->opcode, "nop") == 0) {
            fprintf(stream, " %s\n", current->opcode);
        } 

        // --- Handle Arithmetic Operations (double arrow) ---
        else if (strcmp(current->opcode, "add") == 0 || strcmp(current->opcode, "sub") == 0 ||
                 strcmp(current->opcode, "mult") == 0 || strcmp(current->opcode, "div") == 0 ||
                 strcmp(current->opcode, "addI") == 0 || strcmp(current->opcode, "subI") == 0 ||
                 strcmp(current->opcode, "rsubI") == 0 || strcmp(current->opcode, "multI") == 0 ||
                 strcmp(current->opcode, "divI") == 0 || strcmp(current->opcode, "rdivI") == 0 ||
                 strcmp(current->opcode, "lshift") == 0 || strcmp(current->opcode, "lshiftI") == 0 ||
                 strcmp(current->opcode, "rshift") == 0 || strcmp(current->opcode, "rshiftI") == 0) {
            // Format: op r1, r2 => r3  or  opI r1, c => r3
            fprintf(stream, "%s %s, %s => %s\n", current->opcode, current->arg1, current->arg2, current->result);
        }

        // --- Handle Load Operations ---
        else if (strcmp(current->opcode, "loadI") == 0) {
            // Format: loadI c => r
            fprintf(stream, "%s %s => %s\n", current->opcode, current->arg1, current->result);
        } else if (strcmp(current->opcode, "load") == 0 || strcmp(current->opcode, "loadAI") == 0 ||
                   strcmp(current->opcode, "loadAO") == 0) {
            // Format: loadAI r1, c => r2
            fprintf(stream, "%s %s, %s => %s\n", current->opcode, current->arg1, current->arg2, current->result);
        }

        // --- Handle All Other Standard Operations (double arrow) ---
        else {
            fprintf(stream, "%s ", current->opcode);

            // Print source operands
            if (current->arg1) {
                fprintf(stream, "%s", current->arg1);
            }
            if (current->arg2) {
                // arg2 is a source operand in the general case
                fprintf(stream, ", %s", current->arg2);
            }

            // Print destination operand
            if (current->result) {
                fprintf(stream, " => %s", current->result);
            }
            fprintf(stream, "\n");
        }
        current = current->next;
    }
}