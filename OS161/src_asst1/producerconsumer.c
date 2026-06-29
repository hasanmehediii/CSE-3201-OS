/*
 * Part 2 of ASST1 - Bounded-buffer producer/consumer.
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>

#include "producerconsumer_driver.h"

static struct pc_data pc_buffer[BUFFER_SIZE];
static unsigned buffer_head;
static unsigned buffer_tail;
static unsigned buffer_count;

static struct lock *pc_lock;
static struct cv *not_full_cv;
static struct cv *not_empty_cv;

static int active_producers;

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

void producerconsumer_shutdown(void)
{
	cv_destroy(not_full_cv);
	cv_destroy(not_empty_cv);
	lock_destroy(pc_lock);
}

void producer_produce(struct pc_data *item)
{
	lock_acquire(pc_lock);

	while (buffer_count == BUFFER_SIZE) {
		cv_wait(not_full_cv, pc_lock);
	}

	pc_buffer[buffer_tail] = *item;
	buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
	buffer_count++;

	cv_signal(not_empty_cv, pc_lock);
	lock_release(pc_lock);
}

bool consumer_consume(struct pc_data *item)
{
	lock_acquire(pc_lock);

	while (buffer_count == 0 && active_producers > 0) {
		cv_wait(not_empty_cv, pc_lock);
	}

	if (buffer_count == 0) {
		lock_release(pc_lock);
		return false;
	}

	*item = pc_buffer[buffer_head];
	buffer_head = (buffer_head + 1) % BUFFER_SIZE;
	buffer_count--;

	cv_signal(not_full_cv, pc_lock);
	lock_release(pc_lock);
	return true;
}

void producerconsumer_inc_producers(void)
{
	lock_acquire(pc_lock);
	active_producers++;
	lock_release(pc_lock);
}

void producerconsumer_mark_producer_done(void)
{
	lock_acquire(pc_lock);
	active_producers--;
	KASSERT(active_producers >= 0);

	if (active_producers == 0) {
		cv_broadcast(not_empty_cv, pc_lock);
	}

	lock_release(pc_lock);
}
