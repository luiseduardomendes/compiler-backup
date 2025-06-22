#include <stdio.h>
#include "asd.h"
extern int yyparse(void);
extern int yylex_destroy(void);
asd_tree_t *arvore = NULL;
int main (int argc, char **argv)
{
  (void)argc;
  (void)argv;
  int ret = yyparse();
  if (arvore->children[0]->code != NULL){
    print_iloc_code(stdout, arvore->code);
  }  
  asd_free(arvore);
  yylex_destroy();
  return ret;
}
