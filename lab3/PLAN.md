[x] is_simulation_done

[ ] CDB_to_retire

[ ] execute_To_CDB
- Once execution is complete, and CDB is available, move.

[ ] issue_To_execute
- Once RS is ready and FU available, move to execute.
- Prioritize older instr.

[ ] dispatch_To_issue
- Move from D to S *after* 1 cycle.

[ ] fetch_To_dispatch
- first, fetch
- if correct RS is available for head, move there.
- initialize RS, with copies and tags from map table. 

[ ] fetch
- pull instruction from trace, to fetch queue.

[ ] runTomasulo

Need:
- RS
- FU

RS {
  tagJ
  tagK

  busy
  op
}

FU {
  cycleComplete
}


I1
I3
I4
I5