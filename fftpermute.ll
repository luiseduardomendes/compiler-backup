declare y0 as int,
declare y1 as int,
declare y2 as int,
declare y3 as int,

main returns int is [
  // === Input Counters ===
  declare x0 as int with 1
  declare x1 as int with 1
  declare x2 as int with 1
  declare x3 as int with 1

  y0 is 0
  y1 is 0
  y2 is 0
  y3 is 0

  // === Loop x0 ===
  while (x0 <= 1) [

    // === Loop x1 ===
    x1 is 0
    while (x1 <= 2) [

      // === Loop x2 ===
      x2 is 0
      while (x2 <= 2) [

        // === Loop x3 ===
        x3 is 0
        while (x3 <= 2) [

          // === FFT Computation Inline ===
          declare a0 as int with 0
          declare a1 as int with 0
          declare b0 as int with 0
          declare b1 as int with 0

          // Butterfly stage
          a0 is x0 + x2
          a1 is x0 - x2
          b0 is x1 + x3
          b1 is x1 - x3

          // Combine stage
          y0 is a0 + b0
          y2 is a0 - b0
          y1 is a1 + b1
          y3 is a1 - b1

          // === Output result (simulate print) ===
          // Example format:
          // Input: (x0, x1, x2, x3)  -> Output: (y0, y1, y2, y3)
          // Replace with actual output in your system

          // print "Input:", x0, x1, x2, x3
          // print "Output:", y0, y1, y2, y3

          // === Increment x3 ===
          x3 is x3 + 1
        ]
        // === Increment x2 ===
        x2 is x2 + 1
      ]
      // === Increment x1 ===
      x1 is x1 + 1
    ]
    // === Increment x0 ===
    x0 is x0 + 1
  ]

  // === End ===
];
