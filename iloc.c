#include "iloc.h"
#include "table.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

const char* iloc_cmp_to_jmp(const char* cmp) {
    if (strcmp(cmp, "cmp_EQ") == 0) return "je";
    if (strcmp(cmp, "cmp_NE") == 0) return "jne";
    if (strcmp(cmp, "cmp_LT") == 0) return "jl";
    if (strcmp(cmp, "cmp_LE") == 0) return "jle";
    if (strcmp(cmp, "cmp_GT") == 0) return "jg";
    if (strcmp(cmp, "cmp_GE") == 0) return "jge";
    return "jne"; // fallback
}

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

// --- Register Allocation ---

#define MAX_VIRTUAL_REGS 256
#define NUM_PHYSICAL_REGS 8

// Registradores físicos disponíveis para alocação
static const char* physical_regs[NUM_PHYSICAL_REGS] = {
    "%r8d", "%r9d", "%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d"
};
static int next_physical_reg_idx = 0;

// Mapeamento de registradores virtuais para físicos
typedef struct {
    char* virtual_reg;
    const char* physical_reg;
} reg_map_entry_t;

static reg_map_entry_t reg_map[MAX_VIRTUAL_REGS];
static int reg_map_size = 0;

// Função para obter o registrador físico a partir de um virtual
const char* get_x86_reg(const char* virtual_reg) {
    if (virtual_reg == NULL) return "";
    if (strcmp(virtual_reg, "rfp") == 0) return "%rbp"; // rfp é sempre rbp

    // Procura por um mapeamento existente
    for (int i = 0; i < reg_map_size; i++) {
        if (strcmp(reg_map[i].virtual_reg, virtual_reg) == 0) {
            return reg_map[i].physical_reg;
        }
    }

    // Cria um novo mapeamento se não existir
    if (reg_map_size < MAX_VIRTUAL_REGS) {
        const char* physical_reg = physical_regs[next_physical_reg_idx];
        next_physical_reg_idx = (next_physical_reg_idx + 1) % NUM_PHYSICAL_REGS;

        reg_map[reg_map_size].virtual_reg = strdup(virtual_reg);
        reg_map[reg_map_size].physical_reg = physical_reg;
        return reg_map[reg_map_size++].physical_reg;
    }

    fprintf(stderr, "Erro: Excesso de registradores virtuais!\n"); //ToDo: precisa?
    exit(EXIT_FAILURE);
}

void free_register_map() {
    for (int i = 0; i < reg_map_size; i++) {
        free(reg_map[i].virtual_reg);
    }
    reg_map_size = 0;
    next_physical_reg_idx = 0;
}

