# 🧑‍💻 Chapter 7 - Fork Syscall
## 🧩 Brief explanation
The fork() system call is one of the basic process creation mechanisms in Unix-like operating systems, including Linux. This call allows a running process to create a new process by duplicating itself.

The process that calls fork is called the parent process, and the newly created process is called the child process.

In fact, you can think of a fork as a photocopier that:
1. You put your running process in
2. You get two identical running processes out
3. Both continue executing from the exact same point
4. The only difference is what `fork()` returns to each one

## 🔎 Historical Note
`fork()` was present in the original **Unix (1969, Bell Labs)**. It is one of the oldest and most preserved system calls in computing history. The Linux implementation has evolved significantly under the hood — especially with the introduction of **Copy-on-Write (CoW)** memory semantics — but the API contract has remained essentially unchanged for over 50 years.

Let's take a closer look at this system call : 

Its prototype is as follows:

When a process calls fork, the Kernel initially creates a new call called a child process, which is the calling process of its parent process. The child process will initially receive a nearly identical execution context, and both processes will continue to execute independently.

✅ When fork is called, the kernel creates a nearly identical copy of the parent process. This copy ***includes** the following : 

|What is Copied|Description|
|---|---|
|Virtual address space|Code, stack, heap, data segments|
|File descriptors|Open files, sockets, pipes|
|Signal handlers|Registered signal dispositions|
|Environment variables|The full environment block|
|Process credentials|UID, GID, capabilities|
|Memory mappings|mmap regions|

🟥 However, some things are intentionally **NOT shared or are reset** : 

|What is NOT Copied / Reset|Reason|
|---|---|
|PID|Child gets a brand new unique PID|
|PPID|Child's PPID is set to parent's PID|
|Pending signals|Cleared in child|
|Memory locks|Not inherited|
|File locks (fcntl)|Not inherited by child|
|Timers|Reset in child|

## 🌟 COW (Copy-On-Write)
Cow is a resource management mechanism used in the Linux kernel. This concept is usually very explicit and visible in the fork system call. When a new process is created using the fork Syscall, `memory pages` are shared between the parent process and the child process. As long as the pages are shared, they cannot be changed. When either the parent process or the child process tries to modify a page, the kernel copies that page and marks it as `writable`.

## Return Value — The Bifurcation Point
After fork() returns, two processes are now running the same code. The only way to tell them apart is through the return value:
|Return Value|Who receives it|Meaning|
|---|---|---|
|`> 0`|Parent process|The PID of the newly created child|
|`= 0`|Child process|Signals "I am the child"|
|`= -1`|Parent process|Fork failed, no child was created|

## ❓ Limitations and disadvantages
While powerful, fork() has limitations for `containerization and threading`.
- Always creates a mostly independent process
- Cannot selectively create namespaces
- Cannot selectively share resources
- Cannot directly create threads

These limitations motivated the introduction of:
- clone()
- namespaces
- thread groups

## 🛠️ Usage examples
### 1️⃣ Example 1 - Basic Fork Usage
```
#include <stdio.h>
#include <unistd.h>

int main(void) {
    pid_t pid;
    printf("Before fork — PID: %d\n", getpid());

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        printf("Child Process\n");
        printf("PID  : %d\n", getpid());
        printf("PPID : %d\n", getppid());
    } else {
        printf("Parent Process\n");
        printf("PID  : %d\n", getpid());
        printf("Child PID : %d\n", pid);
    }

    return 0;
}
```
**output:**
```
alireza@rootium:~/Linux-Container-Internals$ ./basic-fork 
Before fork — PID: 23863
Parent Process
PID  : 23863
Child PID : 23864
Child Process
PID  : 23864
PPID : 23863
```

