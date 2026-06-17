#include <gint/cpu.h>

volatile int cpu_sleep_block_counter = 0;

void sleep(void)
{
	if(cpu_sleep_block_counter <= 0) __asm__("sleep");
}

void sleep_block(void)
{
	cpu_atomic_start();
	cpu_sleep_block_counter++;
	cpu_atomic_end();
}

void sleep_unblock(void)
{
	cpu_atomic_start();
	cpu_sleep_block_counter--;
	cpu_atomic_end();
}
