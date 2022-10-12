#ifndef _TRACER_H_
#define _TRACER_H_

#include "utils.h"

/////////////////////////////////////////
/////////////////////////////////////////


typedef enum {
  OPTYPE_LOAD             =0, 
  OPTYPE_STORE            =1,
  OPTYPE_OP               =2,
  OPTYPE_CALL_DIRECT      =3,
  OPTYPE_RET              =4,
  OPTYPE_BRANCH_UNCOND    =5,
  OPTYPE_BRANCH_COND      =6,
  OPTYPE_INDIRECT_BR_CALL =7,
  OPTYPE_MAX              =8
}OpType;

/////////////////////////////////////////
/////////////////////////////////////////

class CBP_TRACE_RECORD{
  public:
  UINT32   PC;
  OpType   opType;
  bool     branchTaken;
  UINT32   branchTarget;

  CBP_TRACE_RECORD(){
    PC=0;
    opType=OPTYPE_MAX;
    branchTaken=0;
    branchTarget=0;
  }
};


/////////////////////////////////////////
/////////////////////////////////////////

class CBP_TRACER{
 private:
  FILE *traceFile;

  UINT64 numInst;        
  UINT64 numCondBranch;

  UINT64 lastHeartBeat;

 public:
  CBP_TRACER(char *traceFileName);

  bool   GetNextRecord(CBP_TRACE_RECORD *record);  
  UINT64 GetNumInst(){ return numInst; }
  UINT64 GetNumCondBranch(){ return numCondBranch; }

 private:
  void   CheckHeartBeat();
};


/////////////////////////////////////////
/////////////////////////////////////////


#endif // _TRACER_H_

