/*
 * Part 3 - The Bar problem solution.
 *
 * This implements the synchronization between Customer threads and Bartender threads.
 * 
 * Synchronization Design:
 * - A single lock (`bar_lock`) protects all shared state.
 * - Customers enter the bar, take a "ticket" (FIFO queue), and wait on `served_cv`.
 *   This ticket assigns them a strict position in the queue.
 * - They signal `order_cv` to wake up any sleeping bartenders.
 * - Bartenders wait on `order_cv` until there are tickets in the queue.
 * - A bartender finds the oldest ticket that hasn't been served yet, mixes the
 *   drink, marks the ticket as done, and broadcasts/signals `served_cv`.
 * - The customer wakes up, sees their ticket is done, collects their drink, and leaves.
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>

#include "bar.h"

#define MAX_TICKETS  256

/* Synchronization primitives */
static struct lock *bar_lock;
static struct cv   *order_cv;  /* Bartenders wait here for orders */
static struct cv   *served_cv; /* Customers wait here for their drinks */

/* Shared State */
static int next_ticket; /* The ticket number to be given to the next customer */
static int served_drink[MAX_TICKETS]; /* Stores the ID of the mixed drink for a ticket */
static int is_served[MAX_TICKETS]; /* Boolean array: 1 if the ticket has been served */

/* Initialize the bar state and synchronization primitives */
void bar_startup(void)
{
	next_ticket = 0;

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
	
	/* Initialize arrays (optional since static, but good practice) */
	for (int i = 0; i < MAX_TICKETS; i++) {
		is_served[i] = 0;
		served_drink[i] = -1;
	}
}

/* Clean up synchronization primitives */
void bar_shutdown(void)
{
	lock_destroy(bar_lock);
	cv_destroy(order_cv);
	cv_destroy(served_cv);
}

/* Called by customer thread to order a drink */
void bar_enter(int customer_id)
{
	lock_acquire(bar_lock);
	
	/* Customer takes a ticket for their order */
	int my_ticket = next_ticket++;
	
	/* Wake up a bartender to handle the order */
	cv_signal(order_cv, bar_lock);
	
	/* Wait until my specific ticket has been served */
	while (!is_served[my_ticket]) {
		/* Use broadcast because multiple customers might be waiting on served_cv, 
		 * and we want to ensure the right one wakes up to check their ticket. */
		cv_wait(served_cv, bar_lock);
	}
	
	int received_drink = served_drink[my_ticket];
	
	lock_release(bar_lock);

	kprintf("Customer %d served drink %d (ticket %d)\n", customer_id, received_drink, my_ticket);
}

/* Called by bartender thread to mix a drink */
void bar_mix(int bartender_id)
{
	lock_acquire(bar_lock);
	
	/* Wait until there is at least one order in the queue */
	while (next_ticket == 0) {
		cv_wait(order_cv, bar_lock);
	}
	
	/* Find the oldest ticket that has not been served yet (FIFO order) */
	int current_ticket = 0;
	while (current_ticket < next_ticket && is_served[current_ticket]) {
		current_ticket++;
	}
	
	/* It is possible we were woken up but all tickets are already served by other bartenders.
	 * If so, we just release the lock and return (or wait again). 
	 * The driver loop handles recalling bar_mix if active customers remain. */
	if (current_ticket < next_ticket) {
		/* Mix the drink and assign it to the ticket */
		served_drink[current_ticket] = (bartender_id * 1000) + current_ticket;
		is_served[current_ticket] = 1;
		
		/* Wake up customers so the one with this ticket can collect it */
		cv_broadcast(served_cv, bar_lock);
		
		kprintf("Bartender %d mixed drink %d (ticket %d)\n", bartender_id, served_drink[current_ticket], current_ticket);
	}
	
	lock_release(bar_lock);
}

/* Called by customer thread when leaving the bar */
void bar_leave(int customer_id)
{
	(void)customer_id;
	/* In this implementation, no action is required when leaving */
}
