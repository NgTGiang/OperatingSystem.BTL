/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "syscall.h"
#include "common.h"
#include "mm.h"
#include "libmem.h"

int __sys_memshow(struct pcb_t *caller, struct sc_regs *regs)
{
    printf("Memory: show the status of the memory allocation in heap and data segments\n");
    printf("=======================\n");
    
    // Hiển thị thông tin vùng nhớ heap
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, 0);
    if (cur_vma) {
        printf("Heap Segment (VMA 0):\n");
        printf("  Start Address: 0x%lx\n", cur_vma->vm_start);
        printf("  End Address: 0x%lx\n", cur_vma->vm_end);
        printf("  Current Break (sbrk): 0x%lx\n", cur_vma->sbrk);
        printf("  Size: %lu bytes\n", cur_vma->vm_end - cur_vma->vm_start);
        
        // Hiển thị các vùng nhớ đã cấp phát trong bảng ký hiệu
        printf("\nAllocated Regions:\n");
        int allocated_count = 0;
        int total_allocated = 0;
        
        for (int i = 0; i < PAGING_MAX_SYMTBL_SZ; i++) {
            struct vm_rg_struct *rg = &caller->mm->symrgtbl[i];
            if (rg->rg_start != 0 || rg->rg_end != 0) {
                printf("  Region %d: [0x%lx - 0x%lx] Size: %lu bytes\n", 
                       i, rg->rg_start, rg->rg_end, rg->rg_end - rg->rg_start);
                allocated_count++;
                total_allocated += (rg->rg_end - rg->rg_start);
            }
        }
        
        printf("\nSummary:\n");
        printf("  Total Regions: %d\n", allocated_count);
        printf("  Total Allocated Memory: %d bytes\n", total_allocated);
        
        // Hiển thị các vùng nhớ trống
        printf("\nFree Regions:\n");
        struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;
        int free_count = 0;
        int total_free = 0;
        
        while (rgit) {
            printf("  Free Region: [0x%lx - 0x%lx] Size: %lu bytes\n", 
                   rgit->rg_start, rgit->rg_end, rgit->rg_end - rgit->rg_start);
            free_count++;
            total_free += (rgit->rg_end - rgit->rg_start);
            rgit = rgit->rg_next;
        }
        
        printf("  Total Free Regions: %d\n", free_count);
        printf("  Total Free Memory: %d bytes\n", total_free);
    }
    
    // Hiển thị thông tin về bảng trang
    printf("\nPage Table Information:\n");
    print_pgtbl(caller);
    
    // Hiển thị ánh xạ trang-khung
    printf("\nPage-Frame Mapping:\n");
    print_page_frame_mapping(caller);
    
    return 0;
}

