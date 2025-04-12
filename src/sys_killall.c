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
#include <string.h>
#include <stdlib.h>

void free_pcb_mem(struct pcb_t *proc);

int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
{
    char proc_name[100];
    uint32_t data;

    //hardcode for demo only
    uint32_t memrg = regs->a1;
    
    /* TODO: Get name of the target proc */
    //proc_name = libread..
    int i = 0;
    data = 0;
    while(data != -1){
        libread(caller, memrg, i, &data);
        proc_name[i]= data;
        if(data == -1) proc_name[i]='\0';
        i++;
    }
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    /* TODO: Traverse proclist to terminate the proc
     *       stcmp to check the process match proc_name
     */
    // Process the running list
    if (caller->running_list != NULL) {
        for (i = 0; i < caller->running_list->size; i++) {
            struct pcb_t *proc = caller->running_list->proc[i];
            if (proc != NULL && strcmp(proc->path, proc_name) == 0) {
                // Free memory regions
                free_pcb_mem(proc);
                for (int j = i; j < caller->running_list->size - 1; j++) {
                    caller->running_list->proc[j] = caller->running_list->proc[j + 1];
                }
                caller->running_list->size--;
                free(proc);
                i--;
            }
        }
    }

    /* TODO Maching and terminating 
     *       all processes with given
     *        name in var proc_name
     */
#ifdef MLQ_SCHED
    // Process the MLQ ready queue
    if (caller->mlq_ready_queue != NULL) {
        for (i = 0; i < caller->mlq_ready_queue->size; i++) {
            struct pcb_t *proc = caller->mlq_ready_queue->proc[i];
            if (proc != NULL && strcmp(proc->path, proc_name) == 0) {
                free_pcb_mem(proc);
                
                for (int j = i; j < caller->mlq_ready_queue->size - 1; j++) {
                    caller->mlq_ready_queue->proc[j] = caller->mlq_ready_queue->proc[j + 1];
                }
                caller->mlq_ready_queue->size--;
                
                free(proc);
                i--;
            }
        }
    }
#endif

    return 0; 
}

// Helper function to free process memory
void free_pcb_mem(struct pcb_t *proc) {
    if (proc->mm && proc->mm->mmap) {
        struct vm_area_struct *vma = proc->mm->mmap;
        
        while (vma != NULL) {
            struct vm_area_struct *next_vma = vma->vm_next;
            
            libfree(proc, vma->vm_id);
            
            vma = next_vma;
        }
    }
}
