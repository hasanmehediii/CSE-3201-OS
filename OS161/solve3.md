# Solution to Part 3: Bar Synchronization Problem

## Problem Identification
In `bar.c`, we have multiple "Customer" threads and multiple "Bartender" threads. Customers arrive, place an order, and wait for a bartender to mix it. Bartenders wait for orders, mix them one by one, and serve the drinks to the specific customer who ordered them. We need a way to pair specific customers with their served drinks while maintaining strict First-In-First-Out (FIFO) ordering. 

*(Note: The Docker container provided contains the Politecnico di Torino "Ticket Queue" variant of the bar problem rather than the UNSW "Mixing Bottles" variant. The solution below satisfies the PoliTO driver logic).*

## How We Solved It
We used a **Ticket-based Queue System** synchronized by a single lock and two Condition Variables. This models a real-world deli counter or DMV where arriving customers take a numbered ticket.

### Steps Implemented
1.  **State Variables**:
    *   `next_ticket`: A counter that issues monotonically increasing ticket numbers.
    *   `is_served[MAX_TICKETS]`: A boolean array marking whether a specific ticket has been completed.
    *   `served_drink[MAX_TICKETS]`: An array storing the ID of the drink prepared for that specific ticket.
2.  **Synchronization Primitives**:
    *   `bar_lock`: Protects all the arrays and the `next_ticket` counter.
    *   `order_cv`: Bartenders sleep on this until `next_ticket > 0` (meaning at least one customer has ordered).
    *   `served_cv`: Customers sleep on this until their specific ticket's `is_served` flag becomes true.
3.  **Customer Logic (`bar_enter`)**:
    *   A customer acquires the lock and grabs `my_ticket = next_ticket++`.
    *   They wake up sleeping bartenders by calling `cv_signal(order_cv)`.
    *   They enter a `while(!is_served[my_ticket])` loop and wait on `served_cv`.
    *   Once woken up and served, they retrieve their drink from `served_drink[my_ticket]` and leave.
4.  **Bartender Logic (`bar_mix`)**:
    *   A bartender acquires the lock and waits on `order_cv` if there are no tickets.
    *   They loop through the tickets starting from 0 to find the oldest ticket where `is_served` is false (ensuring strict FIFO ordering).
    *   They assign a drink ID to that ticket, set its `is_served` flag to true, and call `cv_broadcast(served_cv)` to wake up the customers so the correct one can recognize their ticket is ready.

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
3.  **Run the kernel and execute the bar test (`1c`):**
    ```bash
    cd /home/os161user/os161/root
    sys161 kernel "1c;q"
    ```

You will see the output detailing each customer getting served by the bartenders, maintaining correct order without any deadlocks.
