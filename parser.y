// Nomes: 
//   - Leonardo Kauer Leffa
//   - Luis Eduardo Pereira Mendes
// 
//   turma: B
//   

%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "asd.h"
    #include "errors.h"
    #include "table.h"
    #include "valor_t.h"
    #include "parser.tab.h"
    #include "type.h"
    #include "label.h"
    #include "colors.h"
    #include "code.h"
    #include "iloc.h"

    #define VAR_SIZE 4 // Assuming each variable takes 4 bytes
    #define LOCAL    0
    #define GLOBAL   1

    int get_line_number();
    int yylex(void);
    void yyerror (char const *mensagem);

%}

%{
    #include "table.h"
    static int global_offset = 0;
    static int local_offset = 0; 
    extern asd_tree_t *arvore;
    table_stack_t *stack;
    type_t   type_current_function;
    entry_t *entry_current_function;
    args_t  *args_current_function;

    // Add these helper functions
    static char *safe_strconcat(const char *s1, const char *s2) {
        char *result;
        if (asprintf(&result, "%s%s", s1, s2) == -1) {
            yyerror("String concatenation failed");
            return NULL;
        }
        return result;
    }
%}

%define parse.error verbose

%code requires {
    #include "asd.h"
    #include "errors.h"
    #include "table.h"
    #include "parser.tab.h"
    #include "type.h"
    #include "valor_t.h"
    #include "iloc.h"
}

%union {
    asd_tree_t *no; 
    valor_t *valor_lexico;
    args_t *args;
    type_t *type;
};

%token TK_PR_AS
%token TK_PR_DECLARE
%token TK_PR_ELSE
%token TK_PR_FLOAT
%token TK_PR_IF
%token TK_PR_INT
%token TK_PR_IS
%token TK_PR_RETURN
%token TK_PR_RETURNS
%token TK_PR_WHILE
%token TK_PR_WITH
%token TK_OC_LE
%token TK_OC_GE
%token TK_OC_EQ
%token TK_OC_NE
%token TK_ER

%token <valor_lexico>TK_ID
%token <valor_lexico>TK_LI_INT
%token <valor_lexico>TK_LI_FLOAT

%type<no> programa
%type<no> nivel0
%type<no> nivel1
%type<no> nivel2
%type<no> nivel3
%type<no> nivel4
%type<no> nivel5
%type<no> nivel6
%type<no> nivel7
%type<no> expressao
%type<no> termo
%type<no> argumento
%type<no> comando_atribuicao
%type<no> comando_retorno
%type<no> comando_simples
%type<no> comandos_controle_fluxo
%type<no> chamada_funcao
%type<no> sequencia_comandos
%type<no> lista_argumentos
%type<no> lista_elementos
%type<no> elementos_programa
%type<no> sequencia_opcional_comandos
%type<no> bloco_comandos
%type<no> declaracao_variavel
%type<no> declaracao_variavel_local
%type<no> definicao_funcao
%type<no> corpo_funcao
%type<no> cabecalho_funcao
%type<no> declaracao_variavel_global
%type<no> literal
%type<no> pop
%type<no> push
%type<type> tipo
%type<args> parametro
%type<args> lista_parametros
%type<args> lista_opcional_parametros

%destructor {
    if($$ != NULL && $$ != arvore){
        asd_free($$);
    }
    free_table_stack(stack);
} <no>;

%start programa

%%
//-----------------------------------------------------------------------------------------------------------------------
//  Programa na linguagem
//-----------------------------------------------------------------------------------------------------------------------
programa: 
    push lista_elementos pop ';' { $$ = $2; arvore = $$; stack = new_table_stack();} | 
    /*epsilon*/                  { $$ = NULL; arvore = $$; }
    ;
    
lista_elementos: 
    elementos_programa ',' lista_elementos {

        if ($1 != NULL && $3 != NULL) {
            iloc_list_t* code = NULL;
            asd_add_child($1, $3);
            code = concat_iloc($1->code, $3->code); 
            free_iloc_list($1->code);
            $1->code = code;
            $$ = $1;
        } else if ($1 != NULL) {
            $$ = $1;
        } else {
            $$ = $3;
        }
    } |
    elementos_programa {
        $$ = $1;
        
    };

elementos_programa: 
    definicao_funcao           { $$ = $1; } | 
    declaracao_variavel_global { asd_free($1); $$ = NULL; };

