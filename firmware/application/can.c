#include <FreeRTOS.h>
#include <task.h>

#include "can.h"

static xQueueHandle transmitQueue, receiveQueue, controlQueue;
static xSemaphoreHandle canWake;
static can_send_op_t *canTX[3];

static void canPutMessage(can_send_op_t *op) {
    unsigned int code = (CAN1->TSR & CAN_TSR_CODE) >> 24;
    canTX[code] = op;

    CAN1->sTxMailBox[code].TDHR = op->message->data_high;
    CAN1->sTxMailBox[code].TDLR = op->message->data_low;
    CAN1->sTxMailBox[code].TDTR = op->message->timestamp_length;
    CAN1->sTxMailBox[code].TIR  = op->message->address | CAN_TI0R_TXRQ;
}

void CAN1_RX1_IRQHandler(void) {
    portBASE_TYPE woken = pdFALSE;

    while((CAN1->RF1R & CAN_RF1R_FMP1) != 0) {
        can_message_t message;
        message.address          = CAN1->sFIFOMailBox[1].RIR;
        message.timestamp_length = CAN1->sFIFOMailBox[1].RDTR;
        message.data_low         = CAN1->sFIFOMailBox[1].RDLR;
        message.data_high        = CAN1->sFIFOMailBox[1].RDHR;
        
        xQueueSendToBackFromISR(receiveQueue, &message, &woken);        

        CAN1->RF1R = CAN_RF1R_RFOM1;
    }

    portYIELD_FROM_ISR(woken); 
}

void USB_LP_CAN1_RX0_IRQHandler(void) {
    portBASE_TYPE woken = pdFALSE;

    while((CAN1->RF0R & CAN_RF0R_FMP0) != 0) {
        can_message_t message;
        message.address          = CAN1->sFIFOMailBox[0].RIR;
        message.timestamp_length = CAN1->sFIFOMailBox[0].RDTR;
        message.data_low         = CAN1->sFIFOMailBox[0].RDLR;
        message.data_high        = CAN1->sFIFOMailBox[0].RDHR;
        
        xQueueSendToBackFromISR(receiveQueue, &message, &woken);        

        CAN1->RF0R = CAN_RF0R_RFOM0;
    }

    portYIELD_FROM_ISR(woken);    
}

void USB_HP_CAN1_TX_IRQHandler(void) {
    can_send_op_t *sendOp;
    portBASE_TYPE woken = pdFALSE;

    if(CAN1->TSR & CAN_TSR_RQCP2) {
        CAN1->TSR = CAN_TSR_RQCP2;

        if(canTX[2] != NULL) {
            xSemaphoreGiveFromISR(canTX[2]->semaphore, &woken);
        }

        canTX[2] = NULL;
    }

    if(CAN1->TSR & CAN_TSR_RQCP1) {
        CAN1->TSR = CAN_TSR_RQCP1;

        if(canTX[1] != NULL) {
            xSemaphoreGiveFromISR(canTX[1]->semaphore, &woken);
        }

        canTX[1] = NULL;
    }

    if(CAN1->TSR & CAN_TSR_RQCP0) {
        CAN1->TSR = CAN_TSR_RQCP0;

        if(canTX[0] != NULL) {
            xSemaphoreGiveFromISR(canTX[0]->semaphore, &woken);
        }

        canTX[0] = NULL;
    }

    while(xQueuePeekFromISR(transmitQueue, &sendOp) == pdTRUE) {   
        if((CAN1->TSR & CAN_TSR_TME) == 0)
            break;

        xQueueReceiveFromISR(transmitQueue, &sendOp, &woken);
        canPutMessage(sendOp);
    }

    portYIELD_FROM_ISR(woken);
}

static void canTask(void *arg) {
    (void) arg;

    unsigned char control;
    can_send_op_t *sendOp;

    while(1) {
        xSemaphoreTake(canWake, portMAX_DELAY);

        while(xQueueReceive(controlQueue, &control, 0) == pdTRUE) {
            if(control & CAN_ENABLED) {
                CAN1->MCR &= ~CAN_MCR_SLEEP;
                while(CAN1->MSR & CAN_MSR_SLAK);

                CAN1->MCR |= CAN_MCR_INRQ;
                while(!(CAN1->MSR & CAN_MSR_INAK));

                unsigned int btr = (1 << 16) | (2 << 20) | (12 - 1);

                if(control & CAN_SILENT)
                    btr |= CAN_BTR_SILM;

                if(control & CAN_LOOPBACK)
                    btr |= CAN_BTR_LBKM;

                CAN1->BTR = btr;
                CAN1->IER = CAN_IER_FMPIE1 | CAN_IER_FMPIE0 | CAN_IER_TMEIE;
                CAN1->MCR |= CAN_MCR_ABOM | CAN_MCR_NART;

                CAN1->FMR |= CAN_FMR_FINIT;
                CAN1->FA1R = CAN_FA1R_FACT0;
                CAN1->sFilterRegister[0].FR1 = 0;
                CAN1->sFilterRegister[0].FR2 = 0;
                CAN1->FMR &= ~CAN_FMR_FINIT;

                CAN1->MCR &= ~CAN_MCR_INRQ;
                while(CAN1->MSR & CAN_MSR_INAK);                
                
            } else {
                CAN1->MCR |= CAN_MCR_SLEEP;
                while(!(CAN1->MSR & CAN_MSR_SLAK));
            }
        }

        while(xQueuePeek(transmitQueue, &sendOp, 0) == pdTRUE) {
            NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);

            if((CAN1->TSR & CAN_TSR_TME) == 0) {
                NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);

                break;
            }

            xQueueReceive(transmitQueue, &sendOp, 0);
            canPutMessage(sendOp);

            NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
        }
    }
}

void canInit(void) {
    xTaskHandle handle;

    NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, 6);
    NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);

    NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 6);
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

    NVIC_SetPriority(CAN1_RX1_IRQn, 6);
    NVIC_EnableIRQ(CAN1_RX1_IRQn);

    transmitQueue = xQueueCreate(2, sizeof(can_send_op_t *));
    receiveQueue  = xQueueCreate(CAN_RECEIVE_QUEUE_SIZE, sizeof(can_message_t));
    controlQueue  = xQueueCreate(2, sizeof(unsigned char));

    vSemaphoreCreateBinary(canWake);

    xTaskCreate(canTask, (const signed char *) "CAN", 64, NULL, tskIDLE_PRIORITY + 2, &handle);
}

void canSend(const can_message_t *message) {
    can_send_op_t op, *ptr = &op;
    op.message = message;
    vSemaphoreCreateBinary(op.semaphore);
    xSemaphoreTake(op.semaphore, 0);
    xQueueSendToBack(transmitQueue, &ptr, portMAX_DELAY);
    xSemaphoreGive(canWake);
    xSemaphoreTake(op.semaphore, portMAX_DELAY);
    vSemaphoreDelete(op.semaphore);
}

xQueueHandle canReceiveQueue(void) {
    return receiveQueue;
}

void canControl(unsigned char mode) {
    xQueueSendToBack(controlQueue, &mode, portMAX_DELAY);
    xSemaphoreGive(canWake);
}
