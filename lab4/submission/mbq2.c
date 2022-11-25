// Microbenchmark for stride prefetcher.

// Uncomment to select a benchmark.
// #define BM_UPDATED_STRIDES
#define BM_DISABLED_PREDICTION

#define ITERS 0
#define BLOCK_SIZE_BYTES 64

// This benchmark seeks to validate correct detection and updating across various multi-block level strides.
#ifdef BM_UPDATED_STRIDES
  char data[1000][BLOCK_SIZE_BYTES];

  // Repeating some number of times, simply iterate across multiple different stride lengths.
  // We expect each new length to trigger a miss, since it would have been incorrectly prefetched.
  // Plus, one additional miss to account for creation of the RPT entry.
  // After the first iteration, assuming we don't overflow the cache, we expect no more misses.
  // Given the cache has 256 blocks of storage, we should be OK.
  //
  // Testcases:
  // 1. ITERS = 0, prefetch disabled: measure overhead: ~357 reads, ~12 misses.
  // 2. ITERS = 1, prefetch disabled: expect - overhead: (3 stride lengths) * (5 accesses) = 15 reads, 15 misses.
  // 3. ITERS = 0, prefetch enabled: measure overhead: ~357 reads, ~11 misses, ~59 prefetch accesses, ~3 prefetch misses
  // 4. ITERS = 1, prefetch enabled: expect - overhead: ~15 reads, 4 misses, 15 prefetch accesses.
  int main(void) {
    int sum = 0;

    int i = 0;
    for (i = 0; i < ITERS; i++) {
      int access_index = 0;

      // Iterate through a few different strides.
      int stride;
      for (stride = 4; stride <= 12; stride += 4) {
        // Perform enough accesses at each stride for us to reach steady state.
        int accesses_at_stride;
        for (accesses_at_stride = 0; accesses_at_stride < 5; accesses_at_stride++) {
          // Note: can find the PC of this by compiling, and looking for the "lb" instruction within main.
          // This makes it easier to filter through the RPT behaviour.
          sum ^= data[access_index][0];
          access_index += stride;
        }
      }
    }

    return sum;
  }
#endif

// Here, we want to validate that after 2 failed stride predictions, we stop predicting altogether.
// The baselines will be the same as above.
// But, we expect the prediction accesses to cap at overhead + 1.
// The initial RPT creation will not trigger a prefetch, but the next will (as we go to transient)
// Then, we'll fall back to no_pred.
#ifdef BM_DISABLED_PREDICTION
  char data[1000][BLOCK_SIZE_BYTES];

  int main(void) {
    int sum;

    int i;
    for (i = 0; i < ITERS; i++) {
      // We achieve this behaviour by just using a different stride each time.
      int access_index;
      for (access_index = 1; access_index < 100; access_index *= 2) {
        sum ^= data[access_index][0];
      }
    }

    return sum;
  }
#endif