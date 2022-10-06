// Number of times to repeat benchmark.
// Note: it's important to use the raw number instead of 1eX notation.
//       that becomes a float, with extra instructions.
#define ITERS 10000000

// A step to separate the content of the benchmark from the loop itself.
// Otherwise, the loop operations could add their own hazards.
#define RAW_BUFFER() asm("nop"); asm("nop"); asm("nop")

// If this mb runs for X iterations, we expect (ignoring overhead):
// - 3X 2 cycle stalls.
//   - 1X from the actual forced 2-cycle we perform.
//   - 1X from the RAW present in the 2-cycle while-loop. (the "lt" check expands to two instrs)
//   - 1X from the RAW present in the 1-cycle while-loop.
// - 1X 1 cycle stalls.
//   - only from the forced 1-cycle stall.
int main(void) {
  int a = 0;
  int b = 0;

  int c = 0;
  int d = 0;

  int X = ITERS;

  // This follows the logic presented in the 1 cycle and 2cycle files, combining them for submission.
  
  // We use a while instead of a for, because otherwise the compiler is hard to work it.
  // It tries to optimize the placement of the increment...which messes with us.
  
  // Force a two-cycle stall on each iteration.
  // This is caused by an immediate data access.
  while (a < X) {
    a += 1;
    // In the for loop case, GCC was smart enough to add the loop-increment here,
    // Which turned this into a 1-cycle stall.
    b += a;

    RAW_BUFFER();
  }

  // Force a 1-cycle stall on each iteration.
  // This is caused by a data access separated by one instruction.
  while (c < X) {
    c += 1;
    asm("nop");
    d += c;

    RAW_BUFFER();
  }

  return a + b + c + d;
}