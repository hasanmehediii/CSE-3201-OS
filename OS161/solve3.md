# Part 3: Bar Synchronization

## Files Needed

For this problem the important files are:

```text
OS161/src_asst1/bar.c
OS161/src_asst1/bar.h
OS161/src_asst1/bar_driver.c
```

Inside the actual OS/161 container, they must be placed in:

```text
/home/os161user/os161/src/kern/asst1/bar.c
/home/os161user/os161/src/kern/asst1/bar.h
/home/os161user/os161/src/kern/asst1/bar_driver.c
```

`bar.c` contains the synchronization solution. `bar.h` contains the prototypes
used by the driver. `bar_driver.c` creates customer and bartender threads for
menu option `1c`.

## What Was Added to the Kernel

In `bar.c`, I added a bounded FIFO order queue and reusable order slots:

```c
#define BAR_QUEUE_SIZE 64

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
```

I also added synchronization primitives:

```c
static struct lock *bar_lock;
static struct cv *order_cv;
static struct cv *served_cv;
static struct cv *slot_cv;
```

`bar_lock` protects all shared bar state. `order_cv` is used by bartenders
waiting for orders. `served_cv` is used by customers waiting for their own
drinks. `slot_cv` is used when all reusable order slots are full.

In `bar.h` and `bar_driver.c`, `bar_mix` was changed to return `bool`:

```c
bool bar_mix(int bartender_id);
```

This lets the driver count only real served drinks. If a bartender calls
`bar_mix` when no order is available, it returns `false` and the served count
does not increase.

This repository contains the PoliTO ticket-queue version of the bar problem,
not the full UNSW bottle/ingredient driver. In this version customers call
`bar_enter`, wait for one drink, and then call `bar_leave`; bartenders call
`bar_mix` repeatedly until the driver sees that all customers are finished.

The shared state is the order queue, the reusable order slots, and the
per-slot served flags. The solution uses one lock, `bar_lock`, to protect all
of this state. Customers and bartenders communicate with three condition
variables:

`order_cv`: bartenders wait here before the first order arrives.

`served_cv`: customers wait here until their own slot is marked served.

`slot_cv`: customers wait here if all reusable order slots are occupied.

A customer entering the bar takes a free slot, stores its customer id, and
pushes the slot number into a circular FIFO queue. It then signals
`order_cv` and sleeps on `served_cv` until that exact slot is served. After
waking, the customer frees the slot and signals `slot_cv`, allowing later
orders to reuse the storage.

A bartender removes the oldest slot number from the FIFO queue, assigns a
drink id, marks the slot served, and broadcasts to `served_cv`. Only the
customer whose slot was served can pass its `while (!served)` check. This
keeps FIFO order without losing the connection between an order and the
customer waiting for it.

The earlier fixed-ticket solution could overflow after enough total orders.
This implementation reuses a bounded number of slots safely, so total orders
can exceed `BAR_QUEUE_SIZE` as long as no more than that many are outstanding
at the same time. `bar_mix` returns a boolean in this local driver so the
bartender statistics count real drinks only.

Expected result: `sys161 kernel "1c;q"` exits cleanly, every customer thread
finishes, and the total drinks served by bartenders matches the number of
customer rounds.

## How to Add It Into the OS/161 Kernel

Copy the solved files into the kernel assignment directory:

```bash
docker start polito-os161
docker cp "OS161/src_asst1/bar.c" \
  polito-os161:/home/os161user/os161/src/kern/asst1/bar.c
docker cp "OS161/src_asst1/bar.h" \
  polito-os161:/home/os161user/os161/src/kern/asst1/bar.h
docker cp "OS161/src_asst1/bar_driver.c" \
  polito-os161:/home/os161user/os161/src/kern/asst1/bar_driver.c
```

Then rebuild and install the kernel:

```bash
docker exec -it polito-os161 /bin/bash
cd /home/os161user/os161/src/kern/compile/DUMBVM
bmake depend
bmake
bmake install
```

Run the test:

```bash
cd /home/os161user/os161/root
sys161 kernel '1c;q'
```

Correct output should include:

```text
Starting 8 customers, 2 bartenders
All 8 customers and 2 bartenders finished
```

The bartender exit lines may look mixed together because both bartender
threads can print at the same time. That is not a synchronization failure.
The final line showing that all customers and bartenders finished is the main
success condition.
