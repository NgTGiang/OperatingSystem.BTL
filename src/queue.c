#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
  if (q == NULL)
    return 1;
  
  return (q->size == 0);
}

// Adds a process (pcb_t) to the queue.
void enqueue(struct queue_t *q, struct pcb_t *proc)
{
  /* TODO: put a new process to queue [q] */

  // Check queue and process, exit enqueue() if queue is NUL or process is NULL or size of queue is not valid.
  if (q == NULL || proc == NULL || q->size > MAX_QUEUE_SIZE)
    return;
  
  q->proc[q->size] = proc;
  q->size++;
}

// Removes and returns a process from the queue (intended to select the highest-priority process).
struct pcb_t *dequeue(struct queue_t *q)
{
  /* TODO: return a pcb whose prioprity is the highest
   * in the queue [q] and remember to remove it from q
   * */

  // Check queue, return NULL if queue is NULL or empty.
  if(q == NULL || q->size == 0)
    return NULL;

  // Get first process from queue.
  struct pcb_t * proc = q->proc[0];

  // Shift remaining process elements.
  for(int i = 0; i < q->size - 1; ++i){
    q->proc[i] = q->proc[i + 1];
  }
  
  q->size--;
  q->proc[q->size] = NULL; // Clear last slot
  return proc;
}