### 2️⃣ Example 2 - Demonstrating Memory Isolation
This example demonstrates : 
- Copy-on-Write behavior
- Independent address spaces
```
#include <stdio.h>
#include <unistd.h>

int main(void) {
    int x = 10;

    pid_t pid = fork();

    if (pid == 0) {
        x = 50;

        printf("\n[Child]\n");
        printf("x = %d\n", x);
        printf("Address of x: %p\n", &x);
    } else {
        sleep(1);

        printf("\n[Parent]\n");
        printf("x = %d\n", x);
        printf("Address of x: %p\n", &x);
    }

    return 0;
}
```
**output:**
```
alireza@rootium:~/Linux-Container-Internals$ ./fork-memory 

[Child]
x = 50
Address of x: 0x7ffd60f613e0

[Parent]
x = 10
Address of x: 0x7ffd60f613e0
```
### 3️⃣ Example 3 - Parent Waiting for Child 
In real programs, the parent almost always needs to wait for the child to finish. Without this, you get zombie processes.
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid;
    int   status;

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }
    else if (pid == 0) {
        /* ── Child ── */
        printf("[Child]  PID=%d starting work...\n", getpid());
        sleep(2);                        /* simulate some work      */
        printf("[Child]  PID=%d done. Exiting with code 42.\n", getpid());
        exit(42);                        /* exit with a known code  */
    }
    else {
        /* ── Parent ── */
        printf("[Parent] PID=%d waiting for child PID=%d...\n",
               getpid(), pid);

        pid_t finished = wait(&status);  /* blocks until child exits */

        if (WIFEXITED(status)) {
            printf("[Parent] Child PID=%d exited normally with code: %d\n",
                   finished, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status)) {
            printf("[Parent] Child killed by signal: %d\n",
                   WTERMSIG(status));
        }
    }

    return 0;
}
```
**output**
```
alireza@rootium:~/Linux-Container-Internals$ ./fork-wait 
[Parent] PID=29919 waiting for child PID=29920...
[Child]  PID=29920 starting work...
[Child]  PID=29920 done. Exiting with code 42.
[Parent] Child PID=29920 exited normally with code: 42
```
Key macros for inspecting exit status : 
|Macro|Meaning|
|---|---|
|`WIFEXITED(status)`|Did child exit normally?|
|`WEXITSTATUS(status)`|What was the exit code?|
|`WIFSIGNALED(status)`|Was child killed by a signal?|
|`WTERMSIG(status)`|Which signal killed it?|

### 4️⃣ Example 4 - The Classic Fork-Exec Pattern
This demonstrates the classic Unix process model. Used by:
- Shells
- Daemons
- Container runtimes
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid;
    int   status;

    /* Arguments to pass to the child program */
    char *args[] = { "ls", "-l", "/tmp", NULL };

    printf("[Parent] PID=%d — forking...\n", getpid());

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }
    else if (pid == 0) {
        /* ── Child ── */
        printf("[Child]  PID=%d — executing: ls -l /tmp\n", getpid());

        /*
         * execvp() replaces the child's memory image entirely.
         * If it succeeds, everything below this line is NEVER reached.
         * The child process is now "ls".
         */
        execvp("ls", args);

        /* If we reach here, exec failed */
        perror("execvp");
        exit(1);
    }
    else {
        /* ── Parent ── */
        printf("[Parent] PID=%d — waiting for child...\n", getpid());

        waitpid(pid, &status, 0);

        printf("[Parent] Child finished. Exit status: %d\n",
               WEXITSTATUS(status));
    }

    return 0;
}
```
**output:**
```
alireza@rootium:~/Linux-Container-Internals$ ./fork-exec 
[Parent] PID=34263 — forking...
[Parent] PID=34263 — waiting for child...
[Child]  PID=34264 — executing: ls -l /tmp
total 68
drwxr-xr-x 2 root    root    4096 May 29 09:42 ftnlhv
drwx------ 2 alireza alireza 4096 May 29 10:28 scoped_dirayBn1Y
drwx------ 2 root    root    4096 May 29 09:42 snap-private-tmp
drwx------ 3 root    root    4096 May 29 09:42 systemd-private-cb4c3eadf6f04292827cf30f6dc09c43-bluetooth.service-5imuq5
drwx------ 3 root    root    4096 May 29 09:42 systemd-private-cb4c3eadf6f04292827cf30f6dc09c43-colord.service-lJWioL
drwx------ 3 root    root    4096 May 29 09:43 systemd-private-cb4c3eadf6f04292827cf30f6dc09c43-fwupd.service-a1EjEB
drwx------ 3 root    root    4096 May 29 09:42 systemd-private-cb4c3eadf6f04292827cf30f6dc09c43-ModemManager.service-6FMtxc
drwx------ 3 root    root    4096 May 29 09:42 systemd-private-cb4c3eadf6f04292827cf30f6dc09c43-polkit.service-WUNa6K
drwx------ 3 root    root    4096 May 29 09:42 systemd-private-cb4c3eadf6f04292827cf30f6dc09c43-power-profiles-daemon.service-Rp345g
drwx------ 3 root    root    4096 May 29 09:42 systemd-private-cb4c3eadf6f04292827cf30f6dc09c43-switcheroo-control.service-H05egZ
drwx------ 3 root    root    4096 May 29 09:42 systemd-private-cb4c3eadf6f04292827cf30f6dc09c43-systemd-logind.service-SgVG1K
drwx------ 3 root    root    4096 May 29 09:42 systemd-private-cb4c3eadf6f04292827cf30f6dc09c43-systemd-oomd.service-YNCo8I
drwx------ 3 root    root    4096 May 29 09:42 systemd-private-cb4c3eadf6f04292827cf30f6dc09c43-systemd-resolved.service-eQvIQe
drwx------ 3 root    root    4096 May 29 09:42 systemd-private-cb4c3eadf6f04292827cf30f6dc09c43-systemd-timesyncd.service-5fnbzP
drwx------ 3 root    root    4096 May 29 09:42 systemd-private-cb4c3eadf6f04292827cf30f6dc09c43-upower.service-7jGPMc
drwx------ 2 root    root    4096 May 29 09:42 vmware-root
[Parent] Child finished. Exit status: 0
```
### 5️⃣ Example 5 — Multiple Child Processes
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_CHILDREN 4

