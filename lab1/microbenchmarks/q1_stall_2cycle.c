// The goal of this microbenchmark is to validate the presence of 2-cycle stalls.
// We expect these to occur when one instruction uses the output of its immediate predecesor.
// Since this processor is not bypassed/forwarded, it does not matter what the instrs are:
// No matter what, data becomes available during writeback, and must be consumed during decode.

int main(void) {
  int a = 0;
  int b = 0;

  int i;
  // Now, have to be careful here.
  // This is easily optimized out.
  for (i = 0; i < 1E7; i++) {
    // The goal here is just to make a simple write, and a simple read.
    a += 1;
    b = a;
  }
  return 0;
}