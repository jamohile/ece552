
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "instr.h"

//prints a single instruction
static void print_tom_instr(instruction_t* instr) {

  md_print_insn(instr->inst, instr->pc, stdout);
  myfprintf(stdout, "\t%d\t%d\t%d\t%d\n", 
	    instr->tom_dispatch_cycle,
	    instr->tom_issue_cycle,
	    instr->tom_execute_cycle,
	    instr->tom_cdb_cycle);
}


//prints all the instructions inside the given trace for pipeline
void print_all_instr(instruction_trace_t* trace, int sim_num_insn) {

  fprintf(stdout, "TOMASULO TABLE\n");

  int printed_count = 0;
  int index = 1;
  while (true) {
 
     if (1) { // if (printed_count > 9999900) {
        print_tom_instr(&trace->table[index]);
     }

     printed_count++;

     if (printed_count == sim_num_insn)
        break;
      
     if (++index == INSTR_TRACE_SIZE) {
        trace = trace->next;
	index = 0;
	if (trace == NULL)
	   break;
     }
   }
}

//inserts the instruction into the trace
void put_instr(instruction_trace_t* trace, instruction_t* instr) {

  while ((trace->size == INSTR_TRACE_SIZE) && (trace->next != NULL)) {
    trace = trace->next;
  }
  
  if (trace->size == INSTR_TRACE_SIZE) {
      
     trace->next = malloc(sizeof(instruction_trace_t));
     assert(trace->next != NULL);
     //printf("trace=%p\n", trace);
     trace = trace->next;
     memset(trace, 0, sizeof(instruction_trace_t));
  }
  trace->table[trace->size++] = *instr;
} 

//gets the instruction at the index, from the trace
instruction_t* get_instr(instruction_trace_t* trace, int index) {

  while (index >= INSTR_TRACE_SIZE) {
     index -= INSTR_TRACE_SIZE;
     trace = trace->next;

     assert(trace != NULL);
  }

  return &trace->table[index];
}

