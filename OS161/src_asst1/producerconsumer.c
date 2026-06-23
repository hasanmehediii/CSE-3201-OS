/*
 * Part 2 of ASST1 - Bounded-buffer producer/consumer.
 *
 * This file implements a thread-safe circular buffer that allows
 * multiple producer threads to put items into the buffer and 
 * multiple consumer threads to take items out.
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>

#include "producerconsumer_driver.h"



/* The circular buffer array and its metadata */
static struct pc_data pc_buffer[BUFFER_SIZE];
static int buffer_head; // Index where the next item will be consumed from (front)
static int buffer_tail; // Index where the next item will be produced to (back)
static int buffer_count; // Number of items currently in the buffer

/* Synchronization primitives */
static struct lock *pc_lock;
static struct cv   *not_full_cv;
static struct cv   *not_empty_cv;

/* Counter to track active producer threads */
static int active_producers;

/* Initialize data structures and synchronization primitives */
void producerconsumer_startup(void)
{
	buffer_head = 0;
	buffer_tail = 0;
	buffer_count = 0;
	active_producers = 0;

	pc_lock = lock_create("pc_lock");
	if (pc_lock == NULL) {
		panic("producerconsumer_startup: lock_create failed\n");
	}

	not_full_cv = cv_create("not_full_cv");
	if (not_full_cv == NULL) {
		panic("producerconsumer_startup: cv_create failed (not_full)\n");
	}

	not_empty_cv = cv_create("not_empty_cv");
	if (not_empty_cv == NULL) {
		panic("producerconsumer_startup: cv_create failed (not_empty)\n");
	}
}

/* Clean up synchronization primitives */
void producerconsumer_shutdown(void)
{
	lock_destroy(pc_lock);
	cv_destroy(not_full_cv);
	cv_destroy(not_empty_cv);
}

/* Called by producer threads to add an item to the buffer */
void producer_produce(struct pc_data *item)
{
	lock_acquire(pc_lock);

	/* Wait until there is space in the buffer */
	while (buffer_count == BUFFER_SIZE) {
		cv_wait(not_full_cv, pc_lock);
	}

	/* Add the item to the tail of the buffer */
	pc_buffer[buffer_tail] = *item;
	buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
	buffer_count++;

	/* Signal any waiting consumer that the buffer is no longer empty */
	cv_signal(not_empty_cv, pc_lock);

	lock_release(pc_lock);
}

/* 
 * Called by consumer threads to consume an item.
 * Returns true if an item was consumed, false if no producers remain
 * and the buffer is empty.
 */
bool consumer_consume(struct pc_data *item)
{
	bool item_consumed = false;

	lock_acquire(pc_lock);

	/* Wait until the buffer is not empty OR there are no more active producers */
	while (buffer_count == 0 && active_producers > 0) {
		cv_wait(not_empty_cv, pc_lock);
	}

	/* Check if we actually have data to consume */
	if (buffer_count > 0) {
		*item = pc_buffer[buffer_head];
		buffer_head = (buffer_head + 1) % BUFFER_SIZE;
		buffer_count--;

		/* Signal any waiting producer that the buffer is no longer full */
		cv_signal(not_full_cv, pc_lock);
		
		item_consumed = true;
	}

	lock_release(pc_lock);
	
	return item_consumed;
}

/* Called when a new producer thread is created */
void producerconsumer_inc_producers(void)
{
	lock_acquire(pc_lock);
	active_producers++;
	lock_release(pc_lock);
}

/* Called when a producer thread finishes execution */
void producerconsumer_mark_producer_done(void)
{
	lock_acquire(pc_lock);
	active_producers--;
	
	KASSERT(active_producers >= 0);
	
	/* If there are no more producers, wake up all waiting consumers 
	 * so they can exit gracefully instead of blocking forever. */
	if (active_producers == 0) {
		cv_broadcast(not_empty_cv, pc_lock);
	}
	
	lock_release(pc_lock);
}
