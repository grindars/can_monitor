#ifndef __FREERTOS_CONFIG__H__
#define __FREERTOS_CONFIG__H__

#include <stm32f10x.h>
#include <stdlib.h>

#define configUSE_PREEMPTION        1
#define configUSE_IDLE_HOOK         1
#define configMAX_PRIORITIES        ( ( unsigned portBASE_TYPE ) 3 )
#define config_MAX_CO_ROUTINE_PRIORITIES 1
#define configUSE_TICK_HOOK         0
#define configCPU_CLOCK_HZ          ( ( unsigned long ) 32000000 )
#define configTICK_RATE_HZ          ( ( portTickType ) 1000 )
#define configMINIMAL_STACK_SIZE    ( ( unsigned short ) 100 )
#define configTOTAL_HEAP_SIZE       ( ( size_t ) ( 2560 ) )
#define configMAX_TASK_NAME_LEN     ( 12 )
#define configUSE_TRACE_FACILITY    0
#define configUSE_16_BIT_TICKS      0
#define configIDLE_SHOULD_YIELD     0
#define configUSE_CO_ROUTINES       0
#define configUSE_MUTEXES           1
#define configUSE_RECURSIVE_MUTEXES 1
#define configUSE_COUNTING_SEMAPHORES 1
#define configCHECK_FOR_STACK_OVERFLOW 0
#define configUSE_QUEUE_SETS           1
#define configUSE_TIMERS               0

#define configPRIO_BITS       __NVIC_PRIO_BITS
#define configKERNEL_INTERRUPT_PRIORITY     ( 31 << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( 5 << (8 - configPRIO_BITS) )

#define INCLUDE_xSemaphoreGetMutexHolder 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelete  1
#define INCLUDE_xTaskGetIdleTaskHandle 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskDelay      1
#define INCLUDE_eTaskGetState   1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_xTaskResumeFromISR 1
#define INCLUDE_pcTaskGetTaskName 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_xTaskGetSchedulerState 1

static inline void *pvPortMalloc(size_t size) { return malloc(size); }
static inline void vPortFree(void *ptr) { return free(ptr); }

#endif