//-----------------------------------------------------------------------------------------------------------------------
// Usados em toda a linguagem
//-----------------------------------------------------------------------------------------------------------------------
tipo: 
    TK_PR_FLOAT {
        type_t *type = malloc(sizeof(type_t));
        *type = FLOAT;
        $$ = type;
    } | 
    TK_PR_INT {
        type_t *type = malloc(sizeof(type_t));
        *type = INT;
        $$ = type;
    };

bloco_comandos: 
    '[' push sequencia_opcional_comandos pop ']' { $$ = $3; };

literal:
    // TODO: Acho que deve ser definido AQUI onde eh o registrador no qual sera armazenado o valor final do literal, que sera posteriormente usado no store
    TK_LI_INT   {
        $$ = asd_new($1->lexema, INT, NULL, NULL);
        int value = atoi($1->lexema);
        $$->code = gen_const(value, &($$->place));
        free_valor($1);
    } | 
    TK_LI_FLOAT { $$ = asd_new($1->lexema, FLOAT, NULL, NULL); free_valor($1); } ;
//-----------------------------------------------------------------------------------------------------------------------
// Definicao de Funcao
//-----------------------------------------------------------------------------------------------------------------------

definicao_funcao:
    cabecalho_funcao push corpo_funcao pop
    {
        entry_t *entry;
        
        $$ = $1;

        if($3 != NULL) {
            asd_add_child($$, $3);
            $$->code = copy_iloc_list($3->code);
        }

        entry = new_entry(entry_current_function->line, N_FUNC, entry_current_function->type, entry_current_function->value, args_current_function, GLOBAL, 0);

        free_valor(entry_current_function->value);
        free(entry_current_function);
        
        entry_current_function = entry;

        add_entry(stack->top, entry_current_function);
        

    };

cabecalho_funcao:
    TK_ID TK_PR_RETURNS tipo lista_opcional_parametros TK_PR_IS
    {
        entry_t *entry = search_table(stack->top, $1->lexema);

        if (entry != NULL) {
            printf("%sERR_DECLARED : Line: %d\nFunction <%s> already declared%s\n", RED, get_line_number(), $1->lexema, RESET);
            free_valor($1);
            free($3);
            exit(ERR_DECLARED);
        }
        
        entry = new_entry(get_line_number(), N_FUNC, *($3), $1, NULL, GLOBAL, 0);
        $$ = asd_new($1->lexema, *($3), NULL, NULL);
        
        free_args(args_current_function);
        type_current_function  = *($3);
        args_current_function  = $4;
        entry_current_function = entry;
                

        free_valor($1);
        free($3);
    };

corpo_funcao:
    '[' sequencia_opcional_comandos ']'
    {
         $$ = $2;
    };

lista_opcional_parametros:
    TK_PR_WITH lista_parametros {$$ = $2;} |
    /*epsilon*/{ $$ = NULL; } ;

lista_parametros: 
    parametro ',' lista_parametros{
        $$ = add_arg($1, $3->value, $3->type);
        free_args($3);
    }| 
    parametro{
        $$ = $1;
    };

parametro:
    TK_ID TK_PR_AS tipo{
         entry_t *entry = search_table(stack->top, $1->lexema);
        if (entry != NULL){
            printf("%sERR_DECLARED : Line: %d\nParameter <%s> already declared%s\n", RED, get_line_number(), $1->lexema, RESET);
            free($3);
            free_valor($1);
            exit(ERR_DECLARED);
        }

        $$ = create_arg($1, *($3));

        local_offset       += VAR_SIZE;

        entry = new_entry(get_line_number(), N_VAR, *($3), $1, NULL, LOCAL, local_offset);

        add_entry(stack->top, entry);
        free_valor($1);
        free($3);
    };

//-----------------------------------------------------------------------------------------------------------------------
// Declaracao de variavel global
//-----------------------------------------------------------------------------------------------------------------------
declaracao_variavel_global: 
    TK_PR_DECLARE TK_ID TK_PR_AS tipo {
        entry_t *entry = search_table(stack->top, $2->lexema);

        global_offset += VAR_SIZE; // Atualiza o contador global

        if (entry != NULL || (stack->next != NULL && args_current_function != NULL && contains_in_args(args_current_function, $2->lexema) == 1)){
            printf("%sERR_DECLARED : Line: %d\nVariable <%s> already declared%s\n", RED, get_line_number(), $2->lexema, RESET);
            free($4);
            free_valor($2);
            exit(ERR_DECLARED);
        } else {
            entry = new_entry(get_line_number(), N_VAR, *($4), $2, NULL, GLOBAL, global_offset);
            add_entry(stack->top, entry);
        }
        $$ = asd_new($2->lexema, *($4), NULL, NULL); 
        free_valor($2);
        free($4);
    };

