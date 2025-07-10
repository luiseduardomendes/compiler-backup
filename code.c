#include "code.h"
#include "label.h"
#include "iloc.h"
#include "table.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Global register counter
static int reg_counter = 0;

// Get a new temporary register
char* new_reg() {
    char* reg = malloc(16);
    sprintf(reg, "r%d", reg_counter++);
    return reg;
}

// Generate code for binary operations
iloc_list_t* gen_binary_op(const char* op, const char* instr, iloc_list_t* left_code, char* left_reg, 
                          iloc_list_t* right_code, char* right_reg, char** result_reg) {
    iloc_list_t* code = NULL;
    iloc_list_t* aux_code = NULL;
    if (left_code) {
      aux_code = concat_iloc(code, left_code);
      free_iloc_list(code);
      code = aux_code;
   }
    if (right_code) {
      aux_code = concat_iloc(code, right_code);
      free_iloc_list(code);
      code = aux_code;
   }
   if (code == NULL) code = new_iloc_list();
    *result_reg = new_reg();
    append_iloc(code, make_iloc(NULL, instr, left_reg, right_reg, *result_reg));
    return code;
}

// Generate code for unary operations
iloc_list_t* gen_unary_op(const char* op, const char* instr, iloc_list_t* child_code, char* child_reg, char** result_reg) {
    iloc_list_t* code = NULL;
    if (child_code) code = concat_iloc(code, child_code);
    else code = new_iloc_list();

    *result_reg = new_reg();
    
    append_iloc(code, make_iloc(NULL, instr, child_reg, NULL, *result_reg));
    return code;
}

// Generate code for constants
iloc_list_t* gen_const(int value, char** result_reg) {
    iloc_list_t* code = new_iloc_list();
    *result_reg = new_reg();
    char *val_str = malloc(16);
    sprintf(val_str, "%d", value);
    append_iloc(code, make_iloc(NULL, "loadI", val_str, NULL, *result_reg));
    free(val_str);
    return code;
}

// Generate code for variables
iloc_list_t* gen_var(table_stack_t* stack, const char* var_name, char** result_reg) {
    iloc_list_t* code = new_iloc_list();
    *result_reg = new_reg();

    entry_t *entry = search_table_stack(stack, var_name);
    
    char* base_reg = get_base_of(stack, var_name);
    
     if (strcmp(base_reg, "rbss") == 0) { // Global variable
        append_iloc(code, make_iloc(NULL, "loadAG", (char*)var_name, NULL, *result_reg));
    } else { // Local variable
        int offset = entry->offset;
        char *offset_str = malloc(16);
        // Negate offset for local variables, as they are on the stack relative to rbp
        sprintf(offset_str, "%d", -offset);
        append_iloc(code, make_iloc(NULL, "loadAI", base_reg, offset_str, *result_reg));
        free(offset_str);
    }
    return code;
}

// Generate code for assignment
iloc_list_t* gen_assign(table_stack_t* stack, const char* var_name, iloc_list_t* expr_code, char* expr_reg) {
   iloc_list_t* code = NULL;
   if (expr_code) {
      code = concat_iloc(code, expr_code);
   } else {
      code = new_iloc_list();
   }
   entry_t *entry = search_table_stack(stack, (char*)var_name);
   char* base_reg = get_base_of(stack, var_name);

   if (strcmp(base_reg, "rbss") == 0) { // Global variable
      append_iloc(code, make_iloc(NULL, "storeAG", expr_reg, (char*)var_name, NULL));
   } else { // Local variable
      int offset = entry->offset;
      char *offset_str = malloc(16);
      // Negate offset for local variables
      sprintf(offset_str, "%d", -offset); //ToDo: precisa? sim
      // storeAI src_reg => base_reg, offset_str
      append_iloc(code, make_iloc(NULL, "storeAI", expr_reg, base_reg, offset_str));
      free(offset_str);
   }
   return code;
}

iloc_list_t* gen_if(iloc_list_t* cond_code, char* cond_reg, iloc_list_t* then_code, iloc_list_t* else_code) {
    iloc_list_t* code = new_iloc_list();
    iloc_list_t* code_aux = NULL;
    char* then_label = new_label();  // Label for then block
    char* else_label = new_label();  // Label for else block (if exists)
    char* end_label = new_label();   // Label to jump to after then block

    // Add condition code
    if (cond_code) {
        code_aux = concat_iloc(code, cond_code);
        free_iloc_list(code);
        code = code_aux;
    }

    // Branch: if true go to then, else go to else/end
    if (else_code) {
        append_iloc(code, make_iloc(NULL, "cbr", cond_reg, then_label, else_label));
    } else {
        append_iloc(code, make_iloc(NULL, "cbr", cond_reg, then_label, end_label));
    }

    // Then block
    append_iloc(code, make_iloc(then_label, "nop", NULL, NULL, NULL));
    if (then_code) {
        code_aux = concat_iloc(code, then_code);
        free_iloc_list(code);
        code = code_aux;
    }
    
    // Jump to end (skip else block if it exists)
    if (else_code) {
        append_iloc(code, make_iloc(NULL, "jumpI", NULL, NULL, end_label));
    }

    // Else block (if exists)
    if (else_code) {
        append_iloc(code, make_iloc(else_label, "nop", NULL, NULL, NULL));
        code_aux = concat_iloc(code, else_code);
        free_iloc_list(code);
        code = code_aux;
    }

    // End label
    append_iloc(code, make_iloc(end_label, "nop", NULL, NULL, NULL));

    free(then_label);
    free(else_label);
    free(end_label);
    return code;
}

// Generate code for while loop
iloc_list_t* gen_while(iloc_list_t* cond_code, char* cond_reg, iloc_list_t* body_code) {
    iloc_list_t* code = new_iloc_list();
    iloc_list_t* code_aux = NULL;
    char* start_label = new_label();   // Label for loop start
    char* true_label = new_label();    // Label for when condition is true
    char* end_label = new_label();     // Label for when condition is false (loop end)

    // Loop start
    append_iloc(code, make_iloc(start_label, "nop", NULL, NULL, NULL));
    
    // Condition code
    if (cond_code) {
        code_aux = concat_iloc(code, cond_code);
        free_iloc_list(code);
        code = code_aux;
    }
    
    // Branch: if true go to body, else go to end
    append_iloc(code, make_iloc(NULL, "cbr", cond_reg, true_label, end_label));
    
    // True case label (body)
    append_iloc(code, make_iloc(true_label, "nop", NULL, NULL, NULL));
    
    // Body code
    if (body_code) {
        code_aux = concat_iloc(code, body_code);
        free_iloc_list(code);
        code = code_aux;
    }
    
    // Jump back to start
    append_iloc(code, make_iloc(NULL, "jumpI", NULL, NULL, start_label));
    
    // End label
    append_iloc(code, make_iloc(end_label, "nop", NULL, NULL, NULL));
    
    free(start_label);
    free(true_label);
    free(end_label);
    return code;
}

iloc_list_t *gen_return(iloc_list_t *expr_code, char *expr_reg) {
    iloc_list_t *code = new_iloc_list();
    iloc_list_t *code_aux = NULL;

    if (expr_code) {
        code_aux = concat_iloc(code, expr_code);
        free_iloc_list(code);
        code = code_aux;
    }

    append_iloc(code, make_iloc(NULL, "return", expr_reg, NULL, NULL));
    return code;
}