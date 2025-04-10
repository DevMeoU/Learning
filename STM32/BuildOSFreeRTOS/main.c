#include <stdio.h>
#include <stdint.h>

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
