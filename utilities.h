#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#ifndef NOS_LAB1A_STRUCTURES_H
#define NOS_LAB1A_STRUCTURES_H


#define TRUE 1
#define FALSE 0
#define NUM_INGREDIENTS 3

#define PAPER 1
#define TOBACCO 2
#define MATCH 3

#define STATUS_WANT_SMOKE NUM_INGREDIENTS
#define STATUS_DONE_SMOKE (NUM_INGREDIENTS*2)
#define SMOKE_CONFIRMATION (NUM_INGREDIENTS*3)

#define READABILITY_WAIT_MILLIS 100L
#define REPEAT_LOOPS TRUE

typedef int bool;

typedef struct smoke_buffer {
    long mtype;
    int ingr1;
    int ingr2;
} smokebuf;

typedef struct smoke_message {
    long mtype;
    char mtext[20];
} smokemsg;


// returns a parsed buffer with mtype, ingr1 and ingr2
smokebuf* parse_smoke_buffer(smokemsg* msg) {
    smokebuf *smoke_buf = (smokebuf*) malloc(sizeof(smokebuf));
    if (smoke_buf == NULL) return NULL;

    smoke_buf->mtype = msg->mtype;
    sscanf(msg->mtext, "%d:%d", &smoke_buf->ingr1, &smoke_buf->ingr2);
    return smoke_buf;
}

// returns a serialized buffer with mtype and text
smokemsg* serialize_smoke_buffer(smokebuf* smoke_buf) {
    smokemsg *msg = (smokemsg*) malloc(sizeof(smokemsg));
    if (msg == NULL) return NULL;

    msg->mtype = smoke_buf->mtype;
    snprintf(msg->mtext, sizeof(msg->mtext), "%d:%d", smoke_buf->ingr1, smoke_buf->ingr2);
    return msg;
}

// returns the name of this ingredient
char* ingredient(int ingr) {
    if (ingr == PAPER) {
        return "PAPER";
    } else if (ingr == TOBACCO) {
        return "TOBACCO";
    } else if (ingr == MATCH) {
        return "MATCHES";
    } else {
        return "nothing";
    }
}

// returns the third ingredient
int missing_ingredient(int ingr1, int ingr2) {
    if ((ingr1 == PAPER && ingr2 == TOBACCO) || (ingr1 == TOBACCO && ingr2 == PAPER)) {
        return MATCH;
    } else if ((ingr1 == TOBACCO && ingr2 == MATCH) || (ingr1 == MATCH && ingr2 == TOBACCO)) {
        return PAPER;
    } else if ((ingr1 == MATCH && ingr2 == PAPER) || (ingr1 == PAPER && ingr2 == MATCH)) {
        return TOBACCO;
    } else {
        return -1;
    }
}

// message type string (returns a 2-digit string always)
char* mts(long mtype) {
    size_t size = 3;
    char *str = (char*) malloc(size);
    snprintf(str, size, "%02ld", mtype);
    return str;
}

// converts string to integer
int parse_int(const char *str) {
    return atoi(str);
}

// prints error message prefixed by smoker id
void smokerror(int smoker_id, const char *message) {
    fprintf(stderr, "  [Smoker%d]    ", smoker_id);
    perror(message);
}

// waits a little so that outputs make sense
const struct timespec readability_wait = {0, READABILITY_WAIT_MILLIS * 1000000L};
void wait_to_read() {
    nanosleep(&readability_wait, NULL);
}

// does a printf and waits a little
int printfw(const char *format, ...) {
    va_list argp;
    va_start(argp, format);
    vprintf(format, argp);
    va_end(argp);
    wait_to_read();
}

// returns TRUE if communication should be endless
bool repeat() {
    return REPEAT_LOOPS;
}


#endif //NOS_LAB1A_STRUCTURES_H
