#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(void)
{
    printf("[1] Starting snooper...\n");

    key_t key = ftok("/tmp", 65);

    if (key == (key_t)-1) {
        perror("[ERROR] ftok");
        return EXIT_FAILURE;
    }

    printf("[2] ftok() succeeded\n");
    printf("    key = %d\n", key);

    int shmid = shmget(key, 1024, 0666);

    if (shmid == -1) {
        perror("[ERROR] shmget");
        return EXIT_FAILURE;
    }

    printf("[3] shmget() succeeded\n");
    printf("    shmid = %d\n", shmid);

    char *data = shmat(shmid, NULL, SHM_RDONLY);

    if (data == (char *)-1) {
        perror("[ERROR] shmat");
        return EXIT_FAILURE;
    }

    printf("[4] shmat() succeeded\n");
    printf("    address = %p\n", (void *)data);

    printf("[5] Reading shared memory...\n");
    printf("    data = %s\n", data);

    if (shmdt(data) == -1) {
        perror("[ERROR] shmdt");
        return EXIT_FAILURE;
    }

    printf("[6] shmdt() succeeded\n");

    return EXIT_SUCCESS;
}
