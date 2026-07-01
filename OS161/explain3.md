# Part 3: Bar Synchronization — Output ব্যাখ্যা

## Output কী এসেছে

```
OS/161 kernel: 1c
Starting 8 customers, 2 bartenders
S 0 going home after mixing 16 drinks
S 1 going home after mixing 0 drinks

=== Part 3 results ===
All 8 customers and 2 bartenders finished
Bottle 1 used for 100 doses
Bottle 2 used for 100 doses
...
Bottle 10 used for 100 doses
The bar is closed, bye!!!
```

## সমস্যাটা কী?

একটা **বার (Bar)** এর simulation:
- **8 জন Customer** — বারে আসে, drink অর্ডার করে, drink পেলে খায়, আবার অর্ডার করে
  (প্রতিজন **2 রাউন্ড**, তাই মোট 8 × 2 = **16টা অর্ডার**)
- **2 জন Bartender** — customer এর অর্ডার নিয়ে drink বানিয়ে serve করে

Synchronization ছাড়া কী হতো:
- দুই bartender একই অর্ডার ধরে ফেলতো → এক customer দুইটা drink পেতো
- কোনো customer এর অর্ডার হারিয়ে যেতো → সে সারাজীবন অপেক্ষা করতো (deadlock)
- অর্ডার মিক্স হয়ে যেতো → ভুল customer ভুল drink পেতো

## Output এর প্রতিটা লাইন কেন এসেছে

### `Starting 8 customers, 2 bartenders`
Main thread 8টা customer thread আর 2টা bartender thread fork করলো। সবাই একসাথে
চলা শুরু করলো।

### `S 0 going home after mixing 16 drinks`
**S 0 = Bartender 0**। সে মোট **16টা drink** বানিয়েছে।

### `S 1 going home after mixing 0 drinks`
**S 1 = Bartender 1**। সে **0টা drink** বানিয়েছে।

### কেন Bartender 0 সব করলো, Bartender 1 কিছু করলো না?

এটা OS/161 এর **single-CPU scheduling** এর কারণে:

```
সময়রেখা:
──────────────────────────────────────────────→

Bartender 0:  [অর্ডার নাও][serve][অর্ডার নাও][serve]...[16টা শেষ][বাড়ি যাও]
Bartender 1:  [অপেক্ষা]...................................[কোনো অর্ডার নেই][বাড়ি যাও]
Customer রা:  [অর্ডার দাও → drink পাও → আবার অর্ডার দাও → drink পাও → বাড়ি যাও]
```

Bartender 0 এত দ্রুত কাজ করে ফেলেছে যে queue তে কখনো Bartender 1 এর জন্য
অর্ডার বাকি থাকেনি। **এটা ভুল না** — বাস্তব জীবনেও একজন দ্রুত waiter সব কাজ
করে ফেলতে পারে!

### `All 8 customers and 2 bartenders finished`
সব thread সফলভাবে শেষ হয়েছে:
- ✅ কোনো **deadlock** হয়নি (কেউ আটকে থাকেনি)
- ✅ কোনো **starvation** হয়নি (সব customer drink পেয়েছে)
- ✅ কোনো **race condition** হয়নি (ভুল drink কেউ পায়নি)

### `Bottle 1 used for 100 doses` ... `Bottle 10 used for 100 doses`
এগুলো driver এ hardcode করা cosmetic output। আমাদের PoliTO version এ actual
bottle tracking নেই। Question এর UNSW version এ এগুলো real ছিলো।

### `The bar is closed, bye!!!`
সব resource cleanup হয়ে গেছে — lock, CV, semaphore সব destroy হলো।

## Bar এর ভেতরে কী হচ্ছে — ধাপে ধাপে

### Customer এর জীবনচক্র (`bar_enter` + `bar_leave`):

