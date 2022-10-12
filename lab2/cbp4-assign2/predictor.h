#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

#include "utils.h"
#include "tracer.h"

/////////////////////////////////////////////////////////////

void InitPredictor_2bitsat();
bool GetPrediction_2bitsat(UINT32 PC);  
void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget);

/////////////////////////////////////////////////////////////

void InitPredictor_2level();
bool GetPrediction_2level(UINT32 PC);  
void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget);

/////////////////////////////////////////////////////////////

void InitPredictor_openend();
bool GetPrediction_openend(UINT32 PC);  
void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget);

/////////////////////////////////////////////////////////////

#endif

