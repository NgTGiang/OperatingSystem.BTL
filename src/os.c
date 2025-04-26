#include "cpu.h"
#include "timer.h"
#include "sched.h"
#include "loader.h"
#include "mm.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int time_slot;
static int num_cpus;
static int done = 0;

#ifdef MM_PAGING
static int memramsz;
static int memswpsz[PAGING_MAX_MMSWP];

struct mmpaging_ld_args {
	/* A dispatched argument struct to compact many-fields passing to loader */
	int vmemsz;
	struct memphy_struct *mram;
	struct memphy_struct **mswp;
	struct memphy_struct *active_mswp;
	int active_mswp_id;
	struct timer_id_t  *timer_id;
};
#endif

static struct ld_args{
	char ** path;
	unsigned long * start_time;
#ifdef MLQ_SCHED
	unsigned long * prio;
#endif
} ld_processes;
int num_processes;

struct cpu_args {
	struct timer_id_t * timer_id;
	int id;
};


static void * cpu_routine(void * args) {
	struct timer_id_t * timer_id = ((struct cpu_args*)args)->timer_id;
	int id = ((struct cpu_args*)args)->id;
	/* Check for new process in ready queue */
	int time_left = 0;
	struct pcb_t * proc = NULL;
	while (1) {
		/* Check the status of current process */
		if (proc == NULL) {
			/* No process is running, then we load new process from
			 * ready queue */
			proc = get_proc();
			if (proc == NULL) {
				if (done && queue_empty()) {
					/* No process to run, exit */
					printf("\tCPU %d stopped\n", id);
					break;
				}
				next_slot(timer_id);
				continue; /* First load failed. skip dummy load */
			}
			time_left = time_slot;
			printf("\tCPU %d: Dispatched process %2d\n", id, proc->pid);
		} else if (proc->pc == proc->code->size) {
			/* The process has finished its job */
			printf("\tCPU %d: Processed %2d has finished\n", id, proc->pid);
			free(proc);
			proc = NULL;
			continue;
		} else if (time_left == 0) {
			/* The process has done its job in current time slot */
			printf("\tCPU %d: Put process %2d to run queue\n", id, proc->pid);
			put_proc(proc);
			proc = NULL;
			continue;
		}
		
		/* Run current process */
		int pc_before = proc->pc;
		run(proc);
		time_left--;

		/* Print debug info */
		struct inst_t * inst = &proc->code->text[pc_before];
		switch (inst->opcode) {
		case CALC:
			printf("calc\n");
			break;
		case ALLOC:
			printf("===== PHYSICAL MEMORY AFTER ALLOCATION =====\n");
			printf("PID=%d - Region=%d - Address=%08x - Size=%d byte\n", 
				proc->pid, inst->arg_0, inst->arg_1, inst->arg_2);
			if (proc->mm != NULL) {
				print_pgtbl(proc, 0, 1024);
				/* Print page to frame mapping */
				for (int i = 0; i < 1024; i++) {
					if (proc->mm->pgd[i] != 0) {
						printf("Page Number: %d -> Frame Number: %d\n", 
							i, proc->mm->pgd[i] & 0xFFFF);
					}
				}
			}
			printf("================================================================\n");
			break;
		case FREE:
			printf("===== PHYSICAL MEMORY AFTER DEALLOCATION =====\n");
			printf("PID=%d - Region=%d\n", proc->pid, inst->arg_0);
			if (proc->mm != NULL) {
				print_pgtbl(proc, 0, 1024);
				/* Print page to frame mapping */
				for (int i = 0; i < 1024; i++) {
					if (proc->mm->pgd[i] != 0) {
						printf("Page Number: %d -> Frame Number: %d\n", 
							i, proc->mm->pgd[i] & 0xFFFF);
					}
				}
			}
			printf("================================================================\n");
			break;
		case READ:
			printf("===== PHYSICAL MEMORY AFTER READING =====\n");
			printf("read region=%d offset=%d value=%d\n", 
				inst->arg_0, inst->arg_1, inst->arg_2);
			if (proc->mm != NULL) {
				print_pgtbl(proc, 0, 1024);
				/* Print page to frame mapping */
				for (int i = 0; i < 1024; i++) {
					if (proc->mm->pgd[i] != 0) {
						printf("Page Number: %d -> Frame Number: %d\n", 
							i, proc->mm->pgd[i] & 0xFFFF);
					}
				}
			}
			printf("================================================================\n");
			printf("===== PHYSICAL MEMORY DUMP =====\n");
			printf("BYTE %08x: %d\n", inst->arg_1, inst->arg_2);
			printf("===== PHYSICAL MEMORY END-DUMP =====\n");
			printf("================================================================\n");
			break;
		case WRITE:
			printf("===== PHYSICAL MEMORY AFTER WRITING =====\n");
			printf("write region=%d offset=%d value=%d\n", 
				inst->arg_0, inst->arg_1, inst->arg_2);
			if (proc->mm != NULL) {
				print_pgtbl(proc, 0, 1024);
				/* Print page to frame mapping */
				for (int i = 0; i < 1024; i++) {
					if (proc->mm->pgd[i] != 0) {
						printf("Page Number: %d -> Frame Number: %d\n", 
							i, proc->mm->pgd[i] & 0xFFFF);
					}
				}
			}
			printf("================================================================\n");
			printf("===== PHYSICAL MEMORY DUMP =====\n");
			printf("BYTE %08x: %d\n", inst->arg_1, inst->arg_2);
			printf("===== PHYSICAL MEMORY END-DUMP =====\n");
			printf("================================================================\n");
			break;
		case SYSCALL:
			printf("syscall %d %d %d %d\n",
				inst->arg_0, inst->arg_1, inst->arg_2, inst->arg_3);
			break;
		default:
			printf("unknown instruction\n");
		}

		next_slot(timer_id);
	}
	/* Mark this device as finished */
	detach_event(timer_id);
	pthread_exit(NULL);
}

