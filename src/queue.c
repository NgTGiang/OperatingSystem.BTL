#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
        if (q == NULL)
                return 1;
        return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: put a new process to queue [q] */
        if (q == NULL || proc == NULL)
                return;
        if (q->size >= MAX_QUEUE_SIZE)
                return;
        q->proc[q->size] = proc;
        q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (empty(q))
                return NULL;

        int highest_prio_index = 0;
        for (int i = 1; i < q->size; i++)
        {
                if (q->proc[i]->prio < q->proc[highest_prio_index]->prio)
                {
                        highest_prio_index = i;
                }
        }

        struct pcb_t *highest_prio_proc = q->proc[highest_prio_index];

        /* Remove the highest priority proc from q */
        for (int i = highest_prio_index; i < q->size - 1; i++)
        {
                q->proc[i] = q->proc[i + 1];
        }
        q->size--;

        return highest_prio_proc;
}
