#ifndef __CODE_H__
#define __CODE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table.h"
#include "iloc.h"

// Function prototypes for code generation using ILOC instruction lists

// Binary operations
iloc_list_t* gen_binary_op(const char* op, const char* instr,
                          iloc_list_t* left_code, char* left_reg,
                          iloc_list_t* right_code, char* right_reg,
                          char** result_reg);

// Unary operations
iloc_list_t* gen_unary_op(const char* op, const char* instr,
                         iloc_list_t* child_code, char* child_reg,
                         char** result_reg);

// Constants and variables
iloc_list_t* gen_const(int value, char** result_reg);
iloc_list_t* gen_var(table_stack_t* stack, const char* var_name, char** result_reg);

// Assignment
iloc_list_t* gen_assign(table_stack_t* stack, const char* var_name, iloc_list_t* expr_code, char* expr_reg);

// Control structures
iloc_list_t* gen_if(iloc_list_t* cond_code, char* cond_reg,
                   iloc_list_t* then_code, iloc_list_t* else_code);
iloc_list_t* gen_while(iloc_list_t* cond_code, char* cond_reg, iloc_list_t* body_code);

// Helper functions
char* new_reg();  // For temporary register allocation

#endif // __CODE_H__