```
Customer 3 বারে ঢুকলো
        │
        ▼
┌─ lock_acquire(bar_lock) ── দরজা বন্ধ করো
│
│   free slot খোঁজো → slot 5 পাওয়া গেলো
│   slot 5 এ নিজের order রাখো
│   order_queue তে slot 5 যোগ করো
│
│   cv_signal(order_cv) ──→ Bartender কে জাগাও: "অর্ডার এসেছে!"
│
│   while (!slots[5].served)
│       cv_wait(served_cv) ── ঘুমিয়ে যাও, drink আসা পর্যন্ত
│                              (lock release হয়ে যায়)
│
│   *** Bartender drink বানিয়ে served=true করলো ***
│   *** cv_broadcast(served_cv) দিয়ে জাগালো ***
│
│   জেগে উঠলাম! drink_id নিলাম
│   slot 5 খালি করলাম (occupied = false)
│   cv_signal(slot_cv) ──→ অন্য customer কে জানাও slot ফাঁকা হয়েছে
│
└─ lock_release(bar_lock) ── দরজা খুলে দাও
        │
        ▼
   bar_leave() → বের হয়ে গেলো
```

### Bartender এর জীবনচক্র (`bar_mix`):

```
Bartender 0 কাজ শুরু করলো
        │
        ▼
┌─ lock_acquire(bar_lock)
│
│   queue_count == 0? (অর্ডার নেই?)
│     হ্যাঁ, আর কোনো customer আসেনি → return false → বাড়ি যাও
│     হ্যাঁ, কিন্তু customer আসতে পারে → cv_wait(order_cv) → ঘুমাও
│     না → অর্ডার আছে! →
│
│   queue থেকে slot নম্বর বের করো (FIFO — প্রথমে আসলে প্রথমে পাবে)
│   drink_id সেট করো
│   slots[slot].served = true
│
│   cv_broadcast(served_cv) ──→ সব অপেক্ষারত customer কে জাগাও
│                                (শুধু যার slot served হয়েছে সে-ই এগোবে)
│
└─ lock_release(bar_lock)
        │
        ▼
   return true → driver "served" count বাড়ায়
   আবার loop এ যাও → আর অর্ডার আছে?
```

## তিনটা Condition Variable কেন?

| CV | কে অপেক্ষা করে | কে জাগায় | কেন |
|----|----------------|----------|-----|
| `order_cv` | Bartender | Customer | "অর্ডার এসেছে, কাজ করো" |
| `served_cv` | Customer | Bartender | "তোমার drink তৈরি, নিয়ে যাও" |
| `slot_cv` | Customer | Customer | "slot ফাঁকা হয়েছে, তুমি অর্ডার দিতে পারো" |

## FIFO Order কিভাবে maintain হচ্ছে?

```
Order Queue (circular):
┌───┬───┬───┬───┬───┐
│ 2 │ 5 │ 0 │   │   │  ← slot নম্বর রাখা আছে
└───┴───┴───┴───┴───┘
  ↑           ↑
 head        tail

Customer অর্ডার দিলে → tail এ যোগ হয়
Bartender অর্ডার নিলে → head থেকে নেয়

তাই যে আগে অর্ডার দিয়েছে, সে আগে drink পাবে!
```

## কেন `cv_broadcast` ব্যবহার হচ্ছে `cv_signal` এর বদলে?

`served_cv` তে একাধিক customer ঘুমিয়ে থাকতে পারে। Bartender যখন একটা drink
serve করে, সে **সবাইকে জাগায়** (`broadcast`)। কিন্তু শুধু যার slot `served == true`
সে-ই `while` loop থেকে বের হতে পারবে, বাকিরা আবার ঘুমিয়ে যাবে:

```c
// Customer এর code:
while (!slots[my_slot].served) {    // ← শুধু আমারটা served হলেই বের হবো
    cv_wait(served_cv, bar_lock);   // ← না হলে আবার ঘুমাও
}
```

## Deadlock কেন হচ্ছে না?

1. **একটাই Lock** (`bar_lock`) — nested locking নেই, তাই circular wait impossible
2. **CV wait করলে lock release হয়** — অন্য thread কাজ করতে পারে
3. **Customer শেষ হলে bartender কে জানায়** — `customers_in_bar == 0` হলে
   `cv_broadcast(order_cv)` করে, bartender ঘুম থেকে উঠে দেখে আর কাজ নেই, বাড়ি যায়
