#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

int main(void)
{
    printf("[1] Starting writer...\n");

    /* Generate System V IPC key */
    key_t key = ftok("/tmp", 65);

    if (key == (key_t)-1) {
        perror("[ERROR] ftok");
        return EXIT_FAILURE;
    }

    printf("[2] ftok() succeeded\n");
    printf("    key = %d\n", key);

    /* Create or get shared memory segment */
    int shmid = shmget(key, 1024, 0666 | IPC_CREAT);

    if (shmid == -1) {
        perror("[ERROR] shmget");
        return EXIT_FAILURE;
    }

    printf("[3] shmget() succeeded\n");
    printf("    shmid = %d\n", shmid);

    /* Attach shared memory to process address space */
    char *data = shmat(shmid, NULL, 0);

    if (data == (char *)-1) {
        perror("[ERROR] shmat");
        
        /* Cleanup */
        shmctl(shmid, IPC_RMID, NULL);

        return EXIT_FAILURE;
    }

    printf("[4] shmat() succeeded\n");
    printf("    address = %p\n", (void *)data);

    /* Write data */
    const char *secret = "SECRET_API_KEY=PleaseSubscribe2026";

    strcpy(data, secret);

    printf("[5] Data written successfully\n");
    printf("    data = %s\n", data);

    printf("[6] Shared memory is ready\n");
    printf("    Sleeping for 60 seconds...\n");

    sleep(60);

    /* Detach */
    if (shmdt(data) == -1) {
        perror("[ERROR] shmdt");
    } else {
        printf("[7] shmdt() succeeded\n");
    }

    /* Remove shared memory segment */
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("[ERROR] shmctl(IPC_RMID)");
    } else {
        printf("[8] Shared memory removed\n");
    }

    printf("[9] Writer finished\n");

    return EXIT_SUCCESS;
}