# OS/161 Problem 1a — Complete Workflow Guide

## 1. Start the Docker Container

```bash
docker run -it marcopalena/polito-os161 /bin/bash
```

---

## 2. Create `math.c`

Inside the container, create the file:

```bash
cat > ~/os161/src/kern/test/math.c << 'MATHEOF'
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <clock.h>

#define NUM_THREADS  10
#define TARGET       10000

static volatile int counter;
static int counts[NUM_THREADS];
static struct semaphore *mutex;
static struct semaphore *done_sem;

static
void
adder(void *p, unsigned long which)
{
    (void)p;

    while (1) {
        P(mutex);

        if (counter >= TARGET) {
            V(mutex);
            break;
        }

        counter++;
        counts[which]++;

        V(mutex);

        thread_yield();
    }

    V(done_sem);
}

int
math(int nargs, char **args)
{
    (void)nargs;
    (void)args;

    int i, total, err;

    counter = 0;
    for (i = 0; i < NUM_THREADS; i++)
        counts[i] = 0;

    mutex = sem_create("mutex", 1);
    if (mutex == NULL)
        panic("math: sem_create mutex failed\n");

    done_sem = sem_create("done_sem", 0);
    if (done_sem == NULL)
        panic("math: sem_create done failed\n");

    kprintf("Starting %d adder threads\n", NUM_THREADS);

    for (i = 0; i < NUM_THREADS; i++) {
        err = thread_fork("adder", NULL, adder, NULL, i);
        if (err)
            panic("math: thread_fork failed: %s\n", strerror(err));
    }

    for (i = 0; i < NUM_THREADS; i++)
        P(done_sem);

    kprintf("Adder threads performed %d adds\n", counter);

    total = 0;
    for (i = 0; i < NUM_THREADS; i++) {
        kprintf("Adder %d performed %d increments.\n", i, counts[i]);
        total += counts[i];
    }
    kprintf("The adders performed %d increments overall\n", total);

    sem_destroy(mutex);
    sem_destroy(done_sem);

    return 0;
}
MATHEOF
```

---

## 3. Register `math.c` in `conf.kern`

```bash
printf 'file\t\ttest/math.c\n' >> ~/os161/src/kern/conf/conf.kern
```

Verify it was added correctly:

```bash
tail -3 ~/os161/src/kern/conf/conf.kern
```

Expected output:
```
file		test/fstest.c
optfile net	test/nettest.c
file		test/math.c
```

---

## 4. Add `math` declaration to `test.h`

```bash
sed -i 's/int cvtest2(int, char \*\*);/int cvtest2(int, char \*\*);\nint math(int, char \*\*);/' ~/os161/src/kern/include/test.h
```

Verify:

```bash
grep -n "math" ~/os161/src/kern/include/test.h
```

---

## 5. Add Menu Entry in `menu.c`

### 5a. Add entry to the test menu display list

```bash
sed -i 's/\[semu1-22\] Semaphore unit tests     ",/[semu1-22] Semaphore unit tests     ",\n\t"[1a]  Concurrent math test        ",/' ~/os161/src/kern/main/menu.c
```

### 5b. Add command dispatch entry

```bash
sed -i 's/{ "at",\t\tarraytest },/{ "1a",\t\tmath },\n\t{ "at",\t\tarraytest },/' ~/os161/src/kern/main/menu.c
```

Verify both changes:

```bash
grep -n "1a\|math" ~/os161/src/kern/main/menu.c
```

---

## 6. Configure the Kernel

```bash
cd ~/os161/src/kern/conf
./config DUMBVM
```

Expected output:
```
Configuration DUMBVM
Generating files... opt-dumbvm.h ...
Configuration in ../compile/DUMBVM done
Remember to make depend
```

---

## 7. Compile and Install the Kernel

> **Important:** Use `bmake` not `make` (the Makefiles use BSD make syntax)

```bash
cd ~/os161/src/kern/compile/DUMBVM
bmake depend
bmake
bmake install
```

---

## 8. Run the Test

```bash
cd ~/os161/root
sys161 kernel "1a;q"
```

### Expected Output

```
Starting 10 adder threads
Adder threads performed 10000 adds
Adder 0 performed 995 increments.
Adder 1 performed 1002 increments.
Adder 2 performed 997 increments.
Adder 3 performed 999 increments.
Adder 4 performed 997 increments.
Adder 5 performed 998 increments.
Adder 6 performed 1006 increments.
Adder 7 performed 1004 increments.
Adder 8 performed 1005 increments.
Adder 9 performed 997 increments.
The adders performed 10000 increments overall
Operation took 1.280050520 seconds
```

---

## 9. Copy Files to Local Machine (for VS Code editing)

From your **local terminal** (not inside container):

```bash
# Find your container ID
docker ps

# Copy source files locally
docker cp <container_id>:/home/os161user/os161/src ~/os161-data

# Open in VS Code
code ~/os161-data
```

---

## 10. Edit Locally and Sync Back to Container

After editing a file in VS Code locally, copy it back:

```bash
docker cp ~/os161-data/src/kern/test/math.c <container_id>:/home/os161user/os161/src/kern/test/math.c
```

Then inside the container, rebuild:

```bash
cd ~/os161/src/kern/compile/DUMBVM
bmake
bmake install
cd ~/os161/root
sys161 kernel "1a;q"
```

---

## Quick Reference

| Task | Command |
|---|---|
| Start container | `docker run -it marcopalena/polito-os161 /bin/bash` |
| Re-enter running container | `docker exec -it <container_id> /bin/bash` |
| Configure kernel | `cd ~/os161/src/kern/conf && ./config DUMBVM` |
| Compile | `cd ~/os161/src/kern/compile/DUMBVM && bmake` |
| Install kernel | `bmake install` |
| Run test | `cd ~/os161/root && sys161 kernel "1a;q"` |
| Copy out of container | `docker cp <id>:/home/os161user/os161/src ~/os161-data` |
| Copy into container | `docker cp ~/os161-data/src/kern/test/math.c <id>:/home/os161user/os161/src/kern/test/math.c` |

---

## How the Solution Works

| Primitive | Role |
|---|---|
| `sem_create("mutex", 1)` | Binary semaphore acting as a mutex (only 1 thread in critical section at a time) |
| `P(mutex)` | Acquire the mutex (like lock) |
| `V(mutex)` | Release the mutex (like unlock) |
| `sem_create("done_sem", 0)` | Semaphore for main thread to wait until all adders finish |
| `V(done_sem)` | Each adder signals it is done |
| `P(done_sem)` x10 | Main thread waits for all 10 adders |
| `thread_yield()` | Lets all 10 threads share the CPU fairly |