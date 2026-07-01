# Part 2: Producer/Consumer — Output ব্যাখ্যা

## Output কী এসেছে

```
OS/161 kernel: 1b
run_producerconsumer: starting up
Waiting for producer threads to exit...
Consumer started
Consumer started
Consumer started
Consumer started
Consumer started
Producer started
Producer started
Producer finished
Producer finished
Consumer finished normally
Consumer finished normally
Consumer finished normally
Consumer finished normally
Consumer finished normally
All producer threads have exited.
```

## সমস্যাটা কী?

এটা classic **Producer-Consumer** সমস্যা:
- **2 জন Producer** — তারা data তৈরি করে buffer এ রাখে (প্রতিজন 100টা item)
- **5 জন Consumer** — তারা buffer থেকে data নিয়ে process করে
- **Buffer** — মাঝখানে একটা fixed-size (10 slot) buffer আছে

সমস্যা হলো:
1. Buffer **ভর্তি** থাকলে producer কে অপেক্ষা করতে হবে
2. Buffer **খালি** থাকলে consumer কে অপেক্ষা করতে হবে
3. দুইজন একসাথে buffer এ হাত দিলে data নষ্ট হবে

## Output এর প্রতিটা লাইন কেন এসেছে

### `run_producerconsumer: starting up`
Main thread শুরু হলো। সব synchronization object (lock, CV) তৈরি হলো।

### `Consumer started` (5 বার)
5টা consumer thread তৈরি হয়ে চালু হলো। কিন্তু buffer এখনো খালি, তাই তারা সবাই
**`cv_wait(not_empty_cv)`** এ ঘুমিয়ে আছে — "buffer এ কিছু আসুক, তারপর জাগাও"।

### `Waiting for producer threads to exit...`
Main thread জানাচ্ছে সে producer দের শেষ হওয়ার জন্য অপেক্ষা করছে।

### `Producer started` (2 বার)
2টা producer thread চালু হলো। তারা এখন buffer এ item রাখা শুরু করলো।

### `Producer finished` (2 বার)
প্রতিটা producer 100টা item বানিয়ে buffer এ রেখেছে। মোট 200টা item। তারপর
`producerconsumer_mark_producer_done()` call করে জানিয়ে দিলো "আমি শেষ"।

**শেষ producer যখন done হয়:**
```c
active_producers--;
if (active_producers == 0) {
    cv_broadcast(not_empty_cv);  // ← সব ঘুমন্ত consumer কে জাগাও!
}
```
এখন consumer রা জানে আর কোনো data আসবে না, তাই buffer খালি হলেই তারা বের হয়ে যাবে।

### `Consumer finished normally` (5 বার)
প্রতিটা consumer buffer থেকে item নিতে থাকলো যতক্ষণ পারে। যখন buffer খালি আর কোনো
producer বাকি নেই, `consumer_consume()` return করে `false` — consumer বের হয়ে
গেলো "normally"।

**"normally" মানে** — consumer timeout বা error এ বের হয়নি, বরং সে জানে সব data
process হয়ে গেছে, তাই সুন্দরভাবে exit করেছে।

### `All producer threads have exited.`
Main thread 2 বার `P(prod_sem)` করে নিশ্চিত হলো দুইটা producer ই শেষ।

## Circular Buffer কিভাবে কাজ করে

```
Buffer (size 10):
┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
│ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ 8 │ 9 │
└───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
  ↑                               ↑
 head                            tail
 (consumer                     (producer
  এখান থেকে                    এখানে
  নেয়)                         রাখে)

Producer রাখলে: tail = (tail + 1) % 10  → ঘুরে ঘুরে যায়
Consumer নিলে:  head = (head + 1) % 10  → ঘুরে ঘুরে যায়
```

## Synchronization Flow

```
Producer                          Consumer
   |                                 |
lock_acquire(pc_lock)                |
   |                                 |
buffer ভর্তি?                        |
  হ্যাঁ → cv_wait(not_full_cv)       |   ← ঘুমিয়ে গেলো
  না  → item রাখো buffer এ          |
   |                                 |
cv_signal(not_empty_cv) ──────→ জেগে উঠলো!
   |                                 |
lock_release(pc_lock)           lock_acquire(pc_lock)
   |                                 |
   |                            buffer খালি?
   |                              হ্যাঁ ও producer আছে → cv_wait(not_empty_cv)
   |                              না → item নাও buffer থেকে
   |                                 |
   |                            cv_signal(not_full_cv)
   |                            lock_release(pc_lock)
```

## কেন 5 Consumer কিন্তু 2 Producer?

- **2 Producer × 100 items = 200 items** তৈরি হয়
- **5 Consumer** মিলে এই 200 items ভাগ করে নেয়
- কে কতটা পায় সেটা scheduling এর উপর নির্ভর করে
- গুরুত্বপূর্ণ বিষয়: **কোনো item হারায় না, কোনো item দুইবার process হয় না**

## কেন "Consumer finished normally" আর "Error! Consumer bored" না?

Lock/CV implement করার **আগে**: Consumer `cv_wait()` করতো, কিন্তু CV ছিলো empty
stub (কিছুই করতো না), তাই consumer কখনো block হতো না। Buffer খালি পেলে সাথে
সাথে timeout হয়ে "Error! Consumer bored" দিয়ে বের হয়ে যেতো।

Lock/CV implement করার **পরে**: Consumer সত্যিকারের `cv_wait()` করে ঘুমিয়ে যায়।
Producer item রাখলে `cv_signal()` দিয়ে জাগায়। সব item শেষ হলে `false` return
করে, consumer সুন্দরভাবে "finished normally" দিয়ে বের হয়।
