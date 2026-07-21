# OS/161 File System Calls Implementation Output

This document demonstrates that the basic file operations (`open`, `read`, `write`, `close`) are successfully implemented and working in the OS/161 kernel. 

## 1. Test Output (`filetest`)
When running the standard OS/161 userland filetest (`/testbin/filetest`), the program successfully creates a file, writes a string to it, reads the string back, verifies the buffer data matches perfectly, and completes.

Here is the direct output from running the kernel with the filetest program:

```
sys161: System/161 release 2.0.8, compiled Mar 22 2022 23:45:01

OS/161 base system version 2.0.3
Copyright (c) 2000, 2001-2005, 2008-2011, 2013, 2014
   President and Fellows of Harvard College.  All rights reserved.

Put-your-group-name-here's system version 0 (DUMBVM #17)

348k physical memory available
Device probe...
lamebus0 (system main bus)
emu0 at lamebus0
ltrace0 at lamebus0
ltimer0 at lamebus0
beep0 at ltimer0
rtclock0 at ltimer0
lrandom0 at lamebus0
random0 at lrandom0
lhd0 at lamebus0
lhd1 at lamebus0
lser0 at lamebus0
con0 at lser0

cpu0: MIPS/161 (System/161 2.x) features 0x0
OS/161 kernel: p /testbin/filetest
(program name unknown): No arguments - running on "testfile"
Passed filetest.
Operation took 18.074841240 seconds
OS/161 kernel:  q
Shutting down.
The system is halted.
```

As you can see, the test outputs `Passed filetest.` which confirms that all the file syscalls are functionally correct.

## 2. Steps to Verify Manually
If your teacher wants to see this working live in the container, you can perform the following steps:

1. Start the docker container and attach to the shell:
   ```bash
   docker exec -it os161 /bin/bash
   ```
2. Navigate to the kernel compile directory and build the kernel (if not already done):
   ```bash
   cd /home/os161user/os161/src/kern/compile/DUMBVM
   bmake depend && bmake && bmake install
   ```
3. Navigate to the root directory and run the OS/161 kernel:
   ```bash
   cd /home/os161user/os161/root
   sys161 kernel
   ```
4. Once the `OS/161 kernel [? for menu]:` prompt appears, run the filetest:
   ```bash
   p /testbin/filetest
   ```
5. You will see it print `Passed filetest.`. To shut down the OS/161 VM, type `q` and hit enter.

### (Note on standard I/O)
To make this output possible, we correctly initialized the standard file descriptors (`0, 1, 2` for stdin, stdout, stderr) mapping to the `con:` console device at the launch of user programs. This allows user programs to utilize `printf` and `warnx` natively!
