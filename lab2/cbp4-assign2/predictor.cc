#include "predictor.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////
//#define predictortable2bittotalbits 8192;
#define Nottaken 0
#define weaklynottaken 1
#define weaklytaken 2
#define stronglytaken 3
int predictortable2bit[8192/2];
void InitPredictor_2bitsat() {
  for (int i=0; i<4096; i++) {
      predictortable2bit[i] = weaklynottaken;  
  }
}

bool GetPrediction_2bitsat(UINT32 PC) {
  // 3f is 12 1s
  int last12 = PC & (0x0FFF);
  if ( (predictortable2bit[last12] == Nottaken) ||  (predictortable2bit[last12]  == weaklynottaken)) {
    return NOT_TAKEN;
  }
  else {
    return TAKEN;
  }
  
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  int last12 = (PC & 0x0FFF);
  if (resolveDir == true && predictortable2bit[last12] == Nottaken) {
    predictortable2bit[last12] = weaklynottaken;
  } 
  else if (resolveDir == true && predictortable2bit[last12] == weaklynottaken) {
    predictortable2bit[last12] = weaklytaken;
  }  
  else if (resolveDir == true && predictortable2bit[last12] == weaklytaken) {
    predictortable2bit[last12] = stronglytaken;
  }
  else if (resolveDir == true && predictortable2bit[last12] == stronglytaken) {
    predictortable2bit[last12] = stronglytaken;
  }
  else {
    if (resolveDir == false && predictortable2bit[last12] == stronglytaken) {
      predictortable2bit [last12] = weaklytaken;
    }
    else if (resolveDir == false && predictortable2bit[last12] == weaklytaken) {
      predictortable2bit[last12] = weaklynottaken;
    }
    else if (resolveDir == false && predictortable2bit[last12] == weaklynottaken) {
      predictortable2bit[last12] = Nottaken;
    }
    else if (resolveDir == false && predictortable2bit[last12] == Nottaken) {
      predictortable2bit[last12] = Nottaken;   
    }
  }
}

/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////
int private_history_table [512];
int private_predictor_table [8][64];
void InitPredictor_2level() {
  for (int i =0; i< 512; i++) {
    private_history_table[i] = 0;
  }
  for (int i =0; i<8; i++) {
    for (int j=0; j<64; j++) {
      private_predictor_table[i][j]= weaklynottaken;
    }
  }
}

bool GetPrediction_2level(UINT32 PC) {
  int historytableindex = (PC & 0b111111111000 ) >> 3;
  int value_history_table = private_history_table [historytableindex];
  int tableindex = (PC & 0b111);
  if (private_predictor_table[tableindex][value_history_table] == weaklytaken || private_predictor_table[tableindex][value_history_table] == stronglytaken) {
    return TAKEN;
  } 
  else {
    return NOT_TAKEN;
  }
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  int historytableindex = (PC & 0b111111111000 ) >> 3;
  int value_history_table = private_history_table [historytableindex];
  int tableindex = (PC & 0b111);
  if (resolveDir == true && private_predictor_table[tableindex][value_history_table] == Nottaken) {
    private_predictor_table[tableindex][value_history_table]= weaklynottaken;
  } 
  else if (resolveDir == true && private_predictor_table[tableindex][value_history_table] == weaklynottaken) {
    private_predictor_table[tableindex][value_history_table] = weaklytaken;
  }  
  else if (resolveDir == true && private_predictor_table[tableindex][value_history_table]== weaklytaken) {
    private_predictor_table[tableindex][value_history_table]= stronglytaken;
  }
  else if (resolveDir == true && private_predictor_table[tableindex][value_history_table] == stronglytaken) {
    private_predictor_table[tableindex][value_history_table]= stronglytaken;
  }
  else {
    if (resolveDir == false && private_predictor_table[tableindex][value_history_table] == stronglytaken) {
      private_predictor_table [tableindex][value_history_table] = weaklytaken;
    }
    else if (resolveDir == false && private_predictor_table[tableindex][value_history_table] == weaklytaken) {
      private_predictor_table[tableindex][value_history_table]= weaklynottaken;
    }
    else if (resolveDir == false && private_predictor_table[tableindex][value_history_table] == weaklynottaken) {
      private_predictor_table[tableindex][value_history_table]= Nottaken;
    }
    else if (resolveDir == false && private_predictor_table[tableindex][value_history_table] == Nottaken) {
      private_predictor_table[tableindex][value_history_table] = Nottaken;   
    }
  }
  int new_value_history_table = ((value_history_table << 1) + resolveDir) & (0b111111);
  private_history_table[historytableindex] =  new_value_history_table;
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

