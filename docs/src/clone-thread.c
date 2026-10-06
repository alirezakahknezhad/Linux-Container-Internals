#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>

/*Stack size for the thread*/
#define STACK_SIZE (1024 * 64)          /* 64 KB*/

/*Shared data we will modify*/
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
