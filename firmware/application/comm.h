#ifndef __COMM__H__
#define __COMM__H__

#include <FreeRTOS.h>
#include <semphr.h>

#include <string.h>

#define COMM_MAX_DATA_SIZE      32
#define COMM_RECEIVE_QUEUE_SIZE 2

#define COMM_OK                  0
#define COMM_ERR_OUT_OF_MEMORY  -1
#define COMM_ERR_TIMEOUT        -2
#define COMM_TOO_BIG            -3

#define TYPE_ACK    0
#define TYPE_DEBUG  1
#define TYPE_PING   2
#define TYPE_PONG   3
#define TYPE_CAN_CONTROL 4
#define TYPE_CAN_MESSAGE 5

typedef struct comm_print_context {
    int used;
    char buffer[COMM_MAX_DATA_SIZE];
} comm_print_context_t;

typedef struct comm_send_op {
    struct comm_send_op *next;

    const void *data;
    size_t dataSize;
    xSemaphoreHandle semaphore;
    int status;

    portTickType lastRetriedAt;
    unsigned short serial; 
    unsigned char retries;
    unsigned char type;
} comm_send_op_t;

typedef struct comm_received_message {
    unsigned char type;
    unsigned char size;
    unsigned char data[COMM_MAX_DATA_SIZE];
} comm_received_message_t;

void commInit(void);

xQueueHandle commReceiveQueue(void);
int commSend(unsigned char type, const void *data, size_t dataSize);

int commPrint(const char *fmt, ...);

#endif
