#include "predictor.h"

#include <cmath>

/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////

// The total space allocated to the 2bitsat prediction table.
#define BITS_2BITSAT (8192)
#define ENTRIES_2BITSAT (BITS_2BITSAT / 2)

// Set as log2 of number of entries,
// due to CPP limits, can't be auto-generated.
#define BITS_KEY_2BITSAT (12)

struct keyed_pc_2bitsat {
  unsigned int key : BITS_KEY_2BITSAT;
};

enum Counter2BitSat {
  STRONG_NOT_TAKEN = 0,
  WEAK_NOT_TAKEN = 1,
  WEAK_TAKEN = 2,
  STRONG_TAKEN = 3
};

void UpdateCounter2BitSat(Counter2BitSat* counter, bool correct) {
  // If we are correct, then we only have to update if not already saturated.
  // Otherwise, we must 'desaturate' or switch prediction.
  // TODO: a more clever encoding may make this less verbose, but I'm sensing the verbosity of this is a tradeoff with the get.
  if (correct) {
    switch (*counter) {
      case WEAK_NOT_TAKEN:    *counter = STRONG_NOT_TAKEN;
        break;
      case WEAK_TAKEN:        *counter = STRONG_TAKEN;
        break;
      default: break;
    }
  } else {
    switch (*counter) {
      case STRONG_NOT_TAKEN:  *counter = WEAK_NOT_TAKEN;
      case WEAK_NOT_TAKEN:    *counter = WEAK_TAKEN;
      case WEAK_TAKEN:        *counter = WEAK_NOT_TAKEN;
      case STRONG_TAKEN:      *counter = WEAK_TAKEN;
    }
  }
}

// TODO: confirm doesn't have to be a bitfield for sim.

struct {
  struct {
    Counter2BitSat status;
  } entries[ENTRIES_2BITSAT];
} prediction_table_2bitsat;

void InitPredictor_2bitsat() {
  // Initialize all prediction table entries to same value.
  for (int i = 0; i < ENTRIES_2BITSAT; i++) {
    prediction_table_2bitsat.entries[i].status = WEAK_NOT_TAKEN;
  }
}

bool GetPrediction_2bitsat(UINT32 PC) {
  auto pc = (struct keyed_pc_2bitsat*) &PC;
 
  if (prediction_table_2bitsat.entries[pc->key].status >= WEAK_TAKEN) {
    return TAKEN;
  }
  return NOT_TAKEN;
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  auto pc = (struct keyed_pc_2bitsat*) &PC;
  UpdateCounter2BitSat(&prediction_table_2bitsat.entries[pc->key].status, resolveDir == predDir);
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

struct {
  struct {
    unsigned int history : BITS_HISTORY_BHT_2LEVEL;
  } entries[NUM_BHT_2LEVEL];
} bhts_2level = {0};

struct {
  struct {
    struct {
      Counter2BitSat status;
    } patterns [NUM_PATTERNS_2LEVEL];
  } entries[NUM_PHT_2LEVEL];
} phts_2level;

void InitPredictor_2level() {
  // Initialize all pattern predictors.
  for (int i = 0; i < NUM_PHT_2LEVEL; i++) {
    for (int j = 0; j < NUM_PATTERNS_2LEVEL; j++) {
      phts_2level.entries[i].patterns[j].status = WEAK_NOT_TAKEN;
    }
  }
}

bool GetPrediction_2level(UINT32 PC) {
  auto pc = (struct keyed_pc_2level*) &PC;

  auto history = bhts_2level.entries[pc->bht].history;
  auto status = phts_2level.entries[pc->pht].patterns[history].status;

  if (status >= WEAK_TAKEN) {
    return TAKEN;
  }

  return NOT_TAKEN;
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  auto pc = (struct keyed_pc_2level*) &PC;

  auto history = bhts_2level.entries[pc->bht].history;

  // Update pattern counter.
  UpdateCounter2BitSat(&phts_2level.entries[pc->pht].patterns[history].status, resolveDir == predDir);

  // Update history.
  bhts_2level.entries[pc->bht].history = (history << 1) | resolveDir;
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

