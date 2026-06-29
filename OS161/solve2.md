# Part 2: Bounded Buffer Producer/Consumer

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