//-----------------------------------------------------------------------------------------------------------------------
// Comandos Simples
//-----------------------------------------------------------------------------------------------------------------------
sequencia_opcional_comandos:
    sequencia_comandos  { $$ = $1;   } |
    /*epsilon*/         { $$ = NULL; } ; 

sequencia_comandos:
    comando_simples {
        $$ = $1;
    }|
    comando_simples sequencia_comandos {
        iloc_list_t* code = NULL;
        if ($1 != NULL && $2 != NULL) {            
            asd_add_child($$, $2);
            code = concat_iloc($1->code, $2->code); 
            free_iloc_list($1->code);
            $1->code = code;
            $$ = $1;
        } else if ($1 != NULL) {
            $$ = $1;
        } else {
            $$ = $2;
        }
    };

comando_simples:
    bloco_comandos          { $$ = $1; } | 
    declaracao_variavel     { $$ = $1; } | 
    comando_atribuicao      { $$ = $1; } | 
    comando_retorno         { $$ = $1; } | 
    chamada_funcao          { $$ = $1; } | 
    comandos_controle_fluxo { $$ = $1; } ;

declaracao_variavel:
    declaracao_variavel_local { asd_free($1); $$ = NULL; } | 
    declaracao_variavel_local TK_PR_WITH literal { 
        $$ = asd_new("with", $3->type, gen_assign(stack, $1->label, $3->code, $3->place), NULL);
        if ($1->type != $3->type){
            printf("%sERR_WRONG_TYPE : Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);
            exit(ERR_WRONG_TYPE);}
        if ($1 != NULL){
            asd_add_child($$, $1);
        }
        if ($3 != NULL){
            asd_add_child($$, $3);
        }
    };

declaracao_variavel_local: TK_PR_DECLARE TK_ID TK_PR_AS tipo {
        entry_t *entry = search_table(stack->top, $2->lexema);

         local_offset += VAR_SIZE; // Atualiza o contador local

        if (entry != NULL || (stack->next != NULL && args_current_function != NULL && contains_in_args(args_current_function, $2->lexema) == 1)){
            printf("%sERR_DECLARED : Line: %d\nVariable <%s> already declared%s\n", RED, get_line_number(), $2->lexema, RESET);
            free($4);
            free_valor($2);
            exit(ERR_DECLARED);
        } else {
            entry = new_entry(get_line_number(), N_VAR, *($4), $2, NULL, LOCAL, local_offset);
            add_entry(stack->top, entry);
        }
        $$ = asd_new($2->lexema, *($4), NULL, NULL); 
        free_valor($2);
        free($4);
    };

comando_atribuicao:
    TK_ID TK_PR_IS expressao {
        entry_t *entry_id = search_table_stack(stack, $1->lexema);

        if(entry_id == NULL){
            if(strcmp(entry_current_function->value->lexema, $1->lexema) == 0){
                entry_id = entry_current_function;
            }
            else{
                printf("%sERR_UNDECLARED : Line: %d\nVariable <%s> not declared%s\n", RED, get_line_number(), $1->lexema, RESET);
                free_valor($1);
                exit(ERR_UNDECLARED);
            }
        }
        if (entry_id->nature == N_FUNC){
            printf("%sERR_FUNCTION : Line: %d\nUsing declared function <%s> as variable%s\n", RED, get_line_number(), $1->lexema, RESET);
            free_valor($1);
            exit(ERR_FUNCTION);
        }
        if (entry_id->type != $3->type){
            free_valor($1);
            printf("%sERR_WRONG_TYPE : Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type(entry_id->type), dcd_type($3->type), RESET);
            exit(ERR_WRONG_TYPE);
        }

        $$ = asd_new("is", entry_id->type, NULL, NULL);  // TODO: Placeholder
        if ($1 != NULL && $3 != NULL){
            asd_add_child($$, asd_new(entry_id->value->lexema, entry_id->type, NULL, NULL)); 
            $$->code = gen_assign(stack, $1->lexema, $3->code, $3->place);
            asd_add_child($$, $3);
        }
        free_valor($1);
    } ; 

chamada_funcao: 
    TK_ID '(' lista_argumentos ')'  { 
        entry_t *entry = search_table_stack(stack, $1->lexema);
        args_t *args;
        
        if(entry_current_function != NULL && strcmp(entry_current_function->value->lexema, $1->lexema) == 0){
            entry = entry_current_function;
            args  = args_current_function;
        }
        else if(entry != NULL && entry->args != NULL){
            args = entry->args;
        }
        else{
            args = NULL;
        }

        if (entry == NULL){
            printf("%sERR_UNDECLARED : Line: %d\nFunction <%s> not declared%s\n", RED, get_line_number(), $1->lexema, RESET);
            free_valor($1);
            exit(ERR_UNDECLARED);
        }
        if (entry->nature == N_VAR){
            printf("%sERR_VARIABLE Line: %d\nUsing declared variable <%s> as function%s", RED, get_line_number(), $1->lexema, RESET);
            free_valor($1);
            exit(ERR_VARIABLE);
        }
        char *func_name = safe_strconcat("call ", $1->lexema);

        switch(compare_args(args, $3)){
            case ERR_WRONG_TYPE_ARGS: 
                free(func_name);
                free_valor($1);
                printf("%sERR_WRONG_TYPE_ARGS : Line: %d%s\n", RED, get_line_number(), RESET);
                exit(ERR_WRONG_TYPE_ARGS);
            case ERR_MISSING_ARGS: 
                free(func_name);
                free_valor($1);
                printf("%sERR_MISSING_ARGS : Line: %d%s\n", RED, get_line_number(), RESET);
                exit(ERR_MISSING_ARGS);
            case ERR_EXCESS_ARGS: 
                free(func_name);
                free_valor($1);
                printf("%sERR_EXCESS_ARGS : Line: %d%s\n", RED, get_line_number(), RESET);
                exit(ERR_EXCESS_ARGS);
            case 0: break;
        }
        
        $$ = asd_new(func_name, entry->type, NULL, NULL);
        if ($3 != NULL){
            asd_add_child($$, $3); 
            $$->code = $3->code; 
        }
        free(func_name);
        free_valor($1);
    } |
    TK_ID '(' ')'  { 
        entry_t *entry = search_table_stack(stack, $1->lexema);

        args_t *args;

        if(entry_current_function != NULL && strcmp(entry_current_function->value->lexema, $1->lexema) == 0){
            entry = entry_current_function;
            args  = args_current_function;
        }
        else if(entry != NULL && entry->args != NULL){
            args = entry->args;
        }
        else{
            args = NULL;
        }

        if (entry == NULL){
            printf("%sERR_UNDECLARED : Line: %d\nFunction <%s> not declared%s\n", RED, get_line_number(), $1->lexema, RESET);
            free_valor($1);
            exit(ERR_UNDECLARED);
        }
        if (entry->nature == N_VAR){
            printf("%sERR_VARIABLE Line: %d\nUsing declared variable <%s> as function%s", RED, get_line_number(), $1->lexema, RESET);
            free_valor($1);
            exit(ERR_VARIABLE);
        }
        char *func_name = safe_strconcat("call ", $1->lexema);

        switch(compare_args(args, NULL)){
            case ERR_WRONG_TYPE_ARGS: 
                free(func_name);
                free_valor($1);
                printf("%sERR_WRONG_TYPE_ARGS : Line: %d%s\n", RED, get_line_number(), RESET);
                exit(ERR_WRONG_TYPE_ARGS);
            case ERR_MISSING_ARGS: 
                free(func_name);
                free_valor($1);
                printf("%sERR_MISSING_ARGS : Line: %d%s\n", RED, get_line_number(), RESET);
                exit(ERR_MISSING_ARGS);
            case ERR_EXCESS_ARGS: 
                free(func_name);
                free_valor($1);
                printf("%sERR_EXCESS_ARGS : Line: %d%s\n", RED, get_line_number(), RESET);
                exit(ERR_EXCESS_ARGS);
            case 0: break;
        }
        
        $$ = asd_new(func_name, entry->type, NULL, NULL);
        free(func_name);
        free_valor($1);
    };

lista_argumentos:
    argumento ',' lista_argumentos  { 
        $$ = $1; 
        asd_add_child($$, $3); } |
    argumento                       { 
        $$ = $1;
    };

argumento:
    expressao { $$ = $1; };

comando_retorno:
    TK_PR_RETURN expressao TK_PR_AS tipo { 
        if(type_current_function != *($4)){
            printf("%sERR_WRONG_TYPE : Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type(type_current_function), dcd_type(*($4)), RESET);
            free($4);
            exit(ERR_WRONG_TYPE);
        }
        if(type_current_function != $2->type){
            printf("%sERR_WRONG_TYPE : Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type(type_current_function), dcd_type($2->type), RESET);
            free($4);
            exit(ERR_WRONG_TYPE);
        }
        $$ = asd_new("return", type_current_function, $2 ? $2->code : NULL, $2 ? $2->place : NULL);
        if ($2 != NULL){
            asd_add_child($$, $2); 
        }
        free($4);
    };

comandos_controle_fluxo: 
    TK_PR_IF '(' expressao ')' bloco_comandos TK_PR_ELSE bloco_comandos { 
        if ($5 != NULL && $7 != NULL && $5->type != $7->type){
            printf("%sERR_WRONG_TYPE : Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($5->type), dcd_type($7->type), RESET);
            exit(ERR_WRONG_TYPE);
        }
        $$ = asd_new("if", $3->type, NULL, NULL);
        $$->code = gen_if($3->code, $3->place, $5 ? $5->code : NULL, $7 ? $7->code : NULL);
        asd_add_child($$, $3);
        if ($5) asd_add_child($$, $5);
        if ($7) asd_add_child($$, $7);
    } |  
    TK_PR_IF '(' expressao ')' bloco_comandos{ 
        $$ = asd_new("if", $3->type, NULL, NULL);
        $$->code = gen_if($3->code, $3->place, $5 ? $5->code : NULL, NULL);
        asd_add_child($$, $3);
        if ($5) asd_add_child($$, $5);
    } |
    TK_PR_WHILE '(' expressao ')' bloco_comandos{ 
        $$ = asd_new("while", $3->type, NULL, NULL);
        $$->code = gen_while($3->code, $3->place, $5 ? $5->code : NULL);
        asd_add_child($$, $3);
        if ($5) asd_add_child($$, $5);
    } ;

termo:
    TK_ID {
        entry_t *entry = search_table_stack(stack, $1->lexema);
        if (entry == NULL){
            printf("%sERR_UNDECLARED : Line: %d\nVariable <%s> not declared%s\n", RED, get_line_number(), $1->lexema, RESET);
            free_valor($1);
            exit(ERR_UNDECLARED);
        }
        if (entry->nature != N_VAR){
            printf("%sERR_FUNCTION Line: %d\nUsing declared function <%s> as variable%s\n", RED, get_line_number(), $1->lexema, RESET);
            free_valor($1);
            exit(ERR_FUNCTION);
        }
        $$ = asd_new($1->lexema, entry->type, NULL, NULL);
        $$->code = gen_var(stack, $1->lexema, &($$->place));
        free_valor($1);
    } |
    TK_LI_INT {
        $$ = asd_new($1->lexema, INT, NULL, NULL);
        int value = atoi($1->lexema);
        $$->code = gen_const(value, &($$->place));
        free_valor($1);
    } |
    TK_LI_FLOAT {
        $$ = asd_new($1->lexema, FLOAT, NULL, NULL);
        // For float, you may need a separate gen_const_float if your ILOC supports it
        // $$->code = gen_const_float(atof($1->lexema), &($$->place));
        free_valor($1);
    } ;

expressao:  
    nivel7 { $$ = $1; };

nivel7:
    nivel6            { $$ = $1; } |
    nivel7 '|' nivel6 {

        if ($1->type != $3->type) {printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);exit(ERR_WRONG_TYPE);}
        $$ = asd_new("|", $1->type, NULL, NULL);
        $$->code = gen_binary_op("|", "or", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1);
        asd_add_child($$, $3);
    } ;

nivel6:
    nivel5            { $$ = $1; } |
    nivel6 '&' nivel5 {

        if ($1->type != $3->type) {printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);exit(ERR_WRONG_TYPE);}
        $$ = asd_new("&", $1->type, NULL, NULL);
        $$->code = gen_binary_op("&", "and", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1);
        asd_add_child($$, $3);} ;

nivel5:
    nivel4                 { $$ = $1; } |
    nivel5 TK_OC_EQ nivel4 { 
        if ($1->type != $3->type) {printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);exit(ERR_WRONG_TYPE);}
        $$ = asd_new("==", $1->type, NULL, NULL);
        $$->code = gen_binary_op("==", "cmp_EQ", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1); asd_add_child($$, $3);
    } |
    nivel5 TK_OC_NE nivel4 { 
        if ($1->type != $3->type) {printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);exit(ERR_WRONG_TYPE);}
        $$ = asd_new("!=", $1->type, NULL, NULL);
        $$->code = gen_binary_op("!=", "cmp_NE", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1); asd_add_child($$, $3);
    } ;

nivel4:
    nivel3                 { $$ = $1; } |
    nivel4 '<' nivel3      { 
        if ($1->type != $3->type) {printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);exit(ERR_WRONG_TYPE);}
        $$ = asd_new("<", $1->type, NULL, NULL);
        $$->code = gen_binary_op("<", "cmp_LT", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1); asd_add_child($$, $3);
    } | 
    nivel4 '>' nivel3      { 
        if ($1->type != $3->type) {printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);exit(ERR_WRONG_TYPE);}
        $$ = asd_new(">", $1->type, NULL, NULL);
        $$->code = gen_binary_op(">", "cmp_GT", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1); asd_add_child($$, $3);
    } | 
    nivel4 TK_OC_LE nivel3 { 
        if ($1->type != $3->type) {printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);exit(ERR_WRONG_TYPE);}
        $$ = asd_new("<=", $1->type, NULL, NULL);
        $$->code = gen_binary_op("<=", "cmp_LE", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1); asd_add_child($$, $3);
    } | 
    nivel4 TK_OC_GE nivel3 { 
        if ($1->type != $3->type) {
            printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);exit(ERR_WRONG_TYPE);}
        $$ = asd_new(">=", $1->type, NULL, NULL);
        $$->code = gen_binary_op(">=", "cmp_GE", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1); asd_add_child($$, $3);
    } ;

