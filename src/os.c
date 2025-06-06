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

// #undef MM_PAGING

#ifdef MM_PAGING
static int memramsz;
static int memswpsz[PAGING_MAX_MMSWP];

struct mmpaging_ld_args
{
	/* A dispatched argument struct to compact many-fields passing to loader */
	int vmemsz;
	struct memphy_struct *mram;
	struct memphy_struct **mswp;
	struct memphy_struct *active_mswp;
	int active_mswp_id;
	struct timer_id_t *timer_id;
};
#endif

static struct ld_args
{
	char **path;
	unsigned long *start_time;
#ifdef MLQ_SCHED
	unsigned long *prio;
#endif
} ld_processes;
int num_processes;

struct cpu_args
{
	struct timer_id_t *timer_id;
	int id;
};

static void *cpu_routine(void *args)
{
	struct timer_id_t *timer_id = ((struct cpu_args *)args)->timer_id;
	int id = ((struct cpu_args *)args)->id;
	/* Check for new process in ready queue */
	int time_left = 0;
	struct pcb_t *proc = NULL;
	while (1)
	{
		/* Check the status of current process */
		if (proc == NULL)
		{
			/* No process is running, the we load new process from
			 * ready queue */
			proc = get_proc();
			if (proc == NULL)
			{
				/* Fix infinite loop */
				if (done)
				{
					printf("\tCPU %d stopped\n", id);
					break;
				}

				next_slot(timer_id);
				continue; /* First load failed. skip dummy load */
			}
		}
		else if (proc->pc == proc->code->size)
		{
			/* The process has finished its job */
			printf("\tCPU %d: Processed %2d has finished\n", id, proc->pid);
			free(proc);
			proc = get_proc();
			time_left = 0;
		}
		else if (time_left == 0)
		{
			/* The process has done its job in current time slot */
			printf("\tCPU %d: Put process %2d to run queue\n", id, proc->pid);
			put_proc(proc);
			proc = get_proc();
		}

		/* Recheck process status after loading new process */
		if (proc == NULL && done)
		{
			/* No process to run, exit */
			printf("\tCPU %d stopped\n", id);
			break;
		}
		else if (proc == NULL)
		{
			/* There may be new processes to run in
			 * next time slots, just skip current slot */
			next_slot(timer_id);
			continue;
		}
		else if (time_left == 0)
		{
			printf("\tCPU %d: Dispatched process %2d\n", id, proc->pid);
			time_left = time_slot;
		}

		/* Run current process */
		uint32_t pc_before = proc->pc;
		struct inst_t *inst = &proc->code->text[pc_before];

		run(proc);
		time_left--;

		/* Print debug info với giá trị ban đầu */
		switch (inst->opcode)
		{
		case ALLOC:
			struct vm_rg_struct *rg = get_symrg_byid(proc->mm, inst->arg_1);

			printf("===== PHYSICAL MEMORY AFTER ALLOCATION =====\n");
			printf("PID=%d - Region=%d - Address=%08lx - Size=%d byte\n",
						 proc->pid, inst->arg_1, rg->rg_start, inst->arg_0);
			if (proc->mm != NULL)
			{
				print_pgtbl(proc);
				print_page_frame_mapping(proc);
			}
			printf("================================================================\n");
			break;
		case FREE:
			printf("===== PHYSICAL MEMORY AFTER DEALLOCATION =====\n");
			printf("PID=%d - Region=%d\n", proc->pid, inst->arg_0);
			if (proc->mm != NULL)
			{
				print_pgtbl(proc);
				print_page_frame_mapping(proc);
			}
			printf("================================================================\n");
			break;
		case CALC:
		case READ:
		case WRITE:
		case SYSCALL:
			break;
		}

		next_slot(timer_id);
	}
	detach_event(timer_id);
	pthread_exit(NULL);
}

