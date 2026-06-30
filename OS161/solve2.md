# Part 2: Bounded Buffer Producer/Consumer

## Files Needed

For this problem the important files are:

```text
OS161/src_asst1/producerconsumer.c
OS161/src_asst1/producerconsumer_driver.c
OS161/src_asst1/producerconsumer_driver.h
```

Inside the actual OS/161 container, they must be placed in:

```text
/home/os161user/os161/src/kern/asst1/producerconsumer.c
/home/os161user/os161/src/kern/asst1/producerconsumer_driver.c
/home/os161user/os161/src/kern/asst1/producerconsumer_driver.h
```

`producerconsumer.c` contains the actual synchronization solution.
`producerconsumer_driver.h` defines `struct pc_data`, `BUFFER_SIZE`, and the
function prototypes. `producerconsumer_driver.c` creates producer and consumer
threads for menu option `1b`.

## What Was Added to the Kernel

In `producerconsumer.c`, I added a circular FIFO buffer:

```c
static struct pc_data pc_buffer[BUFFER_SIZE];
static unsigned buffer_head;
static unsigned buffer_tail;
static unsigned buffer_count;
```

I also added the synchronization objects:

```c
static struct lock *pc_lock;
static struct cv *not_full_cv;
static struct cv *not_empty_cv;
static int active_producers;
```

`pc_lock` protects all shared buffer state. `not_full_cv` blocks producers
when the buffer is full. `not_empty_cv` blocks consumers when the buffer is
empty. `active_producers` lets consumers know when they should stop waiting
because no more items will be produced.

The shared resource is a fixed-size FIFO buffer of `struct pc_data` items.
Producers insert items, consumers remove items, and both groups share the
buffer indexes and count. These variables must not be touched without
holding the buffer lock.

The buffer is implemented as a circular array:

```c
static struct pc_data pc_buffer[BUFFER_SIZE];
static unsigned buffer_head;
static unsigned buffer_tail;
static unsigned buffer_count;
```

`buffer_tail` is the next write position and `buffer_head` is the next read
position. Both wrap with `% BUFFER_SIZE`. `buffer_count` distinguishes empty
from full.

Synchronization uses one lock and two condition variables. `pc_lock`
protects all buffer state and the `active_producers` count. Producers wait on
`not_full_cv` while the buffer is full. Consumers wait on `not_empty_cv`
while the buffer is empty and at least one producer is still running. The
waits are in `while` loops so the condition is rechecked after every wakeup.

The local PoliTO driver records producer lifetime with
`producerconsumer_inc_producers()` before forking a producer and
`producerconsumer_mark_producer_done()` when a producer finishes. When the
last producer exits, it broadcasts on `not_empty_cv`; this wakes consumers
that are sleeping on an empty buffer so they can return `false` and terminate
normally.

Expected result: `sys161 kernel "1b;q"` prints equal produced and consumed
totals. With the current driver, that is 200 produced and 200 consumed.

## How to Add It Into the OS/161 Kernel

Copy the solved files into the kernel assignment directory:

```bash
docker start polito-os161
docker cp "OS161/src_asst1/producerconsumer.c" \
  polito-os161:/home/os161user/os161/src/kern/asst1/producerconsumer.c
docker cp "OS161/src_asst1/producerconsumer_driver.c" \
  polito-os161:/home/os161user/os161/src/kern/asst1/producerconsumer_driver.c
docker cp "OS161/src_asst1/producerconsumer_driver.h" \
  polito-os161:/home/os161user/os161/src/kern/asst1/producerconsumer_driver.h
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
sys161 kernel '1b;q'
```

Correct output should include:

```text
Total produced: 200 (expected 200)
Total consumed: 200 (expected 200)
```

Some producer/consumer print lines may look mixed together. That is normal
because multiple kernel threads can call `kprintf` at the same time. The final
totals are the important correctness check.
