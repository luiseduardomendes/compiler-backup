//ERR_WRONG_TYPE
f returns int is [
  declare c as int
  if (c) [
    declare a as int
    while(a) []
  ]else[
    declare b as float
    if(b) [ b is 2.0 ] else [ b is 2.1 ]
  ]
];