
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

#define RESERV_TOTAL_SIZE (RESERV_INT_SIZE + RESERV_FP_SIZE)
#define FU_TOTAL_SIZE (FU_INT_SIZE + FU_FP_SIZE)

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
static instruction_t *instr_queue[INSTR_QUEUE_SIZE] = { 0 };
static int instr_queue_size = 0;

// common data bus
static instruction_t *commonDataBus = NULL;

// The map table keeps track of which instruction produces the value for each register
static instruction_t *map_table[MD_TOTAL_REGS] = { 0 };

// the index of the last instruction fetched
// Since we start our indexing at 1, this means we haven't fetched anything yet.
static int fetch_index = 0;

/* RESERVATION STATIONS */
typedef struct
{
  // The instruction being held in this station.
  // If null, then the station is currently empty (i.e, not busy).
  instruction_t *instr;
} reservation_station_t;

reservation_station_t int_reserv_stations[RESERV_INT_SIZE] = { 0 };
reservation_station_t fp_reserv_stations[RESERV_FP_SIZE] = { 0 };

/* FUNCTIONAL UNITS */
typedef struct
{  
  // The station who's instruction is currently being executed.
  // Since we dealloc these together, it makes sense to store them together too.
  reservation_station_t* station;
  int latency;
} functional_unit_t;

// Create two banks of functional units, initializing latencies accordingly.
functional_unit_t int_func_units[FU_INT_SIZE] = {[0 ...FU_INT_SIZE-1] = {0, .latency=FU_INT_LATENCY}};
functional_unit_t fp_func_units[FU_FP_SIZE] = {[0 ... FU_FP_SIZE-1] = {0, .latency=FU_FP_LATENCY}};
functional_unit_t* all_func_units[FU_TOTAL_SIZE];

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

  // Make sure all instructions have been read.

  for (int i = 0; i < INSTR_QUEUE_SIZE; i++)
  {
    if (instr_queue[i] != NULL)
    {
      return false;
    }
  }

  // Make sure all reservation stations have been emptied.

  for (int i = 0; i < RESERV_INT_SIZE; i++)
  {
    if (int_reserv_stations[i].instr != NULL)
    {
      return false;
    }
  }

  for (int i = 0; i < RESERV_FP_SIZE; i++)
  {
    if (fp_reserv_stations[i].instr != NULL)
    {
      return false;
    }
  }

  // Make sure all functional units have been emptied.

  for (int i = 0; i < FU_INT_SIZE; i++)
  {
    if (int_func_units[i].station != NULL)
    {
      return false;
    }
  }

  for (int i = 0; i < FU_FP_SIZE; i++)
  {
    if (fp_func_units[i].station != NULL)
    {
      return false;
    };
  }

  // Make sure the CDB has been consumed.
  if (commonDataBus != NULL) {
    return false;
  }

  return true;
}

void notify_reservation_stations(reservation_station_t* stations, int num_stations, instruction_t* completed_instr) {
  // Find all stations who depend on this result, and 'notify them' it is complete.
  for (int i = 0; i < num_stations; i++) {
    reservation_station_t* station = &stations[i];
    
    // If the station is empty, it's obviously not waiting.
    if (station->instr == NULL) {
      continue;
    }

    for (int q = 0; q < 3; q++) {
      if (station->instr->Q[q] == completed_instr) {
        station->instr->Q[q] = NULL;
      }
    }
  }
}

