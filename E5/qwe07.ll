declare b as int,
f returns int is [
  declare c as int
  c is 123
  while (c == 123) [
    b is 2
    c is b
  ]
  if (c == 2)[
    b is 1
  ]
  else[
    b is 0
  ]
];
