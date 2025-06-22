#include "type.h"
#include <stdio.h>

char *dcd_type(type_t type) {
  switch (type) {
    case INT:
      return "int";
    case FLOAT:
      return "float";
    default:
      return "unknown";
  }
}