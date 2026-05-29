# 🧑‍💻 PART 6 - clone System Call
## clone()
In the Linux operating system, a process or thread is created by a syscall called clone.<br>
🟥 Both `fork` and `pthread _create` use `clone()` in their implementation.<br>
Its manual shows us the following prototype : 
```
       /* Prototype for the glibc wrapper function */

       #define _GNU_SOURCE
       #include <sched.h>

       int clone(int (*fn)(void *_Nullable), void *stack, int flags,
                 void *_Nullable arg, ...  /* pid_t *_Nullable parent_tid,
                                              void *_Nullable tls,
                                              pid_t *_Nullable child_tid */ );

```
#### The following inputs are very important in this function : 
- `int (*fn)(void)` : This is a function pointer to a function that is executed by the child process.
- `*stack` : A downward-growing stack on which the child performs operations.
- `flags` : An integer value representing all flags used, configured using an OR-conjugation of all flags.
- `arg` : Additional arguments

##### 🔎 The most important parameter for us is the flag. So below we will review the most important flags.
- **CLONE_FILES** : Specifies whether file descriptors are shared with the parent process.
- **CLONE_VM** : Specifies whether virtual addresses are shared with the parent process.
- **CLONE_FS** : Specifies whether File System Informations are shared with the parent process.
- **CLONE_PARENT** : If this value is set, then the parent process is the calling process. Otherwise, the two are different.
- **CLONE_SIGHAND**: If this value is set, then the calling process and the child processes will have the same signal handler tables. If this value is not set, the child process will inherit a copy of the signal handlers from the calling process when it calls clone() , and sigactions performed later by one process will not affect the other process.
- **CLONE_PTRACE** : If this value is set and the Calling Process is traced, then the Child Process will also be traced.
- **CLONE_UNTRACED** : If this value is set, then a tracing process cannot apply ‍‍`CLONE TRACE` to this process.
- **CLONE_STOPPED** : If this value is set, then Child is initially stopped (as if a STOP signal had been sent to it) and must be resumed by sending it a SIGCOUNT signal.
- **CLONE_VFORK** : If this value is set, the execution of the Calling Process will be suspended until the Child Process comes and releases its Virtual Memory Resource.
- **CLONE_IO** : This flag causes the new task to share the same Disk I/O Scheduling Context as the Caller, so the Kernel treats its IOs as a workload.
- **And...**
### ❗Among the flags of this system call, two categories of flags are important : 
1. If the three items `CLONE_FILES`, `CLONE_FS`, and `CLONE_VM` are set, meaning that all three items are shared with the parent, the created Child is called a **`Thread`**.
2. A **`container`** is a process on the operating system that has access to system resources (such as network, FS, IPC, etc.) from different **`Namespaces`**. and these flags create new isolation boundaries : 

| Flag              | Namespace Created         |
| ----------------- | ------------------------- |
| `CLONE_NEWUTS`    | Hostname/domain isolation |
| `CLONE_NEWPID`    | PID namespace             |
| `CLONE_NEWNS`     | Mount namespace           |
| `CLONE_NEWNET`    | Network namespace         |
| `CLONE_NEWIPC`    | IPC namespace             |
| `CLONE_NEWUSER`   | User namespace            |
| `CLONE_NEWCGROUP` | Cgroup namespace          |
| `CLONE_NEWTIME`   | Time namespace            |

## 🛠️ Usage examples
### 1️⃣ Example 1 - clone as a process
```
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>

/* ── Stack size for the child ── */
#define STACK_SIZE (1024 * 64)          /* 64 KB */

/* ── Shared data we will modify ── */
static int shared_variable = 100;

/* ─────────────────────────────────────────────
 * Child function — this is what the child runs
 * ───────────────────────────────────────────── */
static int child_func(void *arg)
{
    printf("\n[PROCESS CHILD] PID      : %d\n", getpid());
    printf("[PROCESS CHILD] PPID     : %d\n",   getppid());
    printf("[PROCESS CHILD] shared_variable BEFORE change : %d\n",
           shared_variable);

    /*
     * Modify the variable.
     * Because address spaces are SEPARATED, this change
     * lives only in the child's copy-on-write page.
     * The parent will NOT see this change.
     */
    shared_variable = 999;

    printf("[PROCESS CHILD] shared_variable AFTER  change : %d\n",
           shared_variable);
    printf("[PROCESS CHILD] Address of shared_variable    : %p\n\n",
           (void *)&shared_variable);

    return 0;                           /* child exits cleanly*/
}

int main(void)
{
    pid_t child_pid;
    int   status;

    char *child_stack = malloc(STACK_SIZE);
    if (!child_stack) {
        perror("malloc");
        return 1;
    }
    char *stack_top = child_stack + STACK_SIZE;   /* top of the stack*/

    printf("[PARENT PROCESS] PID     : %d\n", getpid());
    printf("[PARENT PROCESS] shared_variable initial value: %d\n",
           shared_variable);
    printf("[PARENT PROCESS] Address of shared_variable   : %p\n\n",
           (void *)&shared_variable);

    /*
     * ── KEY FLAGS ──
     *
     * SIGCHLD   : send SIGCHLD to parent when child exits (needed for wait())
     *
     * Notably ABSENT:
     *   NO CLONE_VM    → child gets its OWN copy of address space
     *   NO CLONE_FS    → child gets its OWN filesystem context
     *   NO CLONE_FILES → child gets its OWN file descriptor table
     */
    child_pid = clone(child_func,
                      stack_top,
                      SIGCHLD,          /* <── no CLONE_VM = separate memory */
                      NULL);

    if (child_pid == -1) {
        perror("clone");
        free(child_stack);
        return 1;
    }

    printf("[PARENT PROCESS] Spawned child PID: %d\n", child_pid);

    /* Wait for child to finish */
    waitpid(child_pid, &status, 0);

    /*
     * The parent reads shared_variable AFTER the child modified it.
     * Because they have SEPARATE address spaces, the parent still
     * sees the ORIGINAL value: 100.
     */
    printf("[PARENT PROCESS] shared_variable AFTER child ran : %d\n",
           shared_variable);
    printf("[PARENT PROCESS] Address of shared_variable      : %p\n",
           (void *)&shared_variable);
    printf("\n>>> Address spaces were SEPARATE. "
           "Child's change did NOT affect parent.\n");

    free(child_stack);
    return 0;
}
```
**output:**
```
alireza@rootium:~/Linux-Container-Internals$ ./clone-process 
[PARENT PROCESS] PID     : 111949
[PARENT PROCESS] shared_variable initial value: 100
[PARENT PROCESS] Address of shared_variable   : 0x555f3d421010

[PARENT PROCESS] Spawned child PID: 111950

[PROCESS CHILD] PID      : 111950
[PROCESS CHILD] PPID     : 111949
[PROCESS CHILD] shared_variable BEFORE change : 100
[PROCESS CHILD] shared_variable AFTER  change : 999
[PROCESS CHILD] Address of shared_variable    : 0x555f3d421010

[PARENT PROCESS] shared_variable AFTER child ran : 100
[PARENT PROCESS] Address of shared_variable      : 0x555f3d421010

>>> Address spaces were SEPARATE. Child's change did NOT affect parent.
```