nivel3:
    nivel2            { $$ = $1; } |
    nivel3 '+' nivel2 { 
        if ($1->type != $3->type) {
            printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", 
                RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);
            exit(ERR_WRONG_TYPE);
        }
        $$ = asd_new("+", $1->type, NULL, NULL);
        $$->code = gen_binary_op("+", "add", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1);
        asd_add_child($$, $3);
    } |
    nivel3 '-' nivel2 { 
        if ($1->type != $3->type) {
            printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", 
                RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);
            exit(ERR_WRONG_TYPE);
        }
        $$ = asd_new("-", $1->type, NULL, NULL);
        $$->code = gen_binary_op("-", "sub", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1);
        asd_add_child($$, $3);
    } ;

nivel2:
    nivel1            { $$ = $1; } |
    nivel2 '*' nivel1 { 
        if ($1->type != $3->type) {printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);exit(ERR_WRONG_TYPE);}
        $$ = asd_new("*", $1->type, NULL, NULL);
        $$->code = gen_binary_op("*", "mult", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1); asd_add_child($$, $3);
    } |
    nivel2 '/' nivel1 { 
        if ($1->type != $3->type) {printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);exit(ERR_WRONG_TYPE);}
        $$ = asd_new("/", $1->type, NULL, NULL);
        $$->code = gen_binary_op("/", "div", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1); asd_add_child($$, $3);
    } | 
    nivel2 '%' nivel1 { 
        if ($1->type != $3->type) {printf("%sERR_WRONG_TYPE: Line: %d\nType <%s> does not match <%s>%s\n", RED, get_line_number(), dcd_type($1->type), dcd_type($3->type), RESET);exit(ERR_WRONG_TYPE);}
        $$ = asd_new("%", $1->type, NULL, NULL);
        $$->code = gen_binary_op("%", "mod", $1->code, $1->place, $3->code, $3->place, &($$->place));
        asd_add_child($$, $1); asd_add_child($$, $3);
    } ;

nivel1:
    nivel0     { $$ = $1; } |
    '+' nivel1 { 
        $$ = asd_new("+", $2->type, NULL, NULL);
        $$->code = NULL;
        asd_add_child($$, $2);
    } |
    '-' nivel1 { 
        $$ = asd_new("-", $2->type, NULL, NULL);
        $$->code = NULL;
        asd_add_child($$, $2);
    } |
    '!' nivel1 { 
        $$ = asd_new("!", $2->type, NULL, NULL);
        $$->code = NULL;
        asd_add_child($$, $2);
    } ;

nivel0:
    termo               { $$ = $1; } |
    chamada_funcao      { $$ = $1; } | 
    '(' expressao ')'   { $$ = $2; } ;

push: {   
    table_t *table = new_table();
    push_table(&stack, table);
    $$ = NULL;
};
pop: {
    pop_table(&stack);
    $$ = NULL;
};

%%

    void yyerror(char const *mensagem) {
        printf("[Error] - line %d: %s\n", get_line_number(), mensagem);
    }
