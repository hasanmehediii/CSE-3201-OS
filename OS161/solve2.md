# Solution to Part 2: Bounded-Buffer Producer/Consumer Problem

## Problem Identification
In `producerconsumer.c`, multiple producer threads generate items and place them into a shared buffer, while multiple consumer threads take items out of the buffer. Because the buffer size is finite (`BUFFER_SIZE = 10`), producers must wait if the buffer is full, and consumers must wait if the buffer is empty. We need to prevent race conditions when accessing the buffer and ensure efficient waiting without busy-looping.

## How We Solved It
We used an array to act as a **circular buffer**, combined with a **lock** and **two condition variables** (CVs) for synchronization. 

### Steps Implemented
1.  **Circular Buffer Design**: We defined an array `pc_buffer[BUFFER_SIZE]`. We use `buffer_head` (where consumers read from) and `buffer_tail` (where producers write to), and a `buffer_count` to track how many items are currently in the buffer. Both head and tail wrap around using modulo arithmetic (`% BUFFER_SIZE`).
2.  **Synchronization Primitives**:
    *   `pc_lock`: A lock to protect all buffer state variables (`head`, `tail`, `count`, and the `active_producers` counter).
    *   `not_full_cv`: Producers wait on this when `buffer_count == BUFFER_SIZE`.
    *   `not_empty_cv`: Consumers wait on this when `buffer_count == 0`.
3.  **Producing Data**:
    *   The producer acquires the lock.
    *   It enters a `while (buffer_count == BUFFER_SIZE)` loop and calls `cv_wait(not_full_cv, pc_lock)`.
    *   Once space is available, it adds the item at `buffer_tail`, updates variables, signals `not_empty_cv` to wake up any sleeping consumers, and releases the lock.
4.  **Consuming Data**:
    *   The consumer acquires the lock.
    *   It waits in a `while (buffer_count == 0 && active_producers > 0)` loop using `cv_wait`. This ensures that consumers don't block forever if all producers have finished generating items.
    *   If items are available, it reads from `buffer_head`, signals `not_full_cv` for sleeping producers, and returns true. If the buffer is empty and no producers are left, it returns false, causing the consumer thread to exit gracefully.
5.  **Shutdown Logic**: `producerconsumer_mark_producer_done()` decrements the `active_producers` count. When it hits zero, it broadcasts to `not_empty_cv` to flush out and terminate any remaining sleeping consumers.

## Commands to Compile and Run
1.  **Enter the container:**
    ```bash
    docker exec -it polito-os161 /bin/bash
    ```
2.  **Rebuild the kernel:**
    ```bash
    cd /home/os161user/os161/src/kern/compile/DUMBVM
    bmake depend && bmake && bmake install
    ```
3.  **Run the kernel and execute the producer/consumer test (`1b`):**
    ```bash
    cd /home/os161user/os161/root
    sys161 kernel "1b;q"
    ```

The output will confirm that exactly the expected number of items (e.g., 200 items for 2 producers making 100 each) were both produced and consumed.
