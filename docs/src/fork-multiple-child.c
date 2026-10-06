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