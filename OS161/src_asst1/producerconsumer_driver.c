/*
 * Producer/consumer driver matching the original UNSW ASST1 format.
 *
 * Spawns NUM_CONSUMERS consumers first, then NUM_PRODUCERS producers.
 * The main thread waits for all producers, then waits for all consumers.
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <clock.h>

#include "producerconsumer_driver.h"

#define NUM_PRODUCERS 2
#define NUM_CONSUMERS 5
#define ITEMS_PER_PRODUCER 100

static struct semaphore *prod_sem;
static struct semaphore *cons_sem;

static void
producer(void *p, unsigned long which)
{
	int i;
	struct pc_data item;

	(void)p;
	(void)which;

	kprintf("Producer started\n");

	for (i = 0; i < ITEMS_PER_PRODUCER; i++)
	{
		item.value1 = (int)which;
		item.value2 = i;
		producer_produce(&item);
	}

	producerconsumer_mark_producer_done();
	kprintf("Producer finished\n");
	V(prod_sem);
}

static void
consumer(void *p, unsigned long which)
{
	struct pc_data item;

	(void)p;
	(void)which;

	kprintf("Consumer started\n");

	while (consumer_consume(&item))
	{
		/* process item (nothing to do here) */
	}

	kprintf("Consumer finished normally\n");
	V(cons_sem);
}

int run_producerconsumer(int nargs, char **args)
{
	int i, err;

	(void)nargs;
	(void)args;

	prod_sem = sem_create("prod_sem", 0);
	if (prod_sem == NULL)
		panic("run_producerconsumer: prod_sem create failed\n");

	cons_sem = sem_create("cons_sem", 0);
	if (cons_sem == NULL)
		panic("run_producerconsumer: cons_sem create failed\n");

	producerconsumer_startup();

	kprintf("run_producerconsumer: starting up\n");

	/* Fork consumer threads first */
	for (i = 0; i < NUM_CONSUMERS; i++)
	{
		err = thread_fork("consumer", NULL, consumer, NULL,
						  (unsigned long)i);
		if (err)
			panic("run_producerconsumer: consumer fork: %s\n",
				  strerror(err));
	}

	/* Fork producer threads */
	for (i = 0; i < NUM_PRODUCERS; i++)
	{
		producerconsumer_inc_producers();
		err = thread_fork("producer", NULL, producer, NULL,
						  (unsigned long)i);
		if (err)
			panic("run_producerconsumer: producer fork: %s\n",
				  strerror(err));
	}

	kprintf("Waiting for producer threads to exit...\n");

	/* Wait for all producers to finish */
	for (i = 0; i < NUM_PRODUCERS; i++)
		P(prod_sem);

	kprintf("All producer threads have exited.\n");

	/* Wait for all consumers to finish */
	for (i = 0; i < NUM_CONSUMERS; i++)
		P(cons_sem);

	producerconsumer_shutdown();

	sem_destroy(prod_sem);
	sem_destroy(cons_sem);

	return 0;
}
