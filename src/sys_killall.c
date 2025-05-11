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

 /* Define queue_lock */
 pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
 
 int __sys_killall(struct pcb_t *caller, struct sc_regs *regs)
 {
     printf("sys_killall.c sys_killall: killing all processes with same name\n");
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
         printf("sys_killall.c sys_killall: checking process in running queue %d\n", i);
         struct pcb_t *pcb = running->proc[i];
         if (strstr(pcb->path, proc_name) != 0)
         {
             /* Free process resources */
             printf("Killing process \"%s\"... with path %s\n", proc_name, pcb->path);
             pcb->pc = pcb->code->size; // Set PC to end of code segment
         }
     }
 
     pthread_mutex_unlock(&queue_lock);
 
 #ifdef MLQ_SCHED
     /* Also check MLQ ready queues */
     printf("sys_killall.c sys_killall: checking processes in MLQ\n");
     for (int prio = 0; prio < MAX_PRIO; prio++)
     {
         
         struct queue_t *queue = &caller->mlq_ready_queue[prio];
         printf("sys_killall.c sys_killall: checking MLQ prio %d\n", prio);
         pthread_mutex_lock(&queue_lock);
 
         /* Check all processes in current priority queue */
         for (i = 0; i < queue->size; i++)
         {
             struct pcb_t *pcb = queue->proc[i];
             if (strstr(pcb->path, proc_name) != 0)
             {
                 printf("Killing process \"%s\" in MLQ...\n", proc_name);
                 pcb->pc = pcb->code->size; // Set PC to end of code segment
                 /* Remove from queue */
                 dequeue(queue);
             }
             i--;
         }
 
         pthread_mutex_unlock(&queue_lock);
     }
 #endif
     printf("sys_killall: finished killing processes\n");
     return 0;
 }
 