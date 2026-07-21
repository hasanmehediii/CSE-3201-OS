# OS161 System Calls Implementation — Output Report

## Environment Setup

- **Docker Container**: `os161` (image `marcopalena/polito-os161`)
- **Source Path**: `/home/os161user/os161/src/`
- **Root (compiled output)**: `/home/os161user/os161/root/`
- **Kernel Config**: DUMBVM (348K physical memory)
- **Disk Image**: `LHD0.img` formatted with SFS via `hostbin/host-mksfs LHD0.img os161disk`
- **Final Kernel Build**: DUMBVM #31 (155456 bytes text)

---

## How to Run

```bash
# Enter the container
docker exec -it os161 bash

# Format disk (one-time setup)
cd /home/os161user/os161/root
hostbin/host-mksfs LHD0.img os161disk

# Run kernel with tests
sys161 -Z 20 kernel "mount sfs lhd0;p /testbin/proctest;q"
sys161 -Z 20 kernel "mount sfs lhd0;p /testbin/filetest testfile;q"
```

---

## Process System Call Tests — Results

```
=== Process System Call Tests ===

Test getpid:
  getpid() returned 2
  [PASS] getpid
Test fork+waitpid+exit:
  Child: PID is 3 (parent PID is 2)
  Parent: child 3 exited with status 42
  [PASS] fork+waitpid+exit
Test execv:
  The current process PID is: 3
  Child execv'd mygetpid successfully
  [PASS] execv

=== Results: 3/3 passed ===
```

```
Passed filetest.
```

---

## Implementation Details

### 1. Process Structure (`kern/include/proc.h`, `kern/proc/proc.c`)

**Added fields to `struct proc`:**
```c
pid_t p_pid;                    /* Process ID */
pid_t p_ppid;                   /* Parent Process ID */
int p_status;                   /* Exit status code */
bool p_exited;                  /* True if process has exited (zombie) */
struct semaphore *p_exitsem;    /* Semaphore for wait synchronization */
struct openfile *p_fdtable[OPEN_MAX]; /* Per-process file descriptor table */
```

**Global process table** (shared across files):
```c
#define MAX_PROC 100
struct proc *process_table[MAX_PROC];  /* non-static for cross-file access */
static struct spinlock ptable_lock;
```

**PID allocation:** `pid_alloc()` scans the process table for a free slot under a spinlock. `pid_free()` releases it on `proc_destroy()`.

**Process lifecycle:**
- `proc_create()` — allocates process, creates exit semaphore (initialized to 0), assigns PID
- `proc_destroy()` — closes all open files (decrements refcount, frees when 0), destroys address space, frees PID and semaphore

### 2. `getpid` (`kern/syscall/proc_syscalls.c`)

```c
int sys_getpid(pid_t *retval) {
    *retval = curproc->p_pid;
    return 0;
}
```

Trivially returns the current process's PID.

### 3. `fork` (`kern/syscall/proc_syscalls.c`)

1. Creates a new process via `proc_create_runprogram()`
2. Copies the parent's address space via `as_copy()`
3. Copies the trapframe (to resume child at syscall return point)
4. Copies the file descriptor table (increments refcount on each shared `openfile`)
5. Forks a new thread via `thread_fork()` with `enter_forked_process` as entry
6. Sets child's `p_ppid` to parent's PID
7. Returns child PID to parent, 0 to child

**`enter_forked_process()`** (in `kern/arch/mips/syscall/syscall.c`):
- Sets `tf_v0 = 0` (child sees fork() returning 0)
- Sets `tf_a3 = 0` (success)
- Increments `tf_epc` by 4 to skip the `syscall` instruction
- Calls `mips_usermode()` to enter user mode

### 4. `exit` (`kern/syscall/proc_syscalls.c`)

```c
void sys__exit(int exitcode) {
    curproc->p_status = _MKWAIT_EXIT(exitcode);
    curproc->p_exited = true;
    V(curproc->p_exitsem);    /* Wake parent if waiting */
    thread_exit();            /* Does NOT call proc_destroy */
}
```

**Key design:** `thread_exit()` was modified (`kern/thread/thread.c`) to **not** call `proc_destroy()`. Cleanup happens lazily in `waitpid` when the parent collects the exit status.

The semaphore `p_exitsem` is signaled with `V()` so any parent blocked in `waitpid` wakes up.

### 5. `waitpid` (`kern/syscall/proc_syscalls.c`)

1. Validates PID and status pointer
2. Looks up child in `process_table` under `p_lock` spinlock
3. Verifies child exists and is actually a child of the caller (`p_ppid` check)
4. If child hasn't exited, blocks on `P(child->p_exitsem)`
5. Copies exit status to user-space via `copyout()`
6. Calls `proc_destroy(child)` to clean up the zombie

### 6. `execv` (`kern/syscall/proc_syscalls.c`)

1. Copies path and argv from user space to kernel buffers using `copyinstr`/`copyin`
2. Opens the ELF binary via `vfs_open()`
3. **Saves old address space** before creating new one (critical fix — see below)
4. Creates new address space via `as_create()`, sets it active
5. Loads ELF via `load_elf()`
6. Defines user stack via `as_define_stack()`
7. Copies argv onto the user stack (pointer array + null-terminated strings, word-aligned)
8. Destroys old address space
9. Calls `enter_new_process()` to jump to user mode with new program

