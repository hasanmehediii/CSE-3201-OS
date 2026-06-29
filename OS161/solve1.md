# Part 1: Concurrent Mathematics

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
