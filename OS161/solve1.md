# Part 1: Concurrent Mathematics

## Files Needed

For this problem the important file is:

```text
OS161/src_asst1/math.c
```

Inside the actual OS/161 container, this file must be placed in:

```text
/home/os161user/os161/src/kern/asst1/math.c
```

The menu/driver code calls `runmath()` when you run menu option `1a`.
So the kernel must compile this `math.c` with the rest of the `kern/asst1`
files.

## What Was Added to the Kernel

I added synchronization objects inside `math.c`:

```c
static struct lock *counter_lock;
static struct semaphore *done_sem;
```

`counter_lock` protects the shared counter and statistics array.
`done_sem` is used like a join mechanism so the main test thread waits until
all adder threads finish.

The shared state in this problem is the global counter and the per-thread
statistics array. The broken version lets several adder threads read and
write the counter at the same time, so increments are lost.

The solution uses one OS/161 lock, `counter_lock`, around the complete
counter critical section. Each thread acquires the lock, checks whether the
target has already been reached, increments the shared counter, and updates
its own statistics entry before releasing the lock. The target check and
increment must be protected together; otherwise two threads could both see
room below the target and push the final value too far.

The main thread uses `done_sem` as a join semaphore. Every adder calls
`V(done_sem)` after it leaves the loop, and `runmath` calls `P(done_sem)`
once for each child thread before printing the statistics and destroying the
synchronization objects.

Critical section:

```c
lock_acquire(counter_lock);
if (counter >= TARGET) {
        lock_release(counter_lock);
        break;
}
counter++;
counts[which]++;
lock_release(counter_lock);
```

Expected result: `sys161 kernel "1a;q"` always reports 10000 total adds, and
the sum of the per-thread counts is also 10000.

## How to Add It Into the OS/161 Kernel

Copy the solved file into the kernel assignment directory:

```bash
docker start polito-os161
docker cp "OS161/src_asst1/math.c" \
  polito-os161:/home/os161user/os161/src/kern/asst1/math.c
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
sys161 kernel '1a;q'
```

Correct output should include:

```text
Adder threads performed 10000 adds
The adders performed 10000 increments overall
```
