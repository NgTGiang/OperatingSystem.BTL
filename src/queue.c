#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        if (q == NULL || proc == NULL) return;
        
        if (q->size < MAX_QUEUE_SIZE) {
                q->proc[q->size] = proc;
                q->size++;
        }
}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (q == NULL || q->size == 0) return NULL;
        
        // Find the highest priority process (lowest prio value)
        int highest_prio_idx = 0;
        for (int i = 1; i < q->size; i++) {
                if (q->proc[i]->priority < q->proc[highest_prio_idx]->priority) {
                        highest_prio_idx = i;
                }
        }
        
        // Get the process with highest priority
        struct pcb_t *proc = q->proc[highest_prio_idx];
        
        // Remove the process from the queue
        for (int i = highest_prio_idx; i < q->size - 1; i++) {
                q->proc[i] = q->proc[i + 1];
        }
        q->size--;
        
        return proc;
}

