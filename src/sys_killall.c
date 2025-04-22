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
#include <stdlib.h>
#include "queue.h"
#include <string.h>


void remove_matched_process(struct queue_t *q, const char *target_name, uint32_t caller_pid);
void free_process_resources(struct pcb_t *proc);
void free_pcb_vmem(struct pcb_t *proc);
struct queue_t *new_queue()
{
    struct queue_t *q = (struct queue_t *)malloc(sizeof(struct queue_t));
    if (q) {
        q->size = 0;
        for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
            q->proc[i] = NULL;
        }
    }
    return q;
};
 
 
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
        if (data == -1) {
            proc_name[i] = '\0';
            break; // Add a break to exit the loop when data == -1
        }
        i++;
    }
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    if (caller->running_list) {
        remove_matched_process(caller->running_list, proc_name, caller->pid);
    }
    #ifdef MLQ_SCHED
        if (caller->mlq_ready_queue) {
            remove_matched_process(caller->mlq_ready_queue, proc_name, caller->pid);
        }
    #endif

    return 0;
 }
void remove_matched_process(struct queue_t *q, const char *target_name, uint32_t caller_pid) {

    struct pcb_t *proc;
    struct queue_t *tempq = new_queue();
    while ((proc = dequeue(q)) != NULL) {
        if (proc->pid != caller_pid && strcmp(proc->path, target_name)){
            printf("[sys_killall] Terminating PID %d (%s)\n", proc->pid, proc->path);
            // Giải phóng tài nguyên của tiến trình
            free_process_resources(proc);
        } else {
            enqueue(tempq, proc); // Giữ lại tiến trình không bị kill
        }
    }
 
    // Đưa lại các tiến trình không bị kill vào queue gốc
    while ((proc = dequeue(tempq)) != NULL) {
        enqueue(q, proc);
    }
 
    free(tempq);
}
void free_process_resources(struct pcb_t *proc) {
    if (proc->code) {
        free(proc->code->text); // Giải phóng vùng nhớ code
        free(proc->code);       // Giải phóng code segment
    }
    free_pcb_vmem(proc); // Giải phóng vùng nhớ ảo
    #ifdef MM_PAGING
    //free_pcb_memphy(proc); // Giải phóng vùng nhớ vật lý
    #endif
    if (proc->page_table) {
        // Duyệt qua các mục hợp lệ trong bảng cấp một
        for (int i = 0; i < proc->page_table->size; i++) {
            if (proc->page_table->table[i].next_lv) {
                free(proc->page_table->table[i].next_lv); // Giải phóng bảng cấp hai
                proc->page_table->table[i].next_lv = NULL;
            }
        }
        free(proc->page_table); // Giải phóng bảng cấp một
        proc->page_table = NULL;
    }
    free(proc); // Giải phóng PCB
}
void free_pcb_vmem(struct pcb_t *proc) {
    if (proc->mm && proc->mm->mmap){
        struct vm_area_struct *cur_vma = proc->mm->mmap;
        while (cur_vma) {
            struct vm_area_struct *next = cur_vma->vm_next;
            libfree(proc, cur_vma->vm_id);
            cur_vma = next;
        }
        proc->mm->mmap = NULL;
    }
}
 
 
 
 