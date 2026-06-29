# Part 3: Bar Synchronization

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
