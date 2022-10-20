#include "predictor.h"
#include <strings.h>
#include <vector>
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

template <int BITS_TAG, int BITS_USEFULNESS>
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

    bool matches(unsigned int _tag) {
      return tag == _tag;
    }

    bool predict() {
      return counter.predict();
    }

    void update(bool correct) {
      counter.update(correct);
    }
};

template <int BITS_TAG, int BITS_USEFULNESS>
class BaseTageComponent {
  using Entry = TageEntry<BITS_TAG, BITS_USEFULNESS>;

  public:
    virtual Entry* get_entry(unsigned int pc, unsigned int full_history) = 0;
    virtual Entry* get_tagged_entry(unsigned int pc, unsigned int full_history, unsigned int tag) = 0;
};

template <int BITS_HISTORY, int BITS_KEY, int BITS_TAG, int BITS_USEFULNESS>
class TageComponent : public BaseTageComponent<BITS_TAG, BITS_USEFULNESS> {
  using Entry = TageEntry<BITS_TAG, BITS_USEFULNESS>;

  private:
    Entry entries[1 << (BITS_HISTORY + BITS_KEY)];
  
  public:
    Entry* get_entry(unsigned int pc, unsigned int full_history) {
      auto key = pc & ((1 << BITS_KEY) - 1);
      auto history = history & ((1 << BITS_HISTORY) - 1);
      auto index = (key << BITS_HISTORY) | BITS_HISTORY;

      return &entries[index];
    }

    Entry* get_tagged_entry(unsigned int pc, unsigned int full_history) {
      auto entry = get_entry(pc, full_history);
      auto tag = (pc >> BITS_KEY) & ((1 << BITS_TAG) - 1);
      if (entry->matches(tag)) {
        return entry;
      }
      return NULL;
    }
};

template <int BITS_KEY, int BITS_TAG, int BITS_USEFULNESS>
class TagePredictor {
  using Entry = TageEntry<BITS_TAG, BITS_USEFULNESS>;

  private:
    History<32> history;
    std::vector<BaseTageComponent<BITS_TAG, BITS_USEFULNESS>*> components;

  public:
    TagePredictor() {
      components.push_back(new TageComponent<0, BITS_KEY, BITS_TAG, BITS_USEFULNESS>());
      components.push_back(new TageComponent<1, BITS_KEY, BITS_TAG, BITS_USEFULNESS>());
      components.push_back(new TageComponent<2, BITS_KEY, BITS_TAG, BITS_USEFULNESS>());
      components.push_back(new TageComponent<4, BITS_KEY, BITS_TAG, BITS_USEFULNESS>());
      components.push_back(new TageComponent<8, BITS_KEY, BITS_TAG, BITS_USEFULNESS>());
      components.push_back(new TageComponent<16, BITS_KEY, BITS_TAG, BITS_USEFULNESS>());
      components.push_back(new TageComponent<32, BITS_KEY, BITS_TAG, BITS_USEFULNESS>());
    }

    bool predict(unsigned int pc) {
      for (int i = components.size() - 1; i >= 0; i--) {
        Entry* entry = components[i]->get_tagged_entry(pc, history.get());
        if (entry != NULL) {
          return entry->predict();
        }
      }
    }

    void update(unsigned int pc, bool prediction, bool result) {
      if (prediction == result) {
        // Find the provider, and update it.
        for (int i = components.size() - 1; i >= 0; i--) {
          Entry* predictor = components[i]->get_tagged_entry(pc, history.get());
          if (predictor != NULL) {
            predictor->update(true);
            // Find altpred.
            for (int j = i; j >= 0; j--) {
              Entry* altpred = components[j]->get_tagged_entry(pc, history.get()); 
              if (altpred != NULL && altpred->predict() != prediction) {
                predictor->increment();
              }
            }
            break;
          } 
        }
      } else {
        // Find the provider, and weaken it.
        for (int i = components.size() - 1; i >= 0; i--) {
          Entry* entry = components[i]->get_tagged_entry(pc, history.get());
          if (entry != NULL) {
            entry->update(false);
            // Now, try and allocate a new component with more history.
            // If we're not able to allocate something, go ahead and weaken all.
            bool allocated = false;
            for (int j = i; j < components.size(); j++) {
              // TODO: this should be pseudorandom.
              Entry* candidate = components[j]->get_entry(pc, history.get());
              if (candidate->is_available()) {
                auto tag = (pc >> BITS_KEY) & ((1 << BITS_TAG) - 1);
                allocated = true;
                candidate->allocate(tag);
                break;
              }
            }

            if (!allocated) {
              // Weaken all existing entries that could not be allocated against.
              for (int j = i; j < components.size(); j++) {
                Entry* failed_candidate = components[j]->get_entry(pc, history.get());
                failed_candidate->decrement();
              }
            }
            break;
          } 
        }
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