/*
 * Description:
 * 	Retires the instruction from writing to the Common Data Bus
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
// this function releases Q and release map table entry
void CDB_To_retire(int current_cycle)
{
  if (commonDataBus == NULL) {
    return;
  }

  // clear maptable
  for (int i = 0; i < 2; i++)
  {
    int maptable_index = commonDataBus->r_out[i];
    if (map_table[maptable_index] == commonDataBus)
    {
      map_table[maptable_index] = NULL;
    }
  }

  notify_reservation_stations(int_reserv_stations, RESERV_INT_SIZE, commonDataBus);
  notify_reservation_stations(fp_reserv_stations, RESERV_FP_SIZE, commonDataBus);

  commonDataBus = NULL;
}

bool is_done_executing(functional_unit_t* func_unit, int current_cycle) {
  return current_cycle >= (func_unit->station->instr->tom_execute_cycle + func_unit->latency);
}

// Search functional units for an instruction that is ready to broadcast to the CDB.
// If there are multiple, the youngest one wins.
functional_unit_t* get_cdb_candidate(int current_cycle, functional_unit_t* func_units, int num_func_units) {
  functional_unit_t* cdb_candidate = NULL;

  for (int i = 0; i < num_func_units; i++) {
    functional_unit_t* func_unit = &func_units[i];

    // Empty or ongoing FUs cannot broadcast.
    if (func_unit->station == NULL || !is_done_executing(func_unit, current_cycle)) {
      continue;
    }

    // If the instruction is a store, it does not broadcast.
    if (IS_STORE(func_unit->station->instr->op)) {
      continue;
    }

    // The instruction is able to broadcast!
    // Select it only if it is the youngest.
    if (cdb_candidate == NULL || func_unit->station->instr->index < cdb_candidate->station->instr->index) {
      cdb_candidate = func_unit;
    }
  }

  return cdb_candidate;
}

void cleanup_completed_stores(int current_cycle, functional_unit_t* func_units, int num_func_units) {
  for (int i = 0; i < num_func_units; i++) {
    functional_unit_t* func_unit = &func_units[i];

    // Stores are not complete if the FU is empty, or they are still running.
    if (func_unit->station == NULL || !is_done_executing(func_unit, current_cycle)) {
      continue;
    }

    if (!IS_STORE(func_unit->station->instr->op)) {
      continue;
    }

    // Deallocate the reservation station and functional unit.
    func_unit->station->instr = NULL;
    func_unit->station = NULL;
  }
}

/*
 * Description:
 * 	Moves an instruction from the execution stage to common data bus (if possible)
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
// this function only cares about cdb 
void execute_To_CDB(int current_cycle)
{
  // If there is already something on the CDB, we can't broadcast.
  if (commonDataBus != NULL) {
    return;
  }

  // If there is an instruction ready to broadcast, it should.
  functional_unit_t* int_cdb_candidate = get_cdb_candidate(current_cycle, int_func_units, FU_INT_SIZE);
  functional_unit_t* fp_cdb_candidate = get_cdb_candidate(current_cycle, fp_func_units, FU_FP_SIZE);

  // If only one execution path has a broadcastable instruction, it wins.
  // Otherwise, the younger instruction moves forward.
  functional_unit_t* cdb_candidate = NULL;
  if (int_cdb_candidate == NULL || fp_cdb_candidate == NULL) {
    cdb_candidate = int_cdb_candidate == NULL ? fp_cdb_candidate : int_cdb_candidate;
  } else {
    bool int_younger = int_cdb_candidate->station->instr->index < fp_cdb_candidate->station->instr->index;
    cdb_candidate = int_younger ? int_cdb_candidate : fp_cdb_candidate;
  }

  // Now, if we found a broadcastable instruction, broadcast it.
  // Also, deallocate its functional unit and reservation station.
  if (cdb_candidate != NULL) {
    commonDataBus = cdb_candidate->station->instr;
    cdb_candidate->station->instr->tom_cdb_cycle = current_cycle;

    cdb_candidate->station->instr = NULL;
    cdb_candidate->station = NULL;
  }

  // Cleanup any stores, as these do not move to the CDB.
  cleanup_completed_stores(current_cycle, int_func_units, FU_INT_SIZE);
  cleanup_completed_stores(current_cycle, fp_func_units, FU_FP_SIZE);
}

bool has_raw_dependences(instruction_t* instr) {
  for (int q = 0; q < 3; q++) {
    if (instr->Q[q] != NULL) {
      return true;
    }
  }
  return false;
}

// Returns the first free functional unit in an array of functional units, if there is one.
functional_unit_t* get_free_func_unit(functional_unit_t *func_units, int num_func_units)
{
  for (int i = 0; i < num_func_units; i++)
  {
    if (func_units[i].station == NULL)
    {
      return &func_units[i];
    }
  }
  return NULL;
}

// Given a bank of reservation stations, and another bank of functional units, move as many entries as possible from issue to execute.
// That is, assign free instructions to free functional units, favouring older instructions (based on program count)
void move_issue_to_execute_if_ready(int current_cycle, reservation_station_t* stations, int num_stations, functional_unit_t* func_units, int num_func_units) {
  while (get_free_func_unit(func_units, num_func_units) != NULL) {
    reservation_station_t* execution_candidate = NULL;

    for (int i = 0; i < num_stations; i++) {
      reservation_station_t* station = &stations[i];
      
      // We can't execute if the station is empty or still waiting for data.
      if (station->instr == NULL || has_raw_dependences(station->instr)) {
        continue;
      }

      // We can only execute instructions that have not yet executed (duh) and have been in issue for >= 1 cycle.
      if (station->instr->tom_issue_cycle > 0 && station->instr->tom_issue_cycle < current_cycle && station->instr->tom_execute_cycle == 0) {
        // Favour running older instructions first. (old = lower index)
        if (execution_candidate == NULL || station->instr->index < execution_candidate->instr->index) {
          execution_candidate = station;
        }
      }
    }

    // Ultimately, we can schedule the execution candidate for execution.
    // However, if there was no candidate, no point continuing to search, even if FUs still exist.
    if (execution_candidate != NULL) {
      get_free_func_unit(func_units, num_func_units)->station = execution_candidate;
      execution_candidate->instr->tom_execute_cycle = current_cycle;
    } else {
      break;
    }
  }
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
  move_issue_to_execute_if_ready(current_cycle, int_reserv_stations, RESERV_INT_SIZE, int_func_units, FU_INT_SIZE);
  move_issue_to_execute_if_ready(current_cycle, fp_reserv_stations, RESERV_FP_SIZE, fp_func_units, FU_FP_SIZE);
}

// Apply register renaming to an instruction.
// This includes both updating the map-table, and updating the instruction based on this table.
void apply_register_renaming(instruction_t *instr)
{
  // Map in inputs.
  // It's important that we do this before the outputs,
  // otherwise we may end up with circular dependencies.
  for (int i = 0; i < 3; i++)
  {
    int reg = instr->r_in[i];
    if (reg != DNA)
    {
      // Note, if the value is already ready, map_table entry will be null.
      // This is equivalent to there being no value there, which is OK for cycle-sim,
      // since we're not actually computing anything.
      instr->Q[i] = map_table[reg];
    }
  }

  // Remap outputs.
  for (int i = 0; i < 2; i++)
  {
    int reg = instr->r_out[i];
    if (reg != DNA)
    {
      map_table[reg] = instr;
    }
  }
}

// Remove the first instruction from the queue, and shift all others forward.
void remove_first_instr()
{
  for (int i = 0; i < INSTR_QUEUE_SIZE - 1; i++)
  {
    instr_queue[i] = instr_queue[i + 1];
  }

  // The last element will become null, since nothing to fill it.
  instr_queue[INSTR_QUEUE_SIZE - 1] = NULL;
  instr_queue_size--;
}

// Returns the first free station in an array of stations, if there is one.
reservation_station_t *get_free_reserv(reservation_station_t *reserv_station_array, int num_stations)
{
  for (int i = 0; i < num_stations; i++)
  {
    if (reserv_station_array[i].instr == NULL)
    {
      return &reserv_station_array[i];
    }
  }
  return NULL;
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
  // Only the head of the IFQ is allowed to dispatch.
  if (instr_queue_size == 0) {
    return;
  }

  reservation_station_t* assigned_station = NULL;
  if (USES_INT_FU(instr_queue[0]->op)) {
    assigned_station = get_free_reserv(int_reserv_stations, RESERV_INT_SIZE);
  } else {
    assigned_station = get_free_reserv(fp_reserv_stations, RESERV_FP_SIZE);
  }

  if (assigned_station) {
    assigned_station->instr = instr_queue[0];
    assigned_station->instr->tom_issue_cycle = current_cycle;
    remove_first_instr();
    apply_register_renaming(assigned_station->instr);
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
  if (instr_queue_size >= INSTR_QUEUE_SIZE)
  {
    return;
  }

  // We cannot fetch an instruction if they've all been fetched.
  if (fetch_index == sim_num_insn) {
    return;
  }

  do {
    fetch_index++;
  } while (IS_TRAP(get_instr(trace, fetch_index)->op));

  instr_queue[instr_queue_size] = get_instr(trace, fetch_index);
  instr_queue_size++;
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

  // Regardless of if an inst is ready to send to an RS,
  // it begins dispatch as soon as it hits the IFQ.
  for (int i = 0; i < INSTR_QUEUE_SIZE; i++) {
    if (instr_queue[i] != NULL && instr_queue[i]->tom_dispatch_cycle == 0) {
      instr_queue[i]->tom_dispatch_cycle = current_cycle;
    }
  }

  // We may be able to dispatch the head of the IFQ, if its RS is free.
  instruction_t *instr = instr_queue[0];
  if (instr == NULL)
  {
    return;
  }

  // Branches get handled as a cycle, but are not dispatched.
  if (IS_COND_CTRL(instr->op) || IS_UNCOND_CTRL(instr->op))
  {
    remove_first_instr();
    return;
  }
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
  for (int i = 0; i < FU_INT_SIZE; i++) {
    all_func_units[i] = &int_func_units[i];
  }
  for (int i = 0; i < FU_FP_SIZE; i++) {
    all_func_units[i + FU_INT_SIZE] = &fp_func_units[i];
  }

  int cycle = 1;
  while (true)
  {
    execute_To_CDB(cycle);
    // When executing, we are allowed to immediately use an FU freed above.
    // However, we must wait a cycle before using values broadcasted below.
    issue_To_execute(cycle);
    CDB_To_retire(cycle);

    // Dispatch must precede fetch, to make sure we don't immediately dispatch an instruction.
    // When starting issue, we can immediately use reservation stations freed during broadcast.
    dispatch_To_issue(cycle);
    fetch_To_dispatch(trace, cycle);

    cycle++;

    if (is_simulation_done(sim_num_insn))
      break;
  }

  return cycle;
}
