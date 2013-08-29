#include <FreeRTOS.h>
#include <task.h>

#include <stdio.h>

#include "comm.h"
#include "tty.h"

#define RETRY_TIMEOUT   200
#define MAX_RETRIES     4
#define START_OF_FRAME  0x7F

#define XON_TRANSMIT    1
#define XON_RECEIVE     2
#define ESCAPE_RECEIVED 4

#define QUEUE_SIZE      128
#define XOFF_THRESHOLD  64
#define XON_THRESHOLD   0

#define ESCAPE_MASK  0x20
#define ESCAPE       0x10
#define XON          0x11
#define XOFF         0x13

#define STATE_SOF           0
#define STATE_TYPE          1
#define STATE_DATA_SIZE     2
#define STATE_SERIAL_LOW    3
#define STATE_SERIAL_HIGH   4
#define STATE_DATA          5

static xQueueHandle sendQueue, receiveQueue, uartTransmitQueue, uartReceiveQueue;
static xSemaphoreHandle commTaskWake;
static comm_send_op_t *sendOps;
static unsigned short opSerial;
static unsigned char uartFlags;
static unsigned char commState = STATE_SOF, commMessageType, commMessageDataSize, commMessageDataPointer;
static unsigned short commMessageSerial;
static unsigned char commMessageData[COMM_MAX_DATA_SIZE];

static void uartInit(void);
static void uartSend(unsigned char byte);
static int uartReceive(unsigned char *byte);

static void transmit(comm_send_op_t *op) {
    uartSend(START_OF_FRAME);
    uartSend(op->type);
    uartSend(op->dataSize);
    uartSend(op->serial & 0xFF);
    uartSend(op->serial >> 8);
    for(unsigned int i = 0; i < op->dataSize; i++)
        uartSend(((unsigned char *) op->data)[i]);
}

xQueueHandle commReceiveQueue(void) {
    return receiveQueue;
}

static void commDispatchMessage(void) {
    if(commMessageType == TYPE_ACK) {
        for(comm_send_op_t *op = sendOps, *prev = NULL, *next; op; prev = op, op = next) {
            next = op->next;

            if(op->serial == commMessageSerial) {
                if(prev)
                    prev->next = next;
                else
                    sendOps = next;

                op->status = 0;
                xSemaphoreGive(op->semaphore);    

                break;          
            }
        }
    } else {
        comm_received_message_t message;
        message.type = commMessageType;
        message.size = commMessageDataSize;
        memcpy(message.data, commMessageData, commMessageDataSize);
        if(xQueueSendToBack(receiveQueue, &message, 0) != pdTRUE)
            return;

        uartSend(START_OF_FRAME);
        uartSend(TYPE_ACK);
        uartSend(0);
        uartSend(commMessageSerial & 0xFF);
        uartSend(commMessageSerial >> 8);
    }
}

static void commTask(void *arg) {
    (void) arg;

    comm_send_op_t *sendOp;
    unsigned char byte;

    while(1) {
        xSemaphoreTake(commTaskWake, RETRY_TIMEOUT / 2 / portTICK_RATE_MS);

        portTickType now = xTaskGetTickCount();

        while(xQueueReceive(sendQueue, &sendOp, 0) == pdTRUE) {
            sendOp->next = sendOps;
            sendOps = sendOp;

            sendOp->serial  = opSerial++;
            sendOp->retries = 0;
            sendOp->lastRetriedAt = now;
            transmit(sendOp);
        }

        while(uartReceive(&byte)) {
            switch(commState) {
                case STATE_SOF:
                    if(byte == START_OF_FRAME)
                        commState = STATE_TYPE;

                    break;

                case STATE_TYPE:
                    commMessageType = byte;
                    commState = STATE_DATA_SIZE;

                    break;

                case STATE_DATA_SIZE:
                    commMessageDataSize = byte;
                    commState = STATE_SERIAL_LOW;

                    break;

                case STATE_SERIAL_LOW:
                    commMessageSerial = byte;
                    commState = STATE_SERIAL_HIGH;

                    break;

                case STATE_SERIAL_HIGH:
                    commMessageSerial |= byte << 8;
                    commMessageDataPointer = 0;

                    if(commMessageDataSize == 0) {
                        commState = STATE_SOF;
                        commDispatchMessage();
                    } else
                        commState = STATE_DATA;

                    break;

                case STATE_DATA:
                    if(commMessageDataPointer < sizeof(commMessageData))
                        commMessageData[commMessageDataPointer] = byte;

                    commMessageDataPointer++;
                    if(commMessageDataPointer == commMessageDataSize) {
                        commState = STATE_SOF;
                        if(commMessageDataSize > sizeof(commMessageData))
                            commMessageDataSize = sizeof(commMessageData);

                        commDispatchMessage();
                    }

                    break;
            }
        }

        for(comm_send_op_t *op = sendOps, *prev = NULL, *next; op; prev = op, op = next) {
            next = op->next;

            if(now - op->lastRetriedAt > RETRY_TIMEOUT / portTICK_RATE_MS) {
                if(op->retries == MAX_RETRIES) {
                    if(prev)
                        prev->next = next;
                    else
                        sendOps = next;

                    op->status = COMM_ERR_TIMEOUT;
                    xSemaphoreGive(op->semaphore);                    
                } else {
                    op->lastRetriedAt = now;
                    op->retries++;
                    transmit(op);
                }
            }
        }
    }
}

void commInit(void) {
    xTaskHandle handle;

    uartInit();

    sendQueue = xQueueCreate(2, sizeof(comm_send_op_t *));
    receiveQueue = xQueueCreate(COMM_RECEIVE_QUEUE_SIZE, sizeof(comm_received_message_t));

    vSemaphoreCreateBinary(commTaskWake);

    xTaskCreate(commTask, (const signed char *) "COMM", 64, NULL, tskIDLE_PRIORITY + 2, &handle);
}

