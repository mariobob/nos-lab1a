#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define FORK_WAIT_SECS 1

int table_queue_id;

void fork_seller(char *table_id_str);
void fork_smoker(int smoker_id, char *table_id_str);

// creates new or gets existing message queue
int get_or_create_queue() {
    key_t key = getuid();
    int id = msgget(key, 0600 | IPC_CREAT);
    if (id == -1) {
        perror("[Main] Error while creating table queue (msgget)");
        exit(EXIT_FAILURE);
    }

    printf("[Main] Created table queue with id %d.\n", id);
    return id;
}

// deletes the message queue
void cleanup(int sig) {
    if (msgctl(table_queue_id, IPC_RMID, NULL) == -1) {
        perror("[Main] Error while cleaning table queue (msgctl)");
        exit(EXIT_FAILURE);
    }

    printf("\n[Main] Table is broken. No more smoking.\n");
    exit(EXIT_SUCCESS);
}

// creates message queue
// starts seller process
// starts three smoker processes
int main(int argc, char *argv[]) {
    table_queue_id = get_or_create_queue();
    char table_id_str[20];
    snprintf(table_id_str, sizeof(table_id_str), "%d", table_queue_id);

    signal(SIGINT, cleanup);

    // Start one seller
    fork_seller(&table_id_str[0]);

    // Start three smokers
    fork_smoker(1, &table_id_str[0]);
    fork_smoker(2, &table_id_str[0]);
    fork_smoker(3, &table_id_str[0]);

    while (1);
}

// creates a seller process
void fork_seller(char *table_id_str) {
    char *filename = "./seller";
    char *argv[] = {"seller", table_id_str, NULL};

    switch (fork()) {
        case -1:
            printf("[Main] Could not fork seller.\n");
            break;
        case 0:
            printf("[Main] Forking seller...\n");
            execv(filename, argv);
            exit(EXIT_SUCCESS);
        default:
            sleep(FORK_WAIT_SECS);
    }
}

// creates a smoker process
void fork_smoker(int smoker_id, char *table_id_str) {
    char smoker_id_str[3];
    snprintf(smoker_id_str, sizeof(smoker_id_str), "%d", smoker_id);

    char *filename = "./smoker";
    char *argv[] = {"smoker", smoker_id_str, table_id_str, NULL};

    switch (fork()) {
        case -1:
            printf("Could not fork smoker%d.\n", smoker_id);
            break;
        case 0:
            printf("[Main] Forking smoker%d...\n", smoker_id);
            sleep(FORK_WAIT_SECS);
            execv(filename, argv);
            exit(EXIT_SUCCESS);
    }
}