int main(void)
{
    pid_t pids[NUM_CHILDREN];
    int   i, status;

    printf("[Parent] PID=%d — spawning %d children\n",
           getpid(), NUM_CHILDREN);

    /* ── Spawn all children ── */
    for (i = 0; i < NUM_CHILDREN; i++) {

        pids[i] = fork();

        if (pids[i] < 0) {
            perror("fork");
            exit(1);
        }
        else if (pids[i] == 0) {
            /*
             * IMPORTANT: child must not continue the loop.
             * Without break/exit, the child would fork its
             * own children — creating a fork explosion.
             */
            printf("[Child %d] PID=%d — working for %d seconds\n",
                   i, getpid(), i + 1);
            sleep(i + 1);
            printf("[Child %d] PID=%d — done\n", i, getpid());
            exit(i * 10);              /* each child exits with a unique code */
        }
        /* parent continues the loop to spawn the next child */
    }

    /* ── Parent waits for ALL children ── */
    for (i = 0; i < NUM_CHILDREN; i++) {
        pid_t finished = waitpid(pids[i], &status, 0);
        printf("[Parent] Child PID=%d exited with code %d\n",
               finished, WEXITSTATUS(status));
    }

    printf("[Parent] All children finished.\n");
    return 0;
}
```
**output:**
```
alireza@rootium:~/Linux-Container-Internals$ ./fork-multiple-child 
[Parent] PID=38131 — spawning 4 children
[Child 0] PID=38132 — working for 1 seconds
[Child 1] PID=38133 — working for 2 seconds
[Child 2] PID=38134 — working for 3 seconds
[Child 3] PID=38135 — working for 4 seconds
[Child 0] PID=38132 — done
[Parent] Child PID=38132 exited with code 0
[Child 1] PID=38133 — done
[Parent] Child PID=38133 exited with code 10
[Child 2] PID=38134 — done
[Parent] Child PID=38134 exited with code 20
[Child 3] PID=38135 — done
[Parent] Child PID=38135 exited with code 30
[Parent] All children finished.
```

### 6️⃣ Example 6 - Inter-Process Communication via Pipe
`fork()` + `pipe()` is the classic way for parent and child to communicate. This is how shell pipes `(ls | grep foo)` work internally.
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    int   pipefd[2];          /* pipefd[0]=read end, pipefd[1]=write end */
    pid_t pid;
    char  buffer[128];
    const char *message = "Hello from child process!";

    /* Create the pipe BEFORE forking so both processes inherit it */
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }
    else if (pid == 0) {
        /* ── Child: WRITER ── */
        close(pipefd[0]);              /* child doesn't need read end  */

        printf("[Child]  Sending: \"%s\"\n", message);
        write(pipefd[1], message, strlen(message) + 1);

        close(pipefd[1]);
        exit(0);
    }
    else {
        /* ── Parent: READER ── */
        close(pipefd[1]);              /* parent doesn't need write end */

        read(pipefd[0], buffer, sizeof(buffer));
        printf("[Parent] Received: \"%s\"\n", buffer);

        close(pipefd[0]);
        wait(NULL);
    }

    return 0;
}
```
**output:**
```
alireza@rootium:~/Linux-Container-Internals$ ./fork-pipe 
[Child]  Sending: "Hello from child process!"
[Parent] Received: "Hello from child process!"
```
