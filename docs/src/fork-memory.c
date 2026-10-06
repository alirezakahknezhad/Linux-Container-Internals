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