
#ifndef INSTR_H
#define INSTR_H

#include "machine.h"

//data structure representing each instruction
typedef struct my_instruction
{
  int index; //the unique index value of the instruction 
             //it shows the order the instructions execute in
  md_inst_t inst;  
  int r_out[2]; //output registers
  int r_in[3]; //input registers
  enum md_opcode op; //opcode
  md_addr_t pc; //program counter the instruction executes at

  //the equivalents of Qj, Qk; these are pointers to the instructions producing the results
  // for the input registers of this instruction
  struct my_instruction * Q[3]; 

  //Specify the cycle an instruction **entered** this stage
  int tom_dispatch_cycle;  //dispatch
  int tom_issue_cycle;     //issue
  int tom_execute_cycle;   //execute
  int tom_cdb_cycle;       //writeback via Common Data Bus (CDB)

}instruction_t;

#define INSTR_TRACE_SIZE 16384

typedef struct my_instruction_list
{
  instruction_t table[INSTR_TRACE_SIZE];
  int size;
  struct my_instruction_list* next;
}instruction_trace_t;

//prints all the instructions inside the given trace
extern void print_all_instr(instruction_trace_t* table, int sim_num_insn);

//inserts the instruction into the trace
extern void put_instr(instruction_trace_t* trace, instruction_t* instr);

//gets the instruction at the index, from the trace
extern instruction_t* get_instr(instruction_trace_t* trace, int index);

#endif
