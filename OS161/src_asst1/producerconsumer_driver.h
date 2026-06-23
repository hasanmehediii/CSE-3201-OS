/*
 * Header for the bounded-buffer producer/consumer driver.
 */
#ifndef _PRODUCERCONSUMER_DRIVER_H_
#define _PRODUCERCONSUMER_DRIVER_H_

struct pc_data {
	int value1;
	int value2;
};

#define BUFFER_SIZE 10

void producerconsumer_startup(void);
void producerconsumer_shutdown(void);
void producer_produce(struct pc_data *item);
bool consumer_consume(struct pc_data *item);
void producerconsumer_inc_producers(void);
void producerconsumer_mark_producer_done(void);

int run_producerconsumer(int nargs, char **args);

#endif
