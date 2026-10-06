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