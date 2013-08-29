#include <FreeRTOS.h>
#include <semphr.h>

#include <stdio.h>

#include "tty.h"
#include "hd44780.h"

static xSemaphoreHandle ttyMutex;

#define COLS    16
#define ROWS    2

void ttyPut(char ch) {
    unsigned char buffer[COLS];

    unsigned char address = hd44780_status() & HD44780_STATUS_ADDRESS;
    unsigned char column = address & 0x3F;
    unsigned char row    = address >> 6;

    switch(ch) {
    case '\b':
        if(column != 0) {
            address--;
            hd44780_command(HD44780_SET_DDRAM | address);
        }

        break;

    case '\n':
        if(row < ROWS - 1) {
            row++;
            hd44780_command(HD44780_SET_DDRAM | (row << 6) | column);
        } else {
            for(int line = 0; line < ROWS - 1; line++) {
                hd44780_command(HD44780_SET_DDRAM | ((line + 1) << 6));

                for(int character = 0; character < COLS; character++) {
                    buffer[character] = hd44780_read();
                }

                hd44780_command(HD44780_SET_DDRAM | (line << 6));         
                
                for(int character = 0; character < COLS; character++) {
                    hd44780_write(buffer[character]);
                }
            }

            hd44780_command(HD44780_SET_DDRAM | ((ROWS - 1) << 6));

            for(int character = 0; character < COLS; character++) {
                hd44780_write(' ');
            }

            hd44780_command(HD44780_SET_DDRAM | address);
        }

        break;

    case '\r':
        if(column > 0)
            hd44780_command(HD44780_SET_DDRAM | (row << 6));

        break;

    default:
        hd44780_write(ch);
        if(column == COLS - 1)
            hd44780_command(HD44780_SET_DDRAM | address);

        break;
    }
}

void ttyInit(void) {
    ttyMutex = xSemaphoreCreateMutex();
    hd44780_init();
}

static void ttyPutWrapper(char ch, void *arg) {
    (void) arg;

    ttyPut(ch);
}

int ttyPrint(const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    xSemaphoreTake(ttyMutex, portMAX_DELAY);
    int ret = vcprintf(fmt, list, ttyPutWrapper, NULL);
    xSemaphoreGive(ttyMutex);
    va_end(list);
    return ret;
}
