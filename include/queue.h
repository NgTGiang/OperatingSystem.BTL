
#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

#define MAX_QUEUE_SIZE 10

struct queue_t {
	struct pcb_t * proc[MAX_QUEUE_SIZE];
	int size;
};

// Adds a process (pcb_t) to the queue.
void enqueue(struct queue_t * q, struct pcb_t * proc);

// Removes and returns a process from the queue (intended to select the highest-priority process).
struct pcb_t * dequeue(struct queue_t * q);

// Checks if the queue is empty, returning 1 if empty or NULL, 0 otherwise.
int empty(struct queue_t * q);

#endif

