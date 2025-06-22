declare b as int,
declare d as int,
f returns int is [
  declare a as int
  a is a*b/12
  b is a - 2
  b is a - 3
  declare c as int
  c is b + 1
  d is c + 1
];
