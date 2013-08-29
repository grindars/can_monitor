#ifndef __CAN__H__
#define __CAN__H__

#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

#define CAN_RECEIVE_QUEUE_SIZE  8
#define CAN_ENABLED             1
#define CAN_SILENT              2
#define CAN_LOOPBACK            4

typedef struct can_message {
    unsigned int address;
    unsigned int timestamp_length;
    unsigned int data_low;
    unsigned int data_high;
} can_message_t;

typedef struct can_send_op {
    const can_message_t *message;
    xSemaphoreHandle semaphore;
} can_send_op_t;

void canInit(void);
void canControl(unsigned char mode);
void canSend(const can_message_t *message);
xQueueHandle canReceiveQueue(void);

#endif
