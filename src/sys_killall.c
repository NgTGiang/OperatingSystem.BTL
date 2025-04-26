/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "libmem.h"
#include "queue.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

/* Forward declaration */
extern int free_pcb_memph(struct pcb_t *caller);

/* Define queue_lock */
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

int __sys_killall(struct pcb_t *caller, struct sc_regs *regs)
{
    char proc_name[100];
    uint32_t data;

    uint32_t memrg = regs->a1;

    /* Get name of the target proc from memory */
    int i = 0;
    data = 0;
    while (data != -1 && i < 99)
    {
        libread(caller, memrg, i, &data);
        proc_name[i] = data;
        if (data == -1)
            proc_name[i] = '\0';
        i++;
    }
    proc_name[99] = '\0'; /* Ensure null termination */

    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    /* Traverse running list to terminate matching processes */
    struct queue_t *running = caller->running_list;

    /* Use queue_lock from scheduler */
    pthread_mutex_lock(&queue_lock);

    /* Check all processes in running queue */
    for (i = 0; i < running->size; i++)
    {
        struct pcb_t *pcb = running->proc[i];
        if (pcb != NULL && strcmp(pcb->path, proc_name) == 0)
        {
            /* Free process resources */
            free_pcb_memph(pcb);
            free(pcb);

            /* Remove from queue by shifting */
            for (int j = i; j < running->size - 1; j++)
            {
                running->proc[j] = running->proc[j + 1];
            }
            running->size--;
            i--; /* Recheck current position */
        }
    }

    pthread_mutex_unlock(&queue_lock);

#ifdef MLQ_SCHED
    /* Also check MLQ ready queues */
    for (int prio = 0; prio < MAX_PRIO; prio++)
    {
        struct queue_t *queue = &caller->mlq_ready_queue[prio];

        pthread_mutex_lock(&queue_lock);

        /* Check all processes in current priority queue */
        for (i = 0; i < queue->size; i++)
        {
            struct pcb_t *pcb = queue->proc[i];
            if (pcb != NULL && strcmp(pcb->path, proc_name) == 0)
            {
                /* Free process resources */
                free_pcb_memph(pcb);
                free(pcb);

                /* Remove from queue by shifting */
                for (int j = i; j < queue->size - 1; j++)
                {
                    queue->proc[j] = queue->proc[j + 1];
                }
                queue->size--;
                i--; /* Recheck current position */
            }
        }

        pthread_mutex_unlock(&queue_lock);
    }
#endif

    return 0;
}
