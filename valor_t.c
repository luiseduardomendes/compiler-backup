#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "valor_t.h"
#include "parser.tab.h"

extern int get_line_number();

void free_valor(valor_t *val) {
    if (val) {
      if (val->lexema){
         free(val->lexema);
      }
        free(val);
    }
}

void set_valor_lexico(int token, char* value){
    valor_t *aux = NULL;
    aux = malloc(sizeof(valor_t));
 
    // Checks malloc error
    if(aux == NULL){
       fprintf(stderr, "Erro de alocacao de memoria para valor_t\n");
       exit(1);
    }
 
    aux->line_number = get_line_number();
    aux->token_type = token;
    aux->lexema = strdup(value);

    if(aux->lexema == NULL){
      fprintf(stderr, "Erro de alocacao de memoria para lexema\n");
      free(aux);
      exit(1);
    }
 
    yylval.valor_lexico = aux;
}