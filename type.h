#ifndef _TYPE_T_
#define _TYPE_T_

enum enum_type_t {
  INT, FLOAT
};  

typedef enum enum_type_t type_t;

enum enum_nature_t {
  N_FUNC, N_VAR
};

typedef enum enum_nature_t nature_t;

char *dcd_type(type_t type) ;

#endif // _TYPE__T_