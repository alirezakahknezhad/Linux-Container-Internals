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