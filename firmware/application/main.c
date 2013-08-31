#include <FreeRTOS.h>
#include <task.h>

#include <stdlib.h>

#include "io.h"
#include "heap.h"
#include "comm.h"
#include "tty.h"
#include "can.h"

#define BANNER_SHORT "CAN monitor\r\n"
#define BANNER       "CAN monitor starting up\nFirmware: 1.0\n"

static void abort(void) {
    while(1);
}

static void handleMessage(const comm_received_message_t *message) {
    switch(message->type) {
    case TYPE_PING:
        commSend(TYPE_PONG, message->data, message->size);
        break;

    case TYPE_PONG:
        break;

    case TYPE_CAN_MESSAGE:
        {
            can_message_t can;
            memcpy(&can, message->data, sizeof(can_message_t));
            canSend(&can);
        }

        break;

    case TYPE_CAN_CONTROL:
        {
            unsigned int control;
            memcpy(&control, message->data, sizeof(unsigned int));
            canControl(control);
        }

        break;
        
    default:
        commPrint("received message of type %hhu\n", message->type);
        break;
    }
}

static void pumpTask(void *arg) {
    unsigned volatile int *usedTime = arg;

    xQueueSetHandle workset = xQueueCreateSet(COMM_RECEIVE_QUEUE_SIZE + CAN_RECEIVE_QUEUE_SIZE);
    xQueueHandle commQueue = commReceiveQueue();
    xQueueAddToSet(commQueue, workset);

    xQueueHandle canQueue = canReceiveQueue();
    xQueueAddToSet(canQueue, workset);

    while(1) {
        xQueueHandle signaled = xQueueSelectFromSet(workset, portMAX_DELAY);

        unsigned int started = xTaskGetTickCount(); 

        if(signaled == commQueue) {
            comm_received_message_t message;
            xQueueReceive(commQueue, &message, 0);
            handleMessage(&message);
        } else if(signaled == canQueue) {
            can_message_t message;
            xQueueReceive(canQueue, &message, 0);

            commSend(TYPE_CAN_MESSAGE, &message, sizeof(can_message_t));
        }

        unsigned int finished = xTaskGetTickCount();

        *usedTime += finished - started;
    }
}

static void initTask(void *arg) {
    (void) arg;

    static const char bar[] = { '|', '-' };
    int barCounter = 0;
    unsigned volatile int usedTime = 0;

    xTaskHandle pumpHandle;

    if(initializeHeapLocks() == -1)
        abort();

    ttyInit();
    ttyPrint(BANNER_SHORT);
    
    commInit();
    canInit();
    commPrint(BANNER);

    xTaskCreate(pumpTask, (const signed char *) "PUMP", 128, (void *) &usedTime, tskIDLE_PRIORITY + 1, &pumpHandle);

    while(1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
        unsigned int elapsed = usedTime;
        usedTime = 0;

        ttyPrint("\r%c %3d%% busy", bar[barCounter++], elapsed * 100 / (1000 / portTICK_RATE_MS));

        if(barCounter == sizeof(bar))
            barCounter = 0;
    }
}

int main(void) {
    xTaskHandle initHandle;

    RCC->APB2ENR = RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN
                 | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN | RCC_APB2ENR_USART1EN;

    RCC->APB1ENR = RCC_APB1ENR_PWREN | RCC_APB1ENR_CAN1EN;

    io_init();

    initializeHeap();

    if(xTaskCreate(initTask, (const signed char *) "MONITOR", 128, NULL, tskIDLE_PRIORITY + 1, &initHandle) != pdPASS)
        abort();

    vTaskStartScheduler();

    return 0;
}

void vApplicationIdleHook(void) {
    __WFI();
}
