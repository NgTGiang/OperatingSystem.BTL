#ifndef SCHED_H
#define SCHED_H

#include "common.h"

#ifndef MLQ_SCHED
#define MLQ_SCHED
#endif

#define MAX_PRIO 140

// Checks if all relevant queues (ready and run queues) are empty.
int queue_empty(void);

// Initializes the scheduler’s data structures.
void init_scheduler(void);

// Cleans up scheduler resources (not implemented in provided code).
void finish_scheduler(void);

/* Get the next process from ready queue */
struct pcb_t * get_proc(void);

/* Put a process back to run queue */
void put_proc(struct pcb_t * proc);

/* Add a new process to ready queue */
void add_proc(struct pcb_t * proc);

#endif