### 2️⃣ Example 2 - clone as a thread
```
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>

/* ── Stack size for the thread ── */
#define STACK_SIZE (1024 * 64)          /* 64 KB*/

/* ── Shared data we will modify ── */
static int shared_variable = 100;

/* ─────────────────────────────────────────────
 * Thread function — runs in the same address space
 * ───────────────────────────────────────────── */
static int thread_func(void *arg)
{
    printf("\n[THREAD CHILD]  PID (TID): %d\n", getpid());
    printf("[THREAD CHILD]  PPID     : %d\n",   getppid());
    printf("[THREAD CHILD]  shared_variable BEFORE change : %d\n",
           shared_variable);

    /*
     * Modify the variable.
     * Because address spaces are SHARED, this write goes directly
     * into the SAME physical memory the parent is using.
     * The parent WILL see this change immediately.
     */
    shared_variable = 999;

    printf("[THREAD CHILD]  shared_variable AFTER  change : %d\n",
           shared_variable);
    printf("[THREAD CHILD]  Address of shared_variable    : %p\n\n",
           (void *)&shared_variable);

    return 0;
}

int main(void)
{
    pid_t thread_tid;
    int   status;

    /*
     * Allocate a SEPARATE stack for the thread.
     * Even though memory is shared, each thread needs its own stack
     * for its own local variables and call frames.
     * Stack grows DOWNWARD — pass the TOP.
     */
    char *thread_stack = malloc(STACK_SIZE);
    if (!thread_stack) {
        perror("malloc");
        return 1;
    }
    char *stack_top = thread_stack + STACK_SIZE;

    printf("[PARENT THREAD]  PID     : %d\n", getpid());
    printf("[PARENT THREAD]  shared_variable initial value: %d\n",
           shared_variable);
    printf("[PARENT THREAD]  Address of shared_variable   : %p\n\n",
           (void *)&shared_variable);

    /*
     * ── KEY FLAGS ──
     *
     * CLONE_VM    : share the SAME virtual address space  - makes it a thread
     * CLONE_FS    : share filesystem root, cwd, umask
     * CLONE_FILES : share file descriptor table
     * CLONE_SIGHAND: share signal handlers
     * SIGCHLD     : notify parent on exit
     */
    thread_tid = clone(thread_func,
                       stack_top,
                       CLONE_VM          /* shared memory = thread */
                       | CLONE_FS        /* shared filesystem context */
                       | CLONE_FILES     /* shared file descriptors */
                       | CLONE_SIGHAND   /* shared signal handlers*/
                       | SIGCHLD,
                       NULL);

    if (thread_tid == -1) {
        perror("clone");
        free(thread_stack);
        return 1;
    }

    printf("[PARENT THREAD]  Spawned thread TID: %d\n", thread_tid);

    /* Wait for thread to finish */
    waitpid(thread_tid, &status, 0);

    /*
     * The parent reads shared_variable AFTER the thread modified it.
     * Because they SHARE the address space, the parent now sees 999.
     */
    printf("[PARENT THREAD]  shared_variable AFTER thread ran : %d\n",
           shared_variable);
    printf("[PARENT THREAD]  Address of shared_variable       : %p\n",
           (void *)&shared_variable);
    printf("\n>>> Address spaces were SHARED. "
           "Thread's change DID affect parent.\n");

    free(thread_stack);
    return 0;
}
```

**output:**
```
alireza@rootium:~/Linux-Container-Internals$ ./clone-thread 
[PARENT THREAD]  PID     : 112874
[PARENT THREAD]  shared_variable initial value: 100
[PARENT THREAD]  Address of shared_variable   : 0x62447dcf3010

[PARENT THREAD]  Spawned thread TID: 112875

[THREAD CHILD]  PID (TID): 112875
[THREAD CHILD]  PPID     : 112874
[THREAD CHILD]  shared_variable BEFORE change : 100
[THREAD CHILD]  shared_variable AFTER  change : 999
[THREAD CHILD]  Address of shared_variable    : 0x62447dcf3010

[PARENT THREAD]  shared_variable AFTER thread ran : 999
[PARENT THREAD]  Address of shared_variable       : 0x62447dcf3010

>>> Address spaces were SHARED. Thread's change DID affect parent.
```