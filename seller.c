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

#define REGENERATE_WAIT_SECS 5

void init_seller(int table_queue_id);
void endlessly_offer_ingredients(int table_queue_id);
    void generate_random_ingredients(int *ingadr1, int *ingadr2);
        int get_random_ingredient();
    int determine_msgtype(int ingr1, int ingr2);
    void put_ingredients(int table_queue_id, smokebuf *smoke_buf);
    void receive_message(int table_queue_id, smokebuf *smoke_buf, int expected_status);
    void send_confirmation(int table_queue_id, smokebuf *smoke_buf);

// === INTRO ===
// Seller uses 4 different message types for EACH smoker, to communicate.
// Each message type is derived from the base ingredient for that smoker.
//
// === QUEUE message types ===
// put_ingredients:
//   mtype: ingredient                (e.g. paper-smoker: 1)
// receive_message:
//   want mtype: ingredient + 3 + 0   (e.g. paper-smoker: 4)
//   done mtype: ingredient + 3 + 3   (e.g. paper-smoker: 7)
// send_confirmation:
//   mtype: ingredient + 3 + 3 + 3    (e.g. paper-smoker: 10)
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Need 1 argument: table_queue_id\n");
        exit(EXIT_FAILURE);
    }

    int table_queue_id = parse_int(argv[1]);
    init_seller(table_queue_id);
}

// starts the seller function
void init_seller(int table_queue_id) {
    endlessly_offer_ingredients(table_queue_id);
}

// loops forever, providing smokers with their ingredients
void endlessly_offer_ingredients(int table_queue_id) {
    smokebuf *smoke_buf = (smokebuf*) malloc(sizeof(smokebuf));
    int ingredient1;
    int ingredient2;

    srand((unsigned) time(NULL));
    while (repeat()) {
        // Pick two different ingredients randomly
        generate_random_ingredients(&ingredient1, &ingredient2);
        printfw("[Seller] Pulled out some brand new ingredients: %s and %s\n",
               ingredient(ingredient1), ingredient(ingredient2));

        // Create message with ingredients
        smoke_buf->mtype = determine_msgtype(ingredient1, ingredient2);
        smoke_buf->ingr1 = ingredient1;
        smoke_buf->ingr2 = ingredient2;

        // Send message
        printfw("  [Seller]    Putting ingredients on the table...\n");
        put_ingredients(table_queue_id, smoke_buf);

        printfw("  [Seller]    Waiting for smoke request...\n");
        receive_message(table_queue_id, smoke_buf, STATUS_WANT_SMOKE);

        // CRITICAL SECTION
        send_confirmation(table_queue_id, smoke_buf);

        printfw("  [Seller]    No one else touches the table.\n");
        receive_message(table_queue_id, smoke_buf, STATUS_DONE_SMOKE);
        // END CRITICAL SECTION

        sleep(REGENERATE_WAIT_SECS);
    }

    free(smoke_buf);
}

// returns a random ingredient, out of available ingredients
int get_random_ingredient() {
    return rand() % NUM_INGREDIENTS + 1;
}

// generates two different ingredients and stores them
void generate_random_ingredients(int *ingadr1, int *ingadr2) {
    *ingadr1 = get_random_ingredient();
    do {
        *ingadr2 = get_random_ingredient();
    } while (*ingadr2 == *ingadr1);
}

// returns message type based on the two ingredients
int determine_msgtype(int ingr1, int ingr2) {
    int msgtype = missing_ingredient(ingr1, ingr2);
    if (msgtype == -1) {
        printfw("  [Seller]    Got ingredients for unknown smoker?\n");
    }
    return msgtype;
}

// sends ingredients to to message queue, uses msgtype from buffer
void put_ingredients(int table_queue_id, smokebuf *smoke_buf) {
    smokemsg *msg = serialize_smoke_buffer(smoke_buf);
    if (msgsnd(table_queue_id, (struct msgbuf *)msg, strlen(msg->mtext) + 1, 0) == -1) {
        perror("  [Seller]    Failed to put ingredients on the table");
        exit(EXIT_FAILURE);
    }
    free(msg);

    printfw("  [Seller/%s] Ingredients for smoker%ld are on the table.\n", mts(smoke_buf->mtype), smoke_buf->mtype);
}

// receives a smoke request or smoke done signal from the appropriate smoker
void receive_message(int table_queue_id, smokebuf *smoke_buf, int expected_status) {
    long mtype = smoke_buf->mtype + expected_status; // (1|2|3) + (3|6)

    smokemsg msg;
    if (msgrcv(table_queue_id, (struct msgbuf *)&msg, sizeof(msg.mtext), mtype, 0) == -1) {
        perror("  [Seller]    Failed to receive smoke message");
        exit(EXIT_FAILURE);
    }

    if (expected_status == STATUS_WANT_SMOKE) {
        printfw("  [Seller/%s] Got request from smoker%ld.\n", mts(mtype), smoke_buf->mtype);
    } else if (expected_status == STATUS_DONE_SMOKE) {
        printfw("  [Seller/%s] Ok. Acquiring more ingredients...\n", mts(mtype));
    }
}

// sends smoke approval to the appropriate smoker
void send_confirmation(int table_queue_id, smokebuf *smoke_buf) {
    long mtype = smoke_buf->mtype + SMOKE_CONFIRMATION;
    smokemsg msg = {mtype, ""};

    if (msgsnd(table_queue_id, (struct msgbuf *)&msg, strlen(msg.mtext) + 1, 0) == -1) {
        perror("  [Seller]    Failed to send confirmation");
        exit(EXIT_FAILURE);
    }

    printfw("  [Seller/%s] You may smoke, smoker%ld.\n", mts(mtype), smoke_buf->mtype);
}
