// Microbenchmark for Next Line Prefetcher
// Compiler: PISA GCC.
// Compiler Flags: -O1
// Config: cache-config/cache-lru-open.cfg

#define ITERS 1

// This is very similar to the next-line prefetcher's benchmark.
// The primary difference is the addition of occasional noise.

// The configuration we use defines a 64 set, 64 byte block, 4 way associative cache.
#define SETS (64)
#define WAYS (4)
#define ENTRIES (SETS * WAYS)
#define BLOCK_SIZE_BYTES (64)

char data[ENTRIES][BLOCK_SIZE_BYTES];

int main() {
  int i = 0;
  int entry = 0;
  int block = 0;

  // We use this just to force GCC to actually load data.
  char sum = 0;

  // Each iteration makes ENTRIES (== 255) read accesses.
  for (i = 0; i < ITERS; i++) {
    // We skip operating on the last entry of data, since it would trigger prefetching the *next* line of memory.
    // Since the array itself is sized to fit perfectly into the cache, this would cause unexpected behaviour.
    for (entry = 0; entry < ENTRIES - 1; entry+=4) {
      // Add occasional noise: this should be ignored.
      if (entry % 20 == 0) {
        sum ^= data[3][0];
      } else {
        sum ^= data[entry][0];
      }
    }
  }

  return sum;
}