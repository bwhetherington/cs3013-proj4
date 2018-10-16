#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "message.h"

msg_t *MAILBOXES;
sem_t *SENDS;
sem_t *RECVS;

int NUM_MAILBOXES;

void initMsg(msg_t *dst, int source, int type, int start, int stop) {
    dst->source = source;
    dst->type = type;
    dst->start = start;
    dst->stop = stop;
}

/*
 * iTo: mailbox to send to
 * msg: message to be sent
 */
void sendMsg(int to, msg_t *src) {
    // Wait for a message to be sent
    sem_t *send = SENDS + to;
    sem_wait(send);

    // Copy data of message to mailbox with the specified ID
    MAILBOXES[to] = *src;

    // Allow another to be received from the mailbox
    sem_t *recv = RECVS + to;
    sem_post(recv);
}

/*
 * iTo: mailbox to receive from
 * msg: message struct to be filled in with received message
 */
void recvMsg(int from, msg_t *msg) {
    // Wait for a message to be received
    sem_t *recv = RECVS + from;
    sem_wait(recv);

    // Copy message from mailbox to specified address
    *msg = MAILBOXES[from];

    // Allow another to be sent to the mailbox
    sem_t *send = SENDS + from;
    sem_post(send);
}

void initMailboxes(int numMailboxes) {
    NUM_MAILBOXES = numMailboxes;
    MAILBOXES = malloc(sizeof (msg_t) * numMailboxes);
    SENDS = malloc(sizeof (sem_t) * numMailboxes);
    RECVS = malloc(sizeof (sem_t) * numMailboxes);

    for (int i = 0; i < numMailboxes; i++) {
        sem_init(SENDS + i, 0, 1);
        sem_init(RECVS + i, 0, 0);
    }
}

void cleanupMailboxes() {
    // Destroy semaphores
    for (int i = 0; i < NUM_MAILBOXES; i++) {
        sem_destroy(SENDS + i);
        sem_destroy(RECVS + i);
    }

    // Free arrays
    free(MAILBOXES);
    free(SENDS);
    free(RECVS);
}