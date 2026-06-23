/*
 * Part 2 driver. Spawns NUM_PRODUCERS producers and NUM_CONSUMERS
 * consumers. Each producer pushes ITEMS_PER_PRODUCER items then V()s the
 * done semaphore. Each consumer drains the buffer until all producers
 * have exited (and the buffer is empty), then V()s the done semaphore.
 * The parent P()s once per child so the menu only unblocks after
 * everyone is finished.
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>

#include "producerconsumer_driver.h"

#define NUM_PRODUCERS     2
#define NUM_CONSUMERS     2
#define ITEMS_PER_PRODUCER 100

static struct semaphore *done_sem;

static int produced_total;
static int consumed_total;
static struct lock *stats_lock;

static
void
producer(void *p, unsigned long which)
{
	(void)p;
	int i;
	struct pc_data item;
	for (i = 0; i < ITEMS_PER_PRODUCER; i++) {
		item.value1 = (int)which;
		item.value2 = i;
		producer_produce(&item);
		lock_acquire(stats_lock);
		produced_total++;
		lock_release(stats_lock);
	}
	producerconsumer_mark_producer_done();
	kprintf("Producer %lu exiting (made %d items)\n",
		which, ITEMS_PER_PRODUCER);
	V(done_sem);
}

static
void
consumer(void *p, unsigned long which)
{
	(void)p;
	struct pc_data item;
	int local_count = 0;
	while (consumer_consume(&item)) {
		local_count++;
		lock_acquire(stats_lock);
		consumed_total++;
		lock_release(stats_lock);
	}
	kprintf("Consumer %lu exiting (got %d items)\n",
		which, local_count);
	V(done_sem);
}

int run_producerconsumer(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	int i, err;

	produced_total = 0;
	consumed_total = 0;

	stats_lock = lock_create("stats_lock");
	if (stats_lock == NULL)
		panic("run_producerconsumer: stats lock create failed\n");

	done_sem = sem_create("done_sem", 0);
	if (done_sem == NULL)
		panic("run_producerconsumer: done_sem create failed\n");

	producerconsumer_startup();

	kprintf("Starting %d producers, %d consumers, buffer size %d\n",
		NUM_PRODUCERS, NUM_CONSUMERS, BUFFER_SIZE);

	for (i = 0; i < NUM_PRODUCERS; i++) {
		producerconsumer_inc_producers();
		err = thread_fork("producer", NULL, producer, NULL,
			          (unsigned long)i);
		if (err)
			panic("run_producerconsumer: producer fork: %s\n",
			      strerror(err));
	}

	for (i = 0; i < NUM_CONSUMERS; i++) {
		err = thread_fork("consumer", NULL, consumer, NULL,
			          (unsigned long)i);
		if (err)
			panic("run_producerconsumer: consumer fork: %s\n",
			      strerror(err));
	}

	for (i = 0; i < NUM_PRODUCERS + NUM_CONSUMERS; i++)
		P(done_sem);

	kprintf("\n=== Part 2 results ===\n");
	kprintf("Total produced: %d (expected %d)\n",
		produced_total, NUM_PRODUCERS * ITEMS_PER_PRODUCER);
	kprintf("Total consumed: %d (expected %d)\n",
		consumed_total, NUM_PRODUCERS * ITEMS_PER_PRODUCER);

	producerconsumer_shutdown();
	lock_destroy(stats_lock);
	sem_destroy(done_sem);

	return 0;
}
