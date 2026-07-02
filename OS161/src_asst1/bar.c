/*
 * Part 3 of ASST1 - bar synchronization.
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>

#include "bar.h"

#define BAR_QUEUE_SIZE 64

static struct lock *bar_lock;
static struct cv *order_cv;
static struct cv *served_cv;
static struct cv *slot_cv;

struct order_slot {
	bool occupied;
	bool served;
	int customer_id;
	int drink_id;
};

static struct order_slot slots[BAR_QUEUE_SIZE];
static unsigned order_queue[BAR_QUEUE_SIZE];
static unsigned queue_head;
static unsigned queue_tail;
static unsigned queue_count;
static unsigned free_slots;
static unsigned customers_in_bar;
static bool has_seen_customer;
static int next_drink_id;

static
int
find_free_slot(void)
{
	unsigned i;

	for (i = 0; i < BAR_QUEUE_SIZE; i++) {
		if (!slots[i].occupied) {
			return (int)i;
		}
	}

	return -1;
}

void bar_startup(void)
{
	unsigned i;

	queue_head = 0;
	queue_tail = 0;
	queue_count = 0;
	free_slots = BAR_QUEUE_SIZE;
	customers_in_bar = 0;
	has_seen_customer = false;
	next_drink_id = 0;

	for (i = 0; i < BAR_QUEUE_SIZE; i++) {
		slots[i].occupied = false;
		slots[i].served = false;
		slots[i].customer_id = -1;
		slots[i].drink_id = -1;
		order_queue[i] = 0;
	}

	bar_lock = lock_create("bar_lock");
	if (bar_lock == NULL) {
		panic("bar_startup: lock_create failed\n");
	}

	order_cv = cv_create("order_cv");
	if (order_cv == NULL) {
		panic("bar_startup: cv_create failed (order_cv)\n");
	}

	served_cv = cv_create("served_cv");
	if (served_cv == NULL) {
		panic("bar_startup: cv_create failed (served_cv)\n");
	}

	slot_cv = cv_create("slot_cv");
	if (slot_cv == NULL) {
		panic("bar_startup: cv_create failed (slot_cv)\n");
	}
}

void bar_shutdown(void)
{
	cv_destroy(order_cv);
	cv_destroy(served_cv);
	cv_destroy(slot_cv);
	lock_destroy(bar_lock);
}

void bar_enter(int customer_id)
{
	int slot;
	int drink_id;

	lock_acquire(bar_lock);

	has_seen_customer = true;
	customers_in_bar++;

	while (free_slots == 0 || queue_count == BAR_QUEUE_SIZE) {
		cv_wait(slot_cv, bar_lock);
	}

	slot = find_free_slot();
	KASSERT(slot >= 0);

	slots[slot].occupied = true;
	slots[slot].served = false;
	slots[slot].customer_id = customer_id;
	slots[slot].drink_id = -1;
	free_slots--;

	order_queue[queue_tail] = (unsigned)slot;
	queue_tail = (queue_tail + 1) % BAR_QUEUE_SIZE;
	queue_count++;
	cv_signal(order_cv, bar_lock);

	while (!slots[slot].served) {
		cv_wait(served_cv, bar_lock);
	}

	drink_id = slots[slot].drink_id;
	(void)drink_id;

	slots[slot].occupied = false;
	slots[slot].served = false;
	slots[slot].customer_id = -1;
	slots[slot].drink_id = -1;
	free_slots++;
	customers_in_bar--;

	cv_signal(slot_cv, bar_lock);
	if (customers_in_bar == 0) {
		cv_broadcast(order_cv, bar_lock);
	}

	lock_release(bar_lock);
}

bool bar_mix(int bartender_id)
{
	unsigned slot;

	lock_acquire(bar_lock);

	while (queue_count == 0 && !has_seen_customer) {
		cv_wait(order_cv, bar_lock);
	}

	if (queue_count == 0) {
		lock_release(bar_lock);
		thread_yield();
		return false;
	}

	slot = order_queue[queue_head];
	queue_head = (queue_head + 1) % BAR_QUEUE_SIZE;
	queue_count--;

	KASSERT(slot < BAR_QUEUE_SIZE);
	KASSERT(slots[slot].occupied);
	KASSERT(!slots[slot].served);

	slots[slot].drink_id = bartender_id * 100000 + next_drink_id;
	next_drink_id++;
	slots[slot].served = true;

	cv_broadcast(served_cv, bar_lock);
	lock_release(bar_lock);

	/* Yield CPU so other bartenders can take orders */
	thread_yield();

	return true;
}

void bar_leave(int customer_id)
{
	(void)customer_id;
}
