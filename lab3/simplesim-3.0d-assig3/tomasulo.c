
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"
#include "decode.def"

#include "instr.h"

/* PARAMETERS OF THE TOMASULO'S ALGORITHM */

#define INSTR_QUEUE_SIZE 16

#define RESERV_INT_SIZE 5
#define RESERV_FP_SIZE 3
#define FU_INT_SIZE 3
#define FU_FP_SIZE 1

#define FU_INT_LATENCY 5
#define FU_FP_LATENCY 7

/* IDENTIFYING INSTRUCTIONS */

// unconditional branch, jump or call
#define IS_UNCOND_CTRL(op) (MD_OP_FLAGS(op) & F_CALL || \
                            MD_OP_FLAGS(op) & F_UNCOND)

// conditional branch instruction
#define IS_COND_CTRL(op) (MD_OP_FLAGS(op) & F_COND)

// floating-point computation
#define IS_FCOMP(op) (MD_OP_FLAGS(op) & F_FCOMP)

// integer computation
#define IS_ICOMP(op) (MD_OP_FLAGS(op) & F_ICOMP)

// load instruction
#define IS_LOAD(op) (MD_OP_FLAGS(op) & F_LOAD)

// store instruction
#define IS_STORE(op) (MD_OP_FLAGS(op) & F_STORE)

// trap instruction
#define IS_TRAP(op) (MD_OP_FLAGS(op) & F_TRAP)

#define USES_INT_FU(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_STORE(op))
#define USES_FP_FU(op) (IS_FCOMP(op))

#define WRITES_CDB(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_FCOMP(op))

/* FOR DEBUGGING */

// prints info about an instruction
#define PRINT_INST(out, instr, str, cycle)    \
  myfprintf(out, "%d: %s", cycle, str);       \
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n", instr->index);

#define PRINT_REG(out, reg, str, instr)       \
  myfprintf(out, "reg#%d %s ", reg, str);     \
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n", instr->index);

/* VARIABLES */

// instruction queue for tomasulo
static instruction_t* instr_queue[INSTR_QUEUE_SIZE];
static int instr_queue_size = 0;

// reservation stations (each reservation station entry contains a pointer to an instruction)
static instruction_t *reservINT[RESERV_INT_SIZE];
static instruction_t *reservFP[RESERV_FP_SIZE];

// functional units
static instruction_t *fuINT[FU_INT_SIZE];
static instruction_t *fuFP[FU_FP_SIZE];

// common data bus
static instruction_t *commonDataBus = NULL;

// The map table keeps track of which instruction produces the value for each register
static instruction_t *map_table[MD_TOTAL_REGS];

// the index of the last instruction fetched
static int fetch_index = 0;

// The current trace being fetched from, 
// and the current index of the instruction to fetch within that trace.
// Note, the index here is based on the trace table *array*, not the instruction's index property.
instruction_trace_t* current_trace = NULL;
static int current_trace_table_index = 0;

/* FUNCTIONAL UNITS */
struct functional_unit_t {
  // The instruction being processed.
  // If null, then the FU is available.
  instruction_t* instr;
};

struct functional_unit_t int_func_units[FU_INT_SIZE];
struct functional_unit_t fp_func_units[FU_FP_SIZE];

/* RESERVATION STATIONS */
struct reservation_station_t {
  // The instruction being held in this station.
  // If null, then the station is currently empty (i.e, not busy).
  instruction_t* instr;
};

struct reservation_station_t int_reserv_stations[RESERV_INT_SIZE];
struct reservation_station_t fp_reserv_stations[RESERV_FP_SIZE];

/*
 * Description:
 * 	Checks if simulation is done by finishing the very last instruction
 *      Remember that simulation is done only if the entire pipeline is empty
 * Inputs:
 * 	sim_insn: the total number of instructions simulated
 * Returns:
 * 	True: if simulation is finished
 */
static bool is_simulation_done(counter_t sim_insn)
{
  if (fetch_index > sim_insn)
  {
    return true;
  }
  int i;
  for (i = 0; i < INSTR_QUEUE_SIZE; i++)
  {
    if (instr_queue[i] != NULL)
    {
      return false;
    }
  }
  for (i = 0; i < RESERV_INT_SIZE; i++)
  {
    if (reservINT[i] != NULL)
    {
      return false;
    }
  }

  for (i = 0; i < RESERV_FP_SIZE; i++)
  {
    if (reservFP[i] != NULL)
    {
      return false;
    }
  }
  for (i = 0; i < FU_INT_SIZE; i++)
  {
    if (fuINT[i] != NULL)
    {
      return false;
    }
  }

  for (i = 0; i < FU_FP_SIZE; i++)
  {
    if (fuFP[i] != NULL)
    {
      return false;
    };
  }

  /* ECE552: YOUR CODE GOES HERE */

  return true; // ECE552: you can change this as needed; we've added this so the code provided to you compiles
}

/*
 * Description:
 * 	Retires the instruction from writing to the Common Data Bus
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void CDB_To_retire(int current_cycle)
{

  /* ECE552: YOUR CODE GOES HERE */
}

