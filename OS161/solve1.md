# Solution to Part 1: Concurrent Mathematics Problem

## Problem Identification
The problem in `math.c` is a classic race condition. We have multiple threads (`NUM_THREADS = 10`) trying to increment a shared global variable `counter`. Since the increment operation (`counter++`) is not atomic at the machine-instruction level, multiple threads reading and writing to this variable simultaneously result in lost updates. This is why the final count would often fall short of the target 10000.

## How We Solved It
To solve this, we introduced a **mutual exclusion lock** (`struct lock`). The lock ensures that only one thread can execute the critical section (reading, modifying, and writing the counter) at a time.

### Steps Implemented
1.  **Lock Initialization**: We initialize `counter_lock` in `runmath()` before any threads are spawned using `lock_create("counter_lock")`.
2.  **Critical Section Definition**: Inside the `adder` thread function, we identified the critical section. This section includes checking the `counter` against `TARGET`, incrementing the `counter`, and incrementing the thread-specific `counts[which]`.
3.  **Acquire and Release**: We wrap the critical section with `lock_acquire(counter_lock)` and `lock_release(counter_lock)`.
4.  **Graceful Exit**: We ensure that if a thread breaks out of the `while (1)` loop because `counter >= TARGET`, it releases the lock **before** executing the `break` statement. Otherwise, it would cause a deadlock where the exiting thread holds the lock forever, preventing other threads from finishing.
5.  **Cleanup**: We call `lock_destroy(counter_lock)` at the end of `runmath()`.

## Commands to Compile and Run
To test this solution in your OS/161 environment inside the Docker container:

1.  **Enter the container:**
    ```bash
    docker exec -it polito-os161 /bin/bash
    ```
2.  **Navigate to the compile directory and build:**
    ```bash
    cd /home/os161user/os161/src/kern/compile/DUMBVM
    bmake depend
    bmake
    bmake install
    ```
3.  **Run the kernel and execute the math test (`1a`):**
    ```bash
    cd /home/os161user/os161/root
    sys161 kernel "1a;q"
    ```

You will see that the final sum is exactly 10000, and no updates are lost.
