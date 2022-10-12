#include "predictor.h"

#include <cmath>

/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////

// The total space allocated to the 2bitsat prediction table.
#define BITS_2BITSAT (8192)
// Number of entries present in the 2bitsat table.
// These each track a unique counter.
#define ENTRIES_2BITSAT (BITS_2BITSAT / 2)

// Set as log2 of number of entries,
// due to CPP limits, can't be auto-generated.
#define BITS_KEY_2BITSAT (12)

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
          case WEAK_NOT_TAKEN:    state = WEAK_TAKEN;
          case WEAK_TAKEN:        state = WEAK_NOT_TAKEN;
          case STRONG_TAKEN:      state = WEAK_TAKEN;
        }
      }
    }

    bool predict() {
      if (state >= WEAK_TAKEN) {
        return TAKEN;
      }
      return NOT_TAKEN;
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

class History2Level {
  private:
    unsigned int history : BITS_HISTORY_BHT_2LEVEL;
    
  public:
    History2Level(): history(0) {};

    auto get() {
      return history;
    }

    auto update(bool result) {
      history <<= 1;
      history |= result;
    }
};

// We have several BHTs, which track history for a bucket of PC addresses.
History2Level bhts_2level[NUM_BHT_2LEVEL];

// We have several PHTs, each of which contain several pattern-aware counters.
Counter2BitSat phts_2level[NUM_PHT_2LEVEL][NUM_PATTERNS_2LEVEL];

void InitPredictor_2level() {}

bool GetPrediction_2level(UINT32 PC) {
  auto pc = (struct keyed_pc_2level*) &PC;
  auto history = bhts_2level[pc->bht];

  return phts_2level[pc->pht][history.get()].predict();
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  auto pc = (struct keyed_pc_2level*) &PC;
  auto history = bhts_2level[pc->bht];

  phts_2level[pc->pht][history.get()].update(resolveDir == predDir);
  history.update(resolveDir);
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

