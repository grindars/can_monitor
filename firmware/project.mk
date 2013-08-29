CROSS_COMPILE := ${ROOT}/../../probe/tools/bin/arm-none-eabi-
CC            := ${CROSS_COMPILE}gcc
CXX           := ${CROSS_COMPILE}g++
AR            := ${CROSS_COMPILE}ar
LD            := ${CROSS_COMPILE}ld
OBJCOPY       := ${CROSS_COMPILE}objcopy
GDB           := ${CROSS_COMPILE}gdb

FREERTOS_PORT = ARM_CM3

CFLAGS        := -O2 -Wall -W --std=c11 -mcpu=cortex-m3 -msoft-float -mthumb -g
CPPFLAGS      := -nostdinc -DSTM32F10X_MD -I${ROOT}/libc/include -I${ROOT}/cmsis/include \
                 -I${ROOT}/freertos/include -I${ROOT}/freertos/port/${FREERTOS_PORT}/include \
                 -I${ROOT}/application/include
                