// --- X86 Code Generation ---
void print_x86_code(FILE *stream, iloc_list_t *list) {
    if (!list || !list->head) {
        return;
    }
    // --- X86 Assembly Header ---
    fprintf(stream, "    .text\n");
    fprintf(stream, "    .globl main\n");
    fprintf(stream, "main:\n");
    fprintf(stream, "    pushq %%rbp\n");
    fprintf(stream, "    movq %%rsp, %%rbp\n");
    // Aloca espaço na pilha para variáveis locais (ex: 128 bytes)
    fprintf(stream, "    subq $128, %%rsp\n\n");

    iloc_instr_t *current = list->head;
    const char* last_cmp = NULL;

    while (current != NULL) {
        // Print the label if it exists (skip if it's "main" since already printed)
        if (current->label) {
            fprintf(stream, "%s:\n", current->label);
        }

        // --- Arithmetic Operations ---
        if (strcmp(current->opcode, "add") == 0) {
            fprintf(stream, "    movl %s, %s\n", get_x86_reg(current->arg1), get_x86_reg(current->result));
            fprintf(stream, "    addl %s, %s\n", get_x86_reg(current->arg2), get_x86_reg(current->result));
        } else if (strcmp(current->opcode, "sub") == 0) {
            fprintf(stream, "    movl %s, %s\n", get_x86_reg(current->arg1), get_x86_reg(current->result));
            fprintf(stream, "    subl %s, %s\n", get_x86_reg(current->arg2), get_x86_reg(current->result));
        } else if (strcmp(current->opcode, "mult") == 0) {
            fprintf(stream, "    movl %s, %s\n", get_x86_reg(current->arg1), get_x86_reg(current->result));
            fprintf(stream, "    imull %s, %s\n", get_x86_reg(current->arg2), get_x86_reg(current->result));
        } else if (strcmp(current->opcode, "div") == 0) {
            fprintf(stream, "    movl %s, %%eax\n", get_x86_reg(current->arg1));
            fprintf(stream, "    cltd\n");
            fprintf(stream, "    idivl %s\n", get_x86_reg(current->arg2));
            fprintf(stream, "    movl %%eax, %s\n", current->result);
        // --- Memory ---
        } else if (strcmp(current->opcode, "loadI") == 0) {
            fprintf(stream, "    movl $%s, %s\n", current->arg1, get_x86_reg(current->result));
        } else if (strcmp(current->opcode, "loadAI") == 0) { // Local vars
            fprintf(stream, "    movl %s(%s), %s\n", current->arg2, get_x86_reg(current->arg1), get_x86_reg(current->result));
        } else if (strcmp(current->opcode, "storeAI") == 0) { // Local vars
            fprintf(stream, "    movl %s, %s(%s)\n", get_x86_reg(current->arg1), current->result, get_x86_reg(current->arg2));
        } else if (strcmp(current->opcode, "loadAG") == 0) { // Global vars
            fprintf(stream, "    movl %s(%%rip), %s\n", current->arg1, get_x86_reg(current->result));
        } else if (strcmp(current->opcode, "storeAG") == 0) { // Global vars
            fprintf(stream, "    movl %s, %s(%%rip)\n", get_x86_reg(current->arg1), current->arg2);
        // --- Control Flow ---
        } else if (strcmp(current->opcode, "jumpI") == 0 || strcmp(current->opcode, "jump") == 0) {
            fprintf(stream, "    jmp %s\n", current->result);
        } else if (strncmp(current->opcode, "cmp_", 4) == 0) {
            fprintf(stream, "    cmpl %s, %s\n", get_x86_reg(current->arg2), get_x86_reg(current->arg1));
            last_cmp = current->opcode;
        } else if (strcmp(current->opcode, "cbr") == 0) {
            const char* jmp = iloc_cmp_to_jmp(last_cmp ? last_cmp : "cmp_NE");
            fprintf(stream, "    %s %s\n", jmp, current->arg2); // true branch
            fprintf(stream, "    jmp %s\n", current->result);   // false branch
        } else if (strcmp(current->opcode, "nop") == 0 || current->opcode == NULL) {
            fprintf(stream, "    nop\n");
        } else {
            fprintf(stream, "    # UNHANDLED: %s\n", current->opcode);
        }
        current = current->next;
    }

    // --- X86 Assembly Footer ---
    fprintf(stream, "    movl $0, %%eax\n");
    fprintf(stream, "    popq %%rbp\n");
    fprintf(stream, "    ret\n");
}

// Emits the .data section for all global variables in the symbol table stack
void print_x86_data_section(FILE *stream, table_stack_t *stack) {
    bool has_globals = false;
    table_stack_t *ts_check = stack;
    while(ts_check != NULL) {
        table_t *table = ts_check->top;
        if(table) {
            for (int i = 0; i < table->num_entries; i++) {
                if (table->entries[i] && table->entries[i]->is_global) {
                    has_globals = true;
                    break;
                }
            }
        }
        if(has_globals) break;
        ts_check = ts_check->next;
    }

    if(!has_globals) return;

    fprintf(stream, "    .data\n");
    for (table_stack_t *ts = stack; ts != NULL; ts = ts->next) {
        table_t *table = ts->top;
        if (!table) continue;
        for (int i = 0; i < table->num_entries; i++) {
            entry_t *entry = table->entries[i];
            if (entry && entry->is_global) {
                fprintf(stream, "    .globl %s\n", entry->value->lexema);
                fprintf(stream, "    .align 4\n");
                fprintf(stream, "%s:\n", entry->value->lexema);
                fprintf(stream, "    .long 0\n"); // Initialize globals to 0
            }
        }
    }
    fprintf(stream, "\n");
}

