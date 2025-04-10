#include <stdio.h>
#include <stdint.h>

// Stack memory
#define SIZE_TASK_STACK			(5*1024)
#define SIZE_SCHED_STACK		(5*1024)

#define SRAM_START				(0x20000000U)
#define SIZE_SRAM				(64*1024) // 64kB SRAM
#define SRAM_END				(SRAM_START + SIZE_SRAM)

#define T1_STACK_START		(SRAM_END)
#define T2_STACK_START		(SRAM_END - (1*SIZE_TASK_STACK))
#define T3_STACK_START		(SRAM_END - (2*SIZE_TASK_STACK))
#define T4_STACK_START		(SRAM_END - (3*SIZE_TASK_STACK))
#define SCHED_STACK_START	(SRAM_END - (4*SIZE_TASK_STACK))


// Task handle function
void task1_handle(void);
void task2_handle(void);
void task3_handle(void);
void task4_handle(void);

int main(void)
{
    /* Loop forever */
	for(;;);
}

void task1_handle(void)
{
	while(1)
	{
		printf("Task 1\n");
	}
}

void task2_handle(void)
{
	while(1)
	{
		printf("Task 2\n");
	}
}

void task3_handle(void)
{
	while(1)
	{
		printf("Task 3\n");
	}
}

void task4_handle(void)
{
	while(1)
	{
		printf("Task 4\n");
	}
}
