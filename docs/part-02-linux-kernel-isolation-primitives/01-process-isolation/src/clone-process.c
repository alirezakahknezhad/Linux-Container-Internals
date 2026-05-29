#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>

/*Stack size for the child*/
#define STACK_SIZE (1024 * 64)          /* 64 KB */

/*Shared data we will modify*/
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
     * SIGCHLD   : send SIGCHLD to parent when child exits (needed for wait())
     * Notably ABSENT:
     *   NO CLONE_VM : child gets its OWN copy of address space
     *   NO CLONE_FS  : child gets its OWN filesystem context
     *   NO CLONE_FILES : child gets its OWN file descriptor table
     */
    child_pid = clone(child_func,
                      stack_top,
                      SIGCHLD,          /*no CLONE_VM = separate memory */
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
