#ifndef _BAR_DRIVER_H_
#define _BAR_DRIVER_H_

void bar_startup(void);
void bar_shutdown(void);
void bar_enter(int customer_id);
bool bar_mix(int bartender_id);
void bar_leave(int customer_id);

int runbar(int nargs, char **args);

#endif