static void * ld_routine(void * args) {
#ifdef MM_PAGING
	struct memphy_struct* mram = ((struct mmpaging_ld_args *)args)->mram;
	struct memphy_struct** mswp = ((struct mmpaging_ld_args *)args)->mswp;
	struct memphy_struct* active_mswp = ((struct mmpaging_ld_args *)args)->active_mswp;
	struct timer_id_t * timer_id = ((struct mmpaging_ld_args *)args)->timer_id;
#else
	struct timer_id_t * timer_id = (struct timer_id_t*)args;
#endif
	int i = 0;
	printf("ld_routine\n");
	while (i < num_processes) {
		struct pcb_t * proc = load(ld_processes.path[i]);
#ifdef MLQ_SCHED
		proc->prio = ld_processes.prio[i];
#endif
		while (current_time() < ld_processes.start_time[i]) {
			next_slot(timer_id);
		}
#ifdef MM_PAGING
		proc->mm = malloc(sizeof(struct mm_struct));
		init_mm(proc->mm, proc);
		proc->mram = mram;
		proc->mswp = mswp;
		proc->active_mswp = active_mswp;
#endif
		printf("\tLoaded a process at %s, PID: %d PRIO: %ld\n",
			ld_processes.path[i], proc->pid, ld_processes.prio[i]);
		add_proc(proc);
		free(ld_processes.path[i]);
		i++;
		next_slot(timer_id);
	}
	free(ld_processes.path);
	free(ld_processes.start_time);
	done = 1;
	detach_event(timer_id);
	pthread_exit(NULL);
}