**Critical fix — graceful failure recovery:**
```c
oldas = curproc->p_addrspace;
as = as_create();
proc_setas(as);
result = load_elf(v, &entrypoint);
if (result) {
    as_destroy(as);           /* Free failed new AS */
    proc_setas(oldas);        /* Restore old AS */
    as_activate();            /* Re-activate it */
    goto fail_cleanup2;
}
// Only destroy old AS after load_elf succeeds:
if (oldas != NULL) as_destroy(oldas);
```

Without this, if `load_elf` failed (e.g., ENOMEM), the process would have no address space, causing a user-mode TLB miss → kernel panic.

### 7. File Operations (`kern/syscall/file_syscalls.c`)

**`struct openfile`** (defined in `kern/include/file_syscalls.h`):
```c
struct openfile {
    struct vnode *of_vnode;
    off_t of_offset;
    int of_accmode;
    int of_flags;
    int of_refcount;
    struct lock *of_lock;
};
```

Implemented operations:

| Syscall | Description |
|---------|-------------|
| `sys_open` | Opens file via `vfs_open()`, allocates `openfile` struct, finds free FD slot |
| `sys_read` | Reads from file using `VOP_READ()` with `UIO_USERSPACE` uio struct |
| `sys_write` | Writes to file using `VOP_WRITE()`; supports `O_APPEND` via `VOP_STAT` |
| `sys_close` | Decrements refcount, frees `openfile` and closes vnode when refcount hits 0 |
| `sys_fstat` | Gets file stats via `VOP_STAT()`, copies to user space |
| `sys_remove` | Removes file via `vfs_remove()` |
| `sys_getdirentry` | Reads directory entries via `VOP_GETDIRENTRY()` |
| `stdio_init` | Opens `con:` for stdin (fd 0), stdout (fd 1), stderr (fd 2) |

All operations use the per-file lock for thread safety and validate FD bounds.

### 8. Syscall Dispatcher (`kern/arch/mips/syscall/syscall.c`)

Cases added:
```c
case SYS_fork:      err = sys_fork(tf, &retval); break;
case SYS_getpid:    err = sys_getpid(&retval); break;
case SYS_waitpid:   err = sys_waitpid(tf_a0, tf_a1, tf_a2, &retval); break;
case SYS_execv:     err = sys_execv(tf_a0, tf_a1); break;
case SYS__exit:     sys__exit(tf_a0); panic("sys__exit returned");
```

### 9. Memory Management — dumbvm Free List (`kern/arch/mips/vm/dumbvm.c`)

**Problem:** `ram_stealmem()` is a forward-only bump allocator with no page reclamation. With 348K = 87 pages, fork+exit leaked pages and `load_elf` could not allocate enough for `execv`.

**Solution:** Added a simple linked-list free list for single-page reclamation:

```c
struct freelist { vaddr_t next; };
static struct freelist *freelist_head = NULL;

static void freelist_insert(paddr_t pa);   /* Add page to front of list */
static paddr_t freelist_alloc(unsigned long npages);  /* Pop one page if npages==1 */
```

- `getppages()` checks the free list first (for single pages) before calling `ram_stealmem()`
- `free_kpages()` adds freed pages back to the list
- `as_destroy()` iterates over all region pages and calls `free_kpages()` on each

**Stack size reduced** from 18 to 8 pages (`DUMBVM_STACKPAGES = 8`, 32KB per process) to fit within memory constraints after fork.

### 10. Argument Passing (`kern/syscall/runprogram.c`, `kern/syscall/proc_syscalls.c`)

`runprogram()` was modified to accept `argc` and `argv` parameters:
```c
int runprogram(char *progname, int argc, char **argv);
```

`menu.c`'s `cmd_progthread()` now forwards all command-line arguments to `runprogram()`.

Stack layout (growing downward):
```
[argv strings...]    (word-aligned)
[argv[0], argv[1], ..., NULL]   (pointer array)
         ^--- user_argv passed to enter_new_process
```

---

## Modified Files Summary

| File | Changes |
|------|---------|
| `kern/include/proc.h` | Added p_pid, p_ppid, p_status, p_exited, p_exitsem, p_fdtable |
| `kern/proc/proc.c` | Process table, pid_alloc/free, proc_destroy cleanup, stdio setup |
| `kern/syscall/proc_syscalls.c` | New file: fork, getpid, waitpid, exit, execv implementations |
| `kern/syscall/file_syscalls.c` | New file: open, read, write, close, fstat, remove, getdirentry, stdio_init |
| `kern/syscall/runprogram.c` | Accept argc/argv, copy args to user stack, call stdio_init |
| `kern/include/syscall.h` | Added sys_execv prototype |
| `kern/include/test.h` | Updated runprogram prototype to include argc/argv |
| `kern/arch/mips/syscall/syscall.c` | Added SYS_fork, SYS_getpid, SYS_waitpid, SYS_execv, SYS__exit cases |
| `kern/main/menu.c` | cmd_progthread forwards all args to runprogram |
| `kern/thread/thread.c` | thread_exit() no longer calls proc_destroy() |
| `kern/arch/mips/vm/dumbvm.c` | Free list, as_destroy frees pages, DUMBVM_STACKPAGES=8 |
| `userland/testbin/proctest/proctest.c` | New test: getpid, fork+waitpid+exit, execv (3/3 pass) |
