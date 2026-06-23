/*
 * Part 1 of ASST1 - Concurrent mathematics.
 *
 * This file contains the implementation of a concurrent counting problem.
 * Several threads increment a shared counter until it reaches TARGET.
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <clock.h>
#include "math_driver.h"

#define NUM_THREADS  10
#define TARGET       10000

/* Shared variables */
static volatile int counter;
static int counts[NUM_THREADS];

/* Synchronization primitives */
static struct lock *counter_lock;
static struct semaphore *done_sem;

static void adder(void *p, unsigned long which)
{
	(void)p;
	
	while (1) {
		/* Acquire the lock before accessing shared state */
		lock_acquire(counter_lock);
		
		/* Check if we've reached the target count */
		if (counter >= TARGET) {
			/* Always release the lock before breaking out of the loop */
			lock_release(counter_lock);
			break;
		}
		
		/* Increment the global counter and the thread-specific counter */
		counter++;
		counts[which]++;
		
		/* Release the lock after modifying shared state */
		lock_release(counter_lock);
		
		/* Yield the CPU to encourage interleaving of threads */
		thread_yield();
	}
	
	/* Signal the main thread that this adder is finished */
	V(done_sem);
}

int runmath(int nargs, char **args)
{
	(void)nargs; (void)args;
	int i, total, err;

	counter = 0;
	for (i = 0; i < NUM_THREADS; i++) {
		counts[i] = 0;
	}

	/* Initialize the lock */
	counter_lock = lock_create("counter_lock");
	if (counter_lock == NULL) {
		panic("runmath: lock_create failed\n");
	}

	/* Initialize the semaphore used to wait for adder threads to finish */
	done_sem = sem_create("done_sem", 0);
	if (done_sem == NULL) {
		panic("runmath: sem_create failed\n");
	}

	kprintf("Starting %d adder threads\n", NUM_THREADS);

	/* Fork the adder threads */
	for (i = 0; i < NUM_THREADS; i++) {
		err = thread_fork("adder", NULL, adder, NULL, i);
		if (err) {
			panic("runmath: thread_fork failed: %s\n", strerror(err));
		}
	}

	/* Wait for all adder threads to finish using the semaphore */
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

	/* Clean up synchronization primitives */
	lock_destroy(counter_lock);
	sem_destroy(done_sem);
	
	return 0;
}
