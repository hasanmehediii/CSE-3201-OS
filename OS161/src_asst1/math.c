#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <clock.h>
#include "math_driver.h"

#define NUM_THREADS  10
#define TARGET       10000

static volatile int counter;
static int counts[NUM_THREADS];

static struct lock *counter_lock;
static struct semaphore *done_sem;

static void adder(void *p, unsigned long which)
{
	(void)p;
	
	while (1) {
		lock_acquire(counter_lock);
		
		if (counter >= TARGET) {
			lock_release(counter_lock);
			break;
		}
		
		counter++;
		counts[which]++;
		
		lock_release(counter_lock);
		
		thread_yield();
	}
	
	V(done_sem);
}

int runmath(int nargs, char **args)
{
	int i, total, err;

	(void)nargs;
	(void)args;

	counter = 0;
	for (i = 0; i < NUM_THREADS; i++) {
		counts[i] = 0;
	}

	counter_lock = lock_create("counter_lock");
	if (counter_lock == NULL) {
		panic("runmath: lock_create failed\n");
	}

	done_sem = sem_create("done_sem", 0);
	if (done_sem == NULL) {
		panic("runmath: sem_create failed\n");
	}

	kprintf("Starting %d adder threads\n", NUM_THREADS);

	for (i = 0; i < NUM_THREADS; i++) {
		err = thread_fork("adder", NULL, adder, NULL, i);
		if (err) {
			panic("runmath: thread_fork failed: %s\n", strerror(err));
		}
	}

	for (i = 0; i < NUM_THREADS; i++) {
		P(done_sem);
	}

	kprintf("Adder threads performed %d adds\n", counter);

	total = 0;
	for (i = 0; i < NUM_THREADS; i++) {
		kprintf("Adder %d performed %d increments.\n", i, counts[i]);
		total += counts[i];
	}
	kprintf("The adders performed %d increments overall\n", total);

	lock_destroy(counter_lock);
	sem_destroy(done_sem);
	
	return 0;
}
