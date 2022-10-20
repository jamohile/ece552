#include "predictor.h"
#include <strings.h>

#include <cmath>

/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////

// Note: FFS returns the 1-indexed position of the first non-zero bit.
// We use it as a poor-man's log2, since log2 cannot be used as a constexpr.
constexpr int _log2(int N) {
  return ffs(N) - 1;
}

// The total space allocated to the 2bitsat prediction table.
#define BITS_2BITSAT (8192)
// Number of entries present in the 2bitsat table.
// These each track a unique counter.
#define ENTRIES_2BITSAT (BITS_2BITSAT / 2)
#define BITS_KEY_2BITSAT (_log2(ENTRIES_2BITSAT))

class Counter2BitSat {
  enum States {
    STRONG_NOT_TAKEN = 0,
    WEAK_NOT_TAKEN = 1,
    WEAK_TAKEN = 2,
    STRONG_TAKEN = 3
  };

  private:
    States state;
  
  public:
    Counter2BitSat(): state(WEAK_NOT_TAKEN) {}

    void update(bool correct) {
      // If we are correct, then we only have to update if not already saturated.
      // Otherwise, we must 'desaturate' or switch prediction.
      // TODO: a more clever encoding may make this less verbose, but I'm sensing the verbosity of this is a tradeoff with the get.
      if (correct) {
        switch (state) {
          case WEAK_NOT_TAKEN:    state = STRONG_NOT_TAKEN;
            break;
          case WEAK_TAKEN:        state = STRONG_TAKEN;
            break;
          default: break;
        }
      } else {
        switch (state) {
          case STRONG_NOT_TAKEN:  state = WEAK_NOT_TAKEN;
            break;
          case WEAK_NOT_TAKEN:    state = WEAK_TAKEN;
            break;
          case WEAK_TAKEN:        state = WEAK_NOT_TAKEN;
            break;
          case STRONG_TAKEN:      state = WEAK_TAKEN;
            break;
        }
      }
    }

    bool predict() {
      if (state >= WEAK_TAKEN) {
        return TAKEN;
      }
      return NOT_TAKEN;
    }

    void reset() {
      state = WEAK_TAKEN;
    }
};

struct keyed_pc_2bitsat {
  unsigned int key : BITS_KEY_2BITSAT;
};

Counter2BitSat prediction_table_2bitsat[ENTRIES_2BITSAT];

void InitPredictor_2bitsat() {}

bool GetPrediction_2bitsat(UINT32 PC) {
  auto pc = (struct keyed_pc_2bitsat*) &PC;
  return prediction_table_2bitsat[pc->key].predict();
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  auto pc = (struct keyed_pc_2bitsat*) &PC;
  prediction_table_2bitsat[pc->key].update(resolveDir == predDir);
}

/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////

#define NUM_BHT_2LEVEL (512)
#define BITS_HISTORY_BHT_2LEVEL (6)
#define NUM_PHT_2LEVEL (8)


#define NUM_PATTERNS_2LEVEL ((int) pow(2, BITS_HISTORY_BHT_2LEVEL))

// Must be set based on the log2 of BHTs/PHTs
// Cannot do at compile time due to CPP limitations.
#define BITS_KEY_BHT_2LEVEL (9)
#define BITS_KEY_PHT_2LEVEL (3)


struct keyed_pc_2level {
  unsigned int bht : BITS_KEY_BHT_2LEVEL;
  unsigned int pht : BITS_KEY_PHT_2LEVEL;
};

template<int BITS>
class History {
  private:
    unsigned int history : BITS;
    
  public:
    History(): history(0) {};

    auto get() {
      return history;
    }

    auto update(bool result) {
      history <<= 1;
      history |= result;
    }
};

// We have several BHTs, which track history for a bucket of PC addresses.
History<BITS_HISTORY_BHT_2LEVEL> bhts_2level[NUM_BHT_2LEVEL];

// We have several PHTs, each of which contain several pattern-aware counters.
Counter2BitSat phts_2level[NUM_PHT_2LEVEL][NUM_PATTERNS_2LEVEL];

template <int BITS_HISTORY, int BITS_KEY, int BITS_TAG, int BITS_USEFULNESS>
class TageEntry {
  private:
    Counter2BitSat counter;
    unsigned int tag: BITS_TAG;
    unsigned int usefulness: BITS_USEFULNESS;
  
  public:
    TageEntry(): tag(0), usefulness(0) {};

    void allocate(unsigned int new_tag) {
      tag = new_tag;
      counter.reset();

      usefulness = 0;
    }

    bool is_available() {
      return usefulness == 0;
    }

    void increment() {
      if ((usefulness + 1) > usefulness){
        usefulness++;
      }
    }

    void decrement() {
      if ((usefulness - 1) < usefulness){
        usefulness--;
      }
    }
};

void InitPredictor_2level() {}

bool GetPrediction_2level(UINT32 PC) {
  auto pc = (struct keyed_pc_2level*) &PC;
  auto history = &bhts_2level[pc->bht];

  return phts_2level[pc->pht][history->get()].predict();
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  auto pc = (struct keyed_pc_2level*) &PC;
  auto history = &bhts_2level[pc->bht];

  phts_2level[pc->pht][history->get()].update(resolveDir == predDir);
  history->update(resolveDir);
}

/////////////////////////////////////////////////////////////
// openend
/////////////////////////////////////////////////////////////

void InitPredictor_openend() {

}

bool GetPrediction_openend(UINT32 PC) {

  return TAKEN;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

}