/*
 * Description:
 * 	Moves an instruction from the execution stage to common data bus (if possible)
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void execute_To_CDB(int current_cycle)
{

  /* ECE552: YOUR CODE GOES HERE */
}

/*
 * Description:
 * 	Moves instruction(s) from the issue to the execute stage (if possible). We prioritize old instructions
 *      (in program order) over new ones, if they both contend for the same functional unit.
 *      All RAW dependences need to have been resolved with stalls before an instruction enters execute.
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void issue_To_execute(int current_cycle)
{

  /* ECE552: YOUR CODE GOES HERE */
}

/*
 * Description:
 * 	Moves instruction(s) from the dispatch stage to the issue stage
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void dispatch_To_issue(int current_cycle)
{

  /* ECE552: YOUR CODE GOES HERE */
}

// Move the current trace forward by one index.
// This handles simultaneously advancing the array index, and the linked list.
void advance_trace() {
    current_trace_table_index += 1;
    if (current_trace_table_index >= current_trace->size) {
      current_trace = current_trace->next;
      current_trace_table_index = 0;
    }
}

/*
 * Description:
 * 	Grabs an instruction from the instruction trace (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	None
 */
void fetch(instruction_trace_t *trace)
{
  // We cannot fetch a new instruction if there is no room.
  if (instr_queue_size >= INSTR_QUEUE_SIZE) {
    return;
  }

  // If there is nothing more to fetch, don't.
  if (current_trace == NULL) {
    return;
  }

  // Now, we assume that the current index is a valid one: can be read.
  // However, it may be a trap. If so, advance until we find a non-trap.
  while (IS_TRAP(current_trace->table[fetch_index].op)) {
    advance_trace();
  }

  // It's possible that after advancing, we have no more valid instructions to read.
  if (current_trace == NULL) {
    return;
  }

  // Otherwise, we should have valid data.
  instruction_t* instr = &current_trace->table[current_trace_table_index];
  fetch_index = instr->index;
  instr_queue[instr_queue_size] = instr;
  instr_queue_size++;

  advance_trace();
}

// Remove the first instruction from the queue, and shift all others forward.
void remove_first_instr() {
  for (int i = 0; i < INSTR_QUEUE_SIZE - 1; i++) {
    instr_queue[i] = instr_queue[i + 1];
  }

  // The last element will become null, since nothing to fill it.
  instr_queue[INSTR_QUEUE_SIZE - 1] = NULL;
  instr_queue_size--;
}

/*
 * Description:
 * 	Calls fetch and dispatches an instruction at the same cycle (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void fetch_To_dispatch(instruction_trace_t *trace, int current_cycle)
{

  fetch(trace);

  // We may be able to dispatch the head of the IFQ, if its RS is free.
  instruction_t* instr = instr_queue[0];
  if (instr == NULL) {
    return;
  }

  // Branches get handled as a cycle, but are not dispatched.
  if (IS_COND_CTRL(instr->op) || IS_UNCOND_CTRL(instr->op)) {
    remove_first_instr();
    return;
  } 

  if (IS_FCOMP(instr->op)) {
    // Dispatch to the first free reservation station, if present.
    // Otherwise, no dispatch possible this cycle.
    for (int i = 0; i < RESERV_FP_SIZE; i++) {
      if (fp_reserv_stations[i].instr == NULL) {
        fp_reserv_stations[i].instr = instr;
        // TODO: handle renaming.
        remove_first_instr();
        return;
      }
    }
    return;
  }

  if (IS_ICOMP(instr->op) || IS_LOAD(instr->op) || IS_STORE(instr->op)) {
    for (int i = 0; i < RESERV_FP_SIZE; i++) {
      if (int_reserv_stations[i].instr == NULL) {
        int_reserv_stations[i].instr = instr;
        // TODO: handle renaming.
        remove_first_instr();
        return;
      }
    }
    return;
  }

  // If we reach here, it's an error.
}

/*
 * Description:
 * 	Performs a cycle-by-cycle simulation of the 4-stage pipeline
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	The total number of cycles it takes to execute the instructions.
 * Extra Notes:
 * 	sim_num_insn: the number of instructions in the trace
 */
counter_t runTomasulo(instruction_trace_t *trace)
{
  // initialize instruction queue
  int i;
  for (i = 0; i < INSTR_QUEUE_SIZE; i++)
  {
    instr_queue[i] = NULL;
  }

  // initialize reservation stations
  for (i = 0; i < RESERV_INT_SIZE; i++)
  {
    reservINT[i] = NULL;
  }

  for (i = 0; i < RESERV_FP_SIZE; i++)
  {
    reservFP[i] = NULL;
  }

  // initialize functional units
  for (i = 0; i < FU_INT_SIZE; i++)
  {
    fuINT[i] = NULL;
  }

  for (i = 0; i < FU_FP_SIZE; i++)
  {
    fuFP[i] = NULL;
  }

  // initialize map_table to no producers
  int reg;
  for (reg = 0; reg < MD_TOTAL_REGS; reg++)
  {
    map_table[reg] = NULL;
  }

  int cycle = 1;
  while (true)
  {

    /* ECE552: YOUR CODE GOES HERE */

    cycle++;

    if (is_simulation_done(sim_num_insn))
      break;
  }

  return cycle;
}