int commSend(unsigned char type, const void *data, size_t dataSize) {
    if(dataSize > COMM_MAX_DATA_SIZE)
        return COMM_TOO_BIG;

    comm_send_op_t op, *ptr;
    op.type = type;
    op.data = data;
    op.dataSize = dataSize;

    ptr = &op;

    vSemaphoreCreateBinary(op.semaphore);
    if(op.semaphore == NULL)
        return COMM_ERR_OUT_OF_MEMORY;
    
    xSemaphoreTake(op.semaphore, 0);

    xQueueSendToBack(sendQueue, &ptr, portMAX_DELAY);
    xSemaphoreGive(commTaskWake);
    xSemaphoreTake(op.semaphore, portMAX_DELAY);

    vSemaphoreDelete(op.semaphore);

    return op.status;
}

static void commPrintFlush(comm_print_context_t *context) {
    if(context->used > 0)
        commSend(TYPE_DEBUG, context->buffer, context->used);

    context->used = 0;
}

static void commPrintCallback(char ch, void *arg) {
    comm_print_context_t *context = arg;

    if(context->used == sizeof(context->buffer))
        commPrintFlush(context);

    context->buffer[context->used++] = ch;
    
    if(ch == '\n')
        commPrintFlush(context);
}

int commPrint(const char *fmt, ...) {
    comm_print_context_t context = { .used = 0 };

    va_list list;
    va_start(list, fmt);

    int ret = vcprintf(fmt, list, commPrintCallback, &context);

    va_end(list);

    commPrintFlush(&context);

    return ret;
}

static void uartTransmitReceive(void) {
    USART1->CR1 = USART_CR1_UE | USART_CR1_TXEIE | USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE;
}

static void uartReceiveOnly(void) {
    USART1->CR1 = USART_CR1_UE | USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE;
}

static void uartInit(void) {
    uartTransmitQueue = xQueueCreate(QUEUE_SIZE, 1);
    uartReceiveQueue  = xQueueCreate(QUEUE_SIZE, 1);

    uartFlags = XON_TRANSMIT | XON_RECEIVE;

    USART1->BRR = SystemCoreClock / 460800;
    uartReceiveOnly();
    NVIC_SetPriority(USART1_IRQn, 6);
    NVIC_EnableIRQ(USART1_IRQn);
}

static void uartSend(unsigned char byte) {
    if(byte == XON || byte == XOFF || byte == ESCAPE) {
        unsigned char escape = ESCAPE;

        xQueueSendToBack(uartTransmitQueue, &escape, portMAX_DELAY);
        byte ^= ESCAPE_MASK;
    }

    xQueueSendToBack(uartTransmitQueue, &byte, portMAX_DELAY);
    uartTransmitReceive();
}

static int uartReceive(unsigned char *byte) {
    if(xQueueReceive(uartReceiveQueue, byte, 0) == pdFALSE)
        return 0;
    else {
        if(uxQueueMessagesWaiting(uartReceiveQueue) <= XON_THRESHOLD && !(uartFlags & XON_RECEIVE))
            uartTransmitReceive();
                
        return 1;
    }
}

void USART1_IRQHandler(void) {
    unsigned char status = USART1->SR;
    unsigned char stopTransmit = 0;
    unsigned char byte;
    portBASE_TYPE woken = pdFALSE;

    unsigned int used;

    if(status & USART_SR_TXE) {
        if(uartFlags & XON_TRANSMIT) {
            if(xQueueReceiveFromISR(uartTransmitQueue, &byte, &woken) == pdTRUE)
                USART1->DR = byte;
            else
                stopTransmit = 1;
        } else
            stopTransmit = 1;
    }

    if(status & USART_SR_RXNE) {
        byte = USART1->DR;

        if(byte == ESCAPE) {
            uartFlags |= ESCAPE_RECEIVED;

        } else if(byte == XON) {
            uartFlags |= XON_TRANSMIT;
            stopTransmit = 0;
            uartTransmitReceive();

        } else if(byte == XOFF) {
            uartFlags &= ~XON_TRANSMIT;
            stopTransmit = 1;

        } else if(uartFlags & ESCAPE_RECEIVED) {
            uartFlags &= ~ESCAPE_RECEIVED;
            byte ^= ESCAPE_MASK;
            if(xQueueSendToBackFromISR(uartReceiveQueue, &byte, &woken) == pdTRUE)
                xSemaphoreGiveFromISR(commTaskWake, &woken);


        } else {
            if(xQueueSendToBackFromISR(uartReceiveQueue, &byte, &woken) == pdTRUE)
                xSemaphoreGiveFromISR(commTaskWake, &woken);
        }
    }

    used = uxQueueMessagesWaiting(uartReceiveQueue);

    if(uartFlags & XON_RECEIVE) {
        if(used >= XOFF_THRESHOLD && (uartFlags & XON_TRANSMIT)) {
            byte = XOFF;
            if(xQueueSendToFrontFromISR(uartTransmitQueue, &byte, &woken) == pdTRUE) {
                uartFlags &= ~XON_RECEIVE;
                uartTransmitReceive();
            }
        }
    } else {
        if(used <= XON_THRESHOLD && (uartFlags & XON_TRANSMIT))
            byte = XON;
            if(xQueueSendToFrontFromISR(uartTransmitQueue, &byte, &woken) == pdTRUE) {
                uartFlags |= XON_RECEIVE;
                uartTransmitReceive();
            }
    }

    if(stopTransmit)
        uartReceiveOnly();

    portYIELD_FROM_ISR(woken);
}
