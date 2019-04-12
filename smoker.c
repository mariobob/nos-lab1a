#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "utilities.h"

void init_smoker(int smoker_id, int table_queue_id);
void endlessly_take_ingredients(int smoker_id, int table_queue_id);
    void ask_to_smoke(int smoker_id, int table_queue_id);
    void receive_confirmation(int smoker_id, int table_queue_id);
    void get_ingredients(int smoker_id, int table_queue_id);
    void roll_cigarette(int smoker_id);
    void done_rolling(int smoker_id, int table_queue_id);
    void smoke(int smoker_id);

// === INTRO ===
// Each smoker uses 4 different message types to communicate with seller,
// where each message type is derived from the base ingredient for that smoker.
//
// === QUEUE message types ===
// ask_to_smoke:
//   want mtype: ingredient + 3 + 0   (e.g. paper-smoker: 4)
// receive_confirmation:
//   mtype: ingredient + 3 + 3 + 3    (e.g. paper-smoker: 10)
// get_ingredients:
//   mtype: ingredient                (e.g. paper-smoker: 1)
// done_rolling:
//   done mtype: ingredient + 3 + 3   (e.g. paper-smoker: 7)
int main(int argc, char *argv[]) {
    if (argc != 3) {
        perror("Need 2 arguments: smoker_id table_queue_id\n");
        exit(EXIT_FAILURE);
    }

    int smoker_id = parse_int(argv[1]);
    int table_queue_id = parse_int(argv[2]);
    init_smoker(smoker_id, table_queue_id);
}

// starts the smoker function
void init_smoker(int smoker_id, int table_queue_id) {
    endlessly_take_ingredients(smoker_id, table_queue_id);
}

// loops forever, taking ingredients from the seller
void endlessly_take_ingredients(int smoker_id, int table_queue_id) {
    while (repeat()) {
        // Send request
        ask_to_smoke(smoker_id, table_queue_id);

        // Wait for confirmation
        receive_confirmation(smoker_id, table_queue_id);

        // CRITICAL SECTION
        get_ingredients(smoker_id, table_queue_id);

        roll_cigarette(smoker_id);

        done_rolling(smoker_id, table_queue_id);
        // END CRITICAL SECTION

        smoke(smoker_id);
    }
}

// sends smoke request to the message queue
void ask_to_smoke(int smoker_id, int table_queue_id) {
    long mtype = smoker_id + STATUS_WANT_SMOKE;
    printfw("[Smoker%d/%s] Can I smoke? I only have %s.\n", smoker_id, mts(mtype),
            ingredient(smoker_id));

    smokemsg msg = {mtype, ""};
    if (msgsnd(table_queue_id, (struct msgbuf *)&msg, strlen(msg.mtext) + 1, 0) == -1) {
        smokerror(smoker_id, "Failed to request ingredients");
        exit(EXIT_FAILURE);
    }
}

// receives smoke approval from the message queue
void receive_confirmation(int smoker_id, int table_queue_id) {
    long mtype = smoker_id + SMOKE_CONFIRMATION;

    smokemsg msg;
    if (msgrcv(table_queue_id, (struct msgbuf *)&msg, sizeof(msg.mtext), mtype, 0) == -1) {
        smokerror(smoker_id, "Failed to receive smoke confirmation");
        exit(EXIT_FAILURE);
    }

    printfw("  [Smoker%d/%s] Received confirmation.\n", smoker_id, mts(mtype));
}

// takes ingredients off the message queue
void get_ingredients(int smoker_id, int table_queue_id) {
    long mtype = smoker_id;

    smokemsg msg;
    if (msgrcv(table_queue_id, (struct msgbuf *)&msg, sizeof(msg.mtext), mtype, 0) == -1) {
        smokerror(smoker_id, "Failed to receive smoke ingredients");
        exit(EXIT_FAILURE);
    }

    smokebuf *smoke_buf = parse_smoke_buffer(&msg);
    printfw("  [Smoker%d/%s] Took %s and %s from the table.\n", smoker_id, mts(mtype),
            ingredient(smoke_buf->ingr1), ingredient(smoke_buf->ingr2));
    free(smoke_buf);
}

// rolls the cigarette
void roll_cigarette(int smoker_id) {
    printfw("  [Smoker%d]    Thanks! Rolling cigarette...\n", smoker_id);
}

// sends smoke done signal to the message queue
void done_rolling(int smoker_id, int table_queue_id) {
    long mtype = smoker_id + STATUS_DONE_SMOKE;
    printfw("  [Smoker%d/%s] Done rolling!\n", smoker_id, mts(mtype));

    smokemsg msg = {mtype, ""};
    if (msgsnd(table_queue_id, (struct msgbuf *)&msg, strlen(msg.mtext) + 1, 0) == -1) {
        smokerror(smoker_id, "Failed to signal completion of rolling");
        exit(EXIT_FAILURE);
    }
}

// smokes the rolled cigarette
void smoke(int smoker_id) {
    printfw("  [Smoker%d]    Smoking...\n", smoker_id);
}
