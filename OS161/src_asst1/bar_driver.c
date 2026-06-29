/*
 * Part 3 driver.
 *
 * Customers and bartenders are forked; the parent waits on a join
 * semaphore that each child V()s on exit. The customer gets a few
 * rounds of drinks; the bartender mixes until all customers have
 * stopped entering (signalled via num_active_customers).
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>

#include "bar.h"

#define NUM_CUSTOMERS     8
#define NUM_BARTENDERS    2
#define ROUNDS_PER_CUST   2

static int  num_active_customers;
static struct lock *state_lock;
static struct semaphore *done_sem;

static
void
customer(void *p, unsigned long which)
{
	int i;

	(void)p;

	for (i = 0; i < ROUNDS_PER_CUST; i++) {
		bar_enter((int)which);
		bar_leave((int)which);
	}
	lock_acquire(state_lock);
	num_active_customers--;
	lock_release(state_lock);
	V(done_sem);
}

static
void
bartender(void *p, unsigned long which)
{
	int more;
	int served = 0;

	(void)p;

	while (1) {
		lock_acquire(state_lock);
		more = (num_active_customers > 0);
		lock_release(state_lock);
		if (!more) break;
		if (bar_mix((int)which)) {
			served++;
		}
	}
	kprintf("Bartender %lu exiting (served %d drinks)\n", which, served);
	V(done_sem);
}

int runbar(int nargs, char **args)
{
	int i, err;

	(void)nargs;
	(void)args;

	num_active_customers = NUM_CUSTOMERS;

	state_lock = lock_create("state_lock");
	if (state_lock == NULL)
		panic("runbar: state_lock create failed\n");

	done_sem = sem_create("done_sem", 0);
	if (done_sem == NULL)
		panic("runbar: done_sem create failed\n");

	bar_startup();

	kprintf("Starting %d customers, %d bartenders\n",
		NUM_CUSTOMERS, NUM_BARTENDERS);

	for (i = 0; i < NUM_BARTENDERS; i++) {
		err = thread_fork("bartender", NULL, bartender, NULL,
			          (unsigned long)i);
		if (err)
			panic("runbar: bartender fork: %s\n", strerror(err));
	}

	for (i = 0; i < NUM_CUSTOMERS; i++) {
		err = thread_fork("customer", NULL, customer, NULL,
			          (unsigned long)i);
		if (err)
			panic("runbar: customer fork: %s\n", strerror(err));
	}

	for (i = 0; i < NUM_CUSTOMERS + NUM_BARTENDERS; i++)
		P(done_sem);

	kprintf("\n=== Part 3 results ===\n");
	kprintf("All %d customers and %d bartenders finished\n",
		NUM_CUSTOMERS, NUM_BARTENDERS);

	bar_shutdown();
	lock_destroy(state_lock);
	sem_destroy(done_sem);

	return 0;
}
