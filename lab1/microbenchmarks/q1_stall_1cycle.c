// The goal of this microbenchmark is to validate the presence of 1-cycle stalls.
// We expect these to occur when one instruction uses the output of its predecesor, separated by 1 instruction.

// MUST be built using O2 optimization.

int main(void) {
  int a = 0;
  int b = 0;

  // Even though this is very simple, it is *very* finicky.
  // We use a while instead of a for, because otherwise the compiler is hard to work it.
  // It tries to optimize the placement of the increment...which messes with us.
  while (a < 100000) {
    // We set a, this is our 'write'.
    a += 1;

    // For a 1-cycle stall, we need these separated by a random instruction.
    // We could actually use a real one...but GCC is making that annoying.
    asm("nop");

    // Make b based on a, this is our read.
    // This is actually why we use the 'while'.
    // We're trying to cause a two-cycle stall,
    // but in the for loop case: gcc is smart, and places the increment between these two instrs.
    b += a;

    // Since this loop is so short, we'd probably get some level of RAW just through the loop op.
    // So, we add a bunch of noops to separate the top part.
    // Luckily, GCC is willing to perform the loop checks *after* these nops...unlike the for loop.
    asm("nop");
    asm("nop");
    asm("nop");
  }

  return a + b;
}