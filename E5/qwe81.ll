//ERR_WRONG_TYPE
f returns int is [
  declare c as int
  if (c) [
    declare a as int
    a is 2
  ]else[
    declare b as float
    b is 2.0
  ]
];