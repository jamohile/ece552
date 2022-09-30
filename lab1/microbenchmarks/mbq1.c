// Number of times to repeat benchmark.
#define ITERS 10000000

// A step to separate the content of the benchmark from the loop itself.
// Otherwise, the loop operations could add their own hazards.
#define BUFFER_LOOP() asm("nop"); asm("nop"); asm("nop")

int main(void) {
  int a = 0;
  int b = 0;

  int c = 0;
  int d = 0;

  // This follows the logic presented in the 1 cycle and 2cycle files, combining them for submission.
  
  // We use a while instead of a for, because otherwise the compiler is hard to work it.
  // It tries to optimize the placement of the increment...which messes with us.
  
  // Force a two-cycle stall on each iteration.
  // This is caused by an immediate data access.
  while (a < ITERS) {
    a += 1;
    // In the for loop case, GCC was smart enough to add the loop-increment here,
    // Which turned this into a 1-cycle stall.
    b += a;

    BUFFER_LOOP();
  }

  // Force a 1-cycle stall on each iteration.
  // This is caused by a data access separated by one instruction.
  while (c < ITERS) {
    c += 1;
    asm("nop");
    d += c;

    BUFFER_LOOP();
  }

  return a + b + c + d;
}