static void read_config(const char * path) {
	FILE * file;
	if ((file = fopen(path, "r")) == NULL) {
		printf("Cannot find configure file at %s\n", path);
		exit(1);
	}
	fscanf(file, "%d %d %d\n", &time_slot, &num_cpus, &num_processes);
	ld_processes.path = (char**)malloc(sizeof(char*) * num_processes);
	ld_processes.start_time = (unsigned long*)
		malloc(sizeof(unsigned long) * num_processes);

#ifdef MM_PAGING
	/* Read memory config */
	unsigned long tmp;
	fscanf(file, "%d %lu", &memramsz, &tmp);  // Read as unsigned long
	memswpsz[0] = tmp;  // Convert to int
	for(int sit = 1; sit < PAGING_MAX_MMSWP; sit++) {
		fscanf(file, "%d", &memswpsz[sit]);
	}
	fscanf(file, "\n");
#endif

#ifdef MLQ_SCHED
	ld_processes.prio = (unsigned long*)
		malloc(sizeof(unsigned long) * num_processes);
#endif

	int i;
	for (i = 0; i < num_processes; i++) {
		ld_processes.path[i] = (char*)malloc(sizeof(char) * 100);
		ld_processes.path[i][0] = '\0';
		strcat(ld_processes.path[i], "input/proc/");
		char proc[100];
#ifdef MLQ_SCHED
		fscanf(file, "%lu %s %lu\n", &ld_processes.start_time[i], proc, &ld_processes.prio[i]);
#else
		fscanf(file, "%lu %s\n", &ld_processes.start_time[i], proc);
#endif
		strcat(ld_processes.path[i], proc);
	}
}

int main(int argc, char * argv[]) {
	/* Read config */
	if (argc != 2) {
		printf("Usage: os [path to configure file]\n");
		return 1;
	}
	char path[100];
	path[0] = '\0';
	strcat(path, "input/");
	strcat(path, argv[1]);
	read_config(path);

	pthread_t * cpu = (pthread_t*)malloc(num_cpus * sizeof(pthread_t));
	struct cpu_args * args =
		(struct cpu_args*)malloc(sizeof(struct cpu_args) * num_cpus);
	pthread_t ld;
	
	/* Init timer */
	int i;
	for (i = 0; i < num_cpus; i++) {
		args[i].timer_id = attach_event();
		args[i].id = i;
	}
	struct timer_id_t * ld_event = attach_event();
	start_timer();

#ifdef MM_PAGING
	/* Init all MEMPHY include 1 MEMRAM and n of MEMSWP */
	int rdmflag = 1; /* By default memphy is RANDOM ACCESS MEMORY */

	struct memphy_struct mram;
	struct memphy_struct mswp[PAGING_MAX_MMSWP];

	/* Create MEM RAM */
	init_memphy(&mram, memramsz, rdmflag);

        /* Create all MEM SWAP */ 
	int sit;
	for(sit = 0; sit < PAGING_MAX_MMSWP; sit++)
	       init_memphy(&mswp[sit], memswpsz[sit], rdmflag);

	/* In Paging mode, it needs passing the system mem to each PCB through loader*/
	struct mmpaging_ld_args *mm_ld_args = malloc(sizeof(struct mmpaging_ld_args));

	mm_ld_args->timer_id = ld_event;
	mm_ld_args->mram = (struct memphy_struct *) &mram;
	mm_ld_args->mswp = (struct memphy_struct**) &mswp;
	mm_ld_args->active_mswp = (struct memphy_struct *) &mswp[0];
        mm_ld_args->active_mswp_id = 0;
#endif

	/* Init scheduler */
	init_scheduler();

	/* Run CPU and loader */
#ifdef MM_PAGING
	pthread_create(&ld, NULL, ld_routine, (void*)mm_ld_args);
#else
	pthread_create(&ld, NULL, ld_routine, (void*)ld_event);
#endif
	for (i = 0; i < num_cpus; i++) {
		pthread_create(&cpu[i], NULL,
			cpu_routine, (void*)&args[i]);
	}

	/* Wait for CPU and loader finishing */
	for (i = 0; i < num_cpus; i++) {
		pthread_join(cpu[i], NULL);
	}
	pthread_join(ld, NULL);

	/* Stop timer */
	stop_timer();

	return 0;

}



