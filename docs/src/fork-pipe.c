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