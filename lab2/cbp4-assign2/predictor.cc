#include "predictor.h"

/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////

// The total space allocated to the 2bitsat prediction table.
#define BITS_2BITSAT (8192)

#define ENTRIES_2BITSAT (BITS_2BITSAT / 2)

// We use the bottom X bits of each PC as our index bits.
// The number of bits is set to equal the number of entries we have available above.
// Constructing the mask is a nice little bit trick, try it on paper.
#define MASK_KEY_2BITSAT (ENTRIES_2BITSAT - 1)

enum Counter2BitSat {
  STRONG_NOT_TAKEN = 0,
  WEAK_NOT_TAKEN = 1,
  WEAK_TAKEN = 2,
  STRONG_TAKEN = 3
};

struct __attribute__((__packed__)) {
  struct {
    Counter2BitSat status : 2;
  } entries[ENTRIES_2BITSAT];
} prediction_table_2bitsat;

void InitPredictor_2bitsat() {
  // Initialize all prediction table entries to same value.
  for (int i = 0; i < ENTRIES_2BITSAT; i++) {
    prediction_table_2bitsat.entries[i].status = WEAK_NOT_TAKEN;
  }
}

bool GetPrediction_2bitsat(UINT32 PC) {
  int key = MASK_KEY_2BITSAT & PC;

  if (prediction_table_2bitsat.entries[key].status >= WEAK_TAKEN) {
    return TAKEN;
  }
  return NOT_TAKEN;
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  int key = MASK_KEY_2BITSAT & PC;

  auto currentStatus = prediction_table_2bitsat.entries[key].status;
  bool correct = resolveDir == predDir;

  // If we are correct, then we only have to update if not already saturated.
  // Otherwise, we must 'desaturate' or switch prediction.
  // TODO: a more clever encoding may make this less verbose, but I'm sensing the verbosity of this is a tradeoff with the get.
  if (correct) {
    switch (currentStatus) {
      case WEAK_NOT_TAKEN:    prediction_table_2bitsat.entries[key].status = STRONG_NOT_TAKEN;
        break;
      case WEAK_TAKEN:        prediction_table_2bitsat.entries[key].status = STRONG_TAKEN;
        break;
      default: break;
    }
  } else {
    switch (currentStatus) {
      case STRONG_NOT_TAKEN:  prediction_table_2bitsat.entries[key].status = WEAK_NOT_TAKEN;
      case WEAK_NOT_TAKEN:    prediction_table_2bitsat.entries[key].status = WEAK_TAKEN;
      case WEAK_TAKEN:        prediction_table_2bitsat.entries[key].status = WEAK_NOT_TAKEN;
      case STRONG_TAKEN:      prediction_table_2bitsat.entries[key].status = WEAK_TAKEN;
    }
  }
}

/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////

#define NUM_BHT_2LEVEL (512)
#define BITS_HISTORY_BHT_2LEVEL (6)
#define NUM_PHT_2LEVEL (8)

// Number of unique patterns to maintain history for, per PHT.
// This must be set as 2 ** BITS_HISTORY_LEVEL
#define PATTERNS_2LEVEL (64)

#define MASK_KEY_BHT_2LEVEL (NUM_BHT_2LEVEL - 1)
#define MASK_KEY_PHT_2LEVEL (NUM_PHT_2LEVEL - 1)

struct __attribute__((__packed__)) {
  struct {
    bool history[BITS_HISTORY_BHT_2LEVEL];
  } entries[NUM_BHT_2LEVEL];
} bhts_2level = {0};

struct __attribute__((__packed__)) {
  struct {
    struct {
      Counter2BitSat status : 2;
    } patterns [PATTERNS_2LEVEL];
  } entries[NUM_PHT_2LEVEL];
} phts_2level;

void InitPredictor_2level() {
  // Initialize all pattern predictors.
  for (int i = 0; i < NUM_PHT_2LEVEL; i++) {
    for (int j = 0; j < PATTERNS_2LEVEL; j++) {
      phts_2level.entries[i].patterns[j].status = WEAK_NOT_TAKEN;
    }
  }
}

bool GetPrediction_2level(UINT32 PC) {
  // TODO: I don't like that this offset is hardcoded.
  int key_bht = (PC >> 3) & MASK_KEY_BHT_2LEVEL;
  int key_pht = PC & MASK_KEY_PHT_2LEVEL;

  // TODO: check if this cast is OK.
  auto history = (int*) bhts_2level.entries[key_bht].history;
  auto status = phts_2level.entries[key_pht].patterns[*history].status;

  if (status >= WEAK_TAKEN) {
    return TAKEN;
  }

  return NOT_TAKEN;
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

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

