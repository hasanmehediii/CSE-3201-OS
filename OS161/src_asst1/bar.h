/*
 * Part 3 - "The Bar" problem.
 *
 * Two kinds of customers and bartenders interact through a single
 * shared mixing machine. The mixing step must be atomic; multiple
 * customers queue up and bartenders serve them in FIFO order.
 *
 * Public interface (called by bar_driver.c):
 *   bar_startup()       - allocate locks/CVs
 *   bar_shutdown()      - destroy locks/CVs
 *   bar_enter()         - customer: take a queue slot
 *   bar_mix()           - bartender: serve next customer
 *   bar_leave()         - customer: leave the bar
 *
 * Configuration knobs:
 *   NUM_CUSTOMERS       - customers queued by the driver
 *   NUM_BARTENDERS      - concurrent bartenders
 */
#ifndef _BAR_DRIVER_H_
#define _BAR_DRIVER_H_

struct bar_data {
	int customer_id;
	int drink_id;
};

void bar_startup(void);
void bar_shutdown(void);
void bar_enter(int customer_id);
void bar_mix(int bartender_id);
void bar_leave(int customer_id);

int runbar(int nargs, char **args);

#endif
