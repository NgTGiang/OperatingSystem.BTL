# OperatingSystem.BTL

## REPORT:
- Latex report: [link](https://www.overleaf.com/project/67c2592b2ffe59d56a58d525)

## MEMORY:
1. **libmem.c**: Implements memory management functions, including allocation, deallocation, and paging.
2. **mem.c**: This file handles memory management.
    
    1. alloc_mem(): Allocates physical memory for a process.

    2. free_mem(): Frees physical memory allocated to a process.

    3. read_mem(): Reads data from physical memory.

    4. write_mem(): Writes data to physical memory.

3. **mm-memphy.c**: These files manage physical memory.
4. **mm-vm.c**: These files manage memory virtualization
5. **mm.c**: Implements paging-based memory management, including page table management and virtual memory operations.
6. **sys_mem.c**: handles memory management syscalls.