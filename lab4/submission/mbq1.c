// Microbenchmark for Next Line Prefetcher
// Compiler: PISA GCC.
// Compiler Flags: -O1
// Config: cache-config/cache-lru-nextline.cfg

#define ITERS 1

// Here, we'd like to exercise our prefetcher, to verify a reasonable number of hits/misses.
// Let's say that our cache can hold N blocks.
// Then, if we operate one-time-each on a datastructure spanning N blocks, we should get approximately:
//  - ~N non-prefetch reads of the DL1 cache. These come directly from software.
//  ~ ~N prefetch reads of the DL1 cache, triggered each time there is a SW access above.
//  - ~N DL1 prefetch read misses, since each block of data must be loaded from memory.
//  - ~1 DL1 read misses, since after the first read in the array, the rest will be prefetched.
//    - When the 0th is requested, the 1st is prefetched.
//    - When the 1st is requested, the 2nd is prefetched...etc
//
// If we repeat these accesses M times, we expect:
//  - ~M*N non-prefetch access of the DL1 cache, see above.
//  - ~M*N prefetch accesses of the DL1 cache, see above.
//  - ~N DL1 misses, since the data fits perfectly into the cache, and therefore reaches steady state.
//  - ~1 DL1 read misses, see above, steady state.

// The configuration we use defines a 64 set, 64 byte block, 4 way associative cache.
#define SETS (64)
#define WAYS (4)
#define ENTRIES (SETS * WAYS)
#define BLOCK_SIZE_BYTES (64)

char data[ENTRIES][BLOCK_SIZE_BYTES];

// To properly justify this benchmark and discount overheads, collect the following configurations of data (DL1 statistics)
// 1. 0 iters, no prefetch:  establish baseline overhead
// 2. 0 iters, prefetch:     see how prefetch changes the overhead.
// 3. 1 iter, no prefetch:   validate expected number of non-overhead reads/misses without prefetch (255)
// 4. 1 iter, prefetch:      see how prefetch changes 3. ignoring overhead, expect (255) misses to transfer to prefetch, and (0) for mainline.
// 5. 100 iters, prefetch:   make sure that system reached steady state. hits & accesses scale, but misses do not.

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
    for (entry = 0; entry < ENTRIES - 1; entry++) {
      sum ^= data[entry][0];
    }
  }

  return sum;
}