static void *ld_routine(void *args)
{
#ifdef MM_PAGING
	struct memphy_struct *mram = ((struct mmpaging_ld_args *)args)->mram;
	struct memphy_struct **mswp = ((struct mmpaging_ld_args *)args)->mswp;
	struct memphy_struct *active_mswp = ((struct mmpaging_ld_args *)args)->active_mswp;
	struct timer_id_t *timer_id = ((struct mmpaging_ld_args *)args)->timer_id;
#else
	struct timer_id_t *timer_id = (struct timer_id_t *)args;
#endif
	int i = 0;
	printf("ld_routine\n");
	while (i < num_processes)
	{
		struct pcb_t *proc = load(ld_processes.path[i]);
#ifdef MLQ_SCHED
		proc->prio = ld_processes.prio[i];
#endif
		while (current_time() < ld_processes.start_time[i])
		{
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

static void read_config(const char *path)
{
	FILE *file;
	if ((file = fopen(path, "r")) == NULL)
	{
		printf("Cannot find configure file at %s\n", path);
		exit(1);
	}

	// Đọc thông tin cấu hình cơ bản
	fscanf(file, "%d %d %d\n", &time_slot, &num_cpus, &num_processes);

#ifdef MM_PAGING
	// Đọc thông tin cấu hình bộ nhớ
	fscanf(file, "%d", &memramsz);
	for (int i = 0; i < PAGING_MAX_MMSWP; i++)
	{
		fscanf(file, "%d", &memswpsz[i]);
	}
	fscanf(file, "\n");
#endif

	// Cấp phát bộ nhớ cho các mảng
	ld_processes.path = (char **)malloc(sizeof(char *) * num_processes);
	ld_processes.start_time = (unsigned long *)malloc(sizeof(unsigned long) * num_processes);
#ifdef MLQ_SCHED
	ld_processes.prio = (unsigned long *)malloc(sizeof(unsigned long) * num_processes);
#endif

	// Đọc thông tin từng process
	for (int i = 0; i < num_processes; i++)
	{
		ld_processes.path[i] = (char *)malloc(sizeof(char) * 100);
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

	fclose(file);
}

int main(int argc, char *argv[])
{
	/* Read config */
	if (argc != 2)
	{
		printf("Usage: os [path to configure file]\n");
		return 1;
	}
	char path[100];
	path[0] = '\0';
	strcat(path, "input/");
	strcat(path, argv[1]);
	read_config(path);

	pthread_t *cpu = (pthread_t *)malloc(num_cpus * sizeof(pthread_t));
	struct cpu_args *args =
			(struct cpu_args *)malloc(sizeof(struct cpu_args) * num_cpus);
	pthread_t ld;

	/* Init timer */
	int i;
	for (i = 0; i < num_cpus; i++)
	{
		args[i].timer_id = attach_event();
		args[i].id = i;
	}
	struct timer_id_t *ld_event = attach_event();
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
	for (sit = 0; sit < PAGING_MAX_MMSWP; sit++)
		init_memphy(&mswp[sit], memswpsz[sit], rdmflag);

	/* In Paging mode, it needs passing the system mem to each PCB through loader*/
	struct mmpaging_ld_args *mm_ld_args = malloc(sizeof(struct mmpaging_ld_args));

	mm_ld_args->timer_id = ld_event;
	mm_ld_args->mram = (struct memphy_struct *)&mram;
	mm_ld_args->mswp = (struct memphy_struct **)&mswp;
	mm_ld_args->active_mswp = (struct memphy_struct *)&mswp[0];
	mm_ld_args->active_mswp_id = 0;
#endif

	/* Init scheduler */
	init_scheduler();

	/* Run CPU and loader */
#ifdef MM_PAGING
	pthread_create(&ld, NULL, ld_routine, (void *)mm_ld_args);
#else
	pthread_create(&ld, NULL, ld_routine, (void *)ld_event);
#endif
	for (i = 0; i < num_cpus; i++)
	{
		pthread_create(&cpu[i], NULL,
									 cpu_routine, (void *)&args[i]);
	}

	/* Wait for CPU and loader finishing */
	for (i = 0; i < num_cpus; i++)
	{
		pthread_join(cpu[i], NULL);
	}
	pthread_join(ld, NULL);

	/* Stop timer */
	stop_timer();

	return 0;
}
