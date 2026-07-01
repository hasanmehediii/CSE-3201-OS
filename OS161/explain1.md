# Part 1: Concurrent Mathematics — Output ব্যাখ্যা

## Output কী এসেছে

```
OS/161 kernel: 1a
Starting 10 adder threads
Adder threads performed 10000 adds
Adder 0 performed 1000 increments.
Adder 1 performed 1003 increments.
Adder 2 performed 995 increments.
Adder 3 performed 1001 increments.
Adder 4 performed 998 increments.
Adder 5 performed 1003 increments.
Adder 6 performed 998 increments.
Adder 7 performed 1000 increments.
Adder 8 performed 1000 increments.
Adder 9 performed 1002 increments.
The adders performed 10000 increments overall
```

## সমস্যাটা কী ছিলো?

একটা **shared counter** আছে যেটা 0 থেকে শুরু হয়ে 10000 পর্যন্ত যাবে। কিন্তু একটা thread
না, **10টা thread** একসাথে এই counter বাড়াচ্ছে।

Lock ছাড়া কী হতো:
```
Thread A: counter পড়লো = 500
Thread B: counter পড়লো = 500  (A এখনো লেখেনি!)
Thread A: counter = 501 লিখলো
Thread B: counter = 501 লিখলো  (502 হওয়া উচিত ছিলো!)
```
এভাবে increment হারিয়ে যেতো, শেষে 10000 এর বদলে 8000 বা 9000 আসতো।

## সমাধান কিভাবে কাজ করছে

```c
lock_acquire(counter_lock);      // ← দরজা বন্ধ করো, অন্য কেউ ঢুকতে পারবে না
if (counter >= TARGET) {
    lock_release(counter_lock);
    break;
}
counter++;                       // ← নিরাপদে counter বাড়াও
counts[which]++;                 // ← নিজের হিসাব রাখো
lock_release(counter_lock);      // ← দরজা খুলে দাও, পরেরজন ঢুকতে পারবে
```

**Lock** মানে হলো একটা ঘরের তালা — একবারে শুধু একজন thread ঢুকতে পারে। বাকিরা
বাইরে দাঁড়িয়ে অপেক্ষা করে (`lock_acquire` এ block হয়ে থাকে)।

## Output এর প্রতিটা লাইন কেন এসেছে

| Output | কারণ |
|--------|-------|
| `Starting 10 adder threads` | Main thread 10টা child thread fork করলো |
| `Adder threads performed 10000 adds` | সব thread শেষ হওয়ার পর main thread দেখলো counter ঠিক 10000 |
| `Adder 0 performed 1000 increments` | Thread 0 মোট 1000 বার counter বাড়িয়েছে |
| `Adder 1 performed 1003 increments` | Thread 1 মোট 1003 বার বাড়িয়েছে |
| `The adders performed 10000 increments overall` | সব thread এর count যোগ করলে = 10000 ✅ |

## কেন প্রতিটা thread এর সংখ্যা আলাদা?

প্রতিটা thread প্রায় **1000** করে increment করেছে (10000 ÷ 10 = 1000), কিন্তু হুবহু
1000 না কারণ:

1. প্রতিবার increment এর পর `thread_yield()` call হয়
2. OS এর **scheduler** ঠিক করে কোন thread পরে চলবে
3. কখনো Thread 3 একটু বেশি সুযোগ পায়, কখনো Thread 7

এটা **সমস্যা না** — গুরুত্বপূর্ণ বিষয় হলো **মোট যোগফল সবসময় 10000**।

## Synchronization Flow

```
Thread 0                Thread 1                Thread 2
   |                       |                       |
lock_acquire() ←──────── অপেক্ষা ──────── অপেক্ষা
   |                       |                       |
counter++ (0→1)            |                       |
counts[0]++                |                       |
   |                       |                       |
lock_release() ─────→ lock_acquire()               |
   |                       |                       |
thread_yield()         counter++ (1→2)              |
   |                   counts[1]++                  |
   |                       |                       |
   |                   lock_release() ────→ lock_acquire()
   |                       |                       |
   |                   thread_yield()          counter++ (2→3)
   ...                     ...                     ...
```

## Main Thread কিভাবে জানে সব শেষ হয়েছে?

```c
// প্রতিটা adder thread শেষে:
V(done_sem);     // ← semaphore বাড়াও, "আমি শেষ" সিগনাল

// Main thread:
for (i = 0; i < 10; i++) {
    P(done_sem); // ← 10 বার অপেক্ষা করো, যতক্ষণ না সব thread সিগনাল দেয়
}
// এখানে আসলে মানে সব 10টা thread শেষ
```

`P(done_sem)` মানে: semaphore এর count 0 হলে ঘুমিয়ে যাও, কেউ `V()` করলে জাগো।
Main thread 10 বার `P()` করে, তাই 10টা thread এর সবগুলো `V()` না করা পর্যন্ত main
thread এগোয় না।
