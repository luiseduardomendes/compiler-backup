declare y0 as int with 0
declare y1 as int with 0
declare y2 as int with 0
declare y3 as int with 0

fft returns int is [
  // === Input Values ===
  declare x0 as int with 5
  declare x1 as int with 7
  declare x2 as int with 3
  declare x3 as int with 1

  // === Temporary Variables ===
  declare a0 as int with 0
  declare a1 as int with 0
  declare b0 as int with 0
  declare b1 as int with 0

  // === Stage 1: Butterfly Computations ===
  a0 is x0 + x2    // Even indices sum
  a1 is x0 - x2    // Even indices difference
  b0 is x1 + x3    // Odd indices sum
  b1 is x1 - x3    // Odd indices difference

  // === Stage 2: Combine with Twiddle Factors ===
  y0 is a0 + b0    // FFT output bin 0
  y2 is a0 - b0    // FFT output bin 2
  y1 is a1 + b1    // FFT output bin 1 (approx real part)
  y3 is a1 - b1    // FFT output bin 3 (approx real part)

  // === Return Status ===
  return 0
];
