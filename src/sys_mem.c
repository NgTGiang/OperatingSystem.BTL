/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "syscall.h"
#include "libmem.h"
#include "mm.h"
#include <stdlib.h>

//typedef char BYTE;

int __sys_memmap(struct pcb_t *caller, struct sc_regs* regs)
{
   int memop = regs->a1;
   BYTE value;

   switch (memop) {
   case SYSMEM_MAP_OP:
            /* Map memory for process */
            struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, regs->a2);
            if (cur_vma != NULL) {
                /* Allocate memory region */
                struct vm_rg_struct *new_rg = malloc(sizeof(struct vm_rg_struct));
                new_rg->rg_start = cur_vma->vm_start;
                new_rg->rg_end = cur_vma->vm_end;
                
                /* Map to physical memory */
                int num_pages = (new_rg->rg_end - new_rg->rg_start) / PAGING_PAGESZ;
                if (vm_map_ram(caller, new_rg->rg_start, new_rg->rg_end, 
                              new_rg->rg_start, num_pages, new_rg) < 0) {
                    free(new_rg);
                    return -1;
                }
            }
            break;
   case SYSMEM_INC_OP:
            inc_vma_limit(caller, regs->a2, regs->a3);
            break;
   case SYSMEM_SWP_OP:
            __mm_swap_page(caller, regs->a2, regs->a3);
            break;
   case SYSMEM_IO_READ:
            MEMPHY_read(caller->mram, regs->a2, &value);
            regs->a3 = value;
            break;
   case SYSMEM_IO_WRITE:
            MEMPHY_write(caller->mram, regs->a2, regs->a3);
            break;
   default:
            printf("Memop code: %d\n", memop);
            break;
   }
   
   return 0;
}


