#include <FreeRTOS.h>
#include <task.h>

#include "hd44780.h"
#include "io.h"
#include "delay.h"

static const unsigned char nibble_map[16] = {
    0x00,
    0x08,
    0x04,
    0x0C,
    0x02,
    0x0A,
    0x06,
    0x0E,
    0x01,
    0x09,
    0x05,
    0x0D,
    0x03,
    0x0B,
    0x07,
    0x0F
};

static void write_nibble(unsigned char nibble) {
    io_lcd_assert_bus(nibble_map[nibble]);

    io_lcd_assert_e();
    delay_cycles(SystemCoreClock / 1000000 / 2);
    io_lcd_deassert_e();

    io_lcd_release_bus();
}

static unsigned char read_nibble(void) {
    io_lcd_assert_e();
    delay_cycles(SystemCoreClock / 1000000 / 2);

    unsigned char nibble = io_lcd_read_bus();
    io_lcd_deassert_e();

    return nibble_map[nibble];    
}

void hd44780_command(unsigned char command) {
    write_nibble(command >> 4);
    write_nibble(command & 0xF);

    while(hd44780_status() & HD44780_STATUS_BF);
}

unsigned char hd44780_status(void) {
    io_lcd_assert_rw();

    unsigned char byte = read_nibble() << 4;
    byte |= read_nibble();

    io_lcd_deassert_rw();

    return byte;
}

void hd44780_write(unsigned char byte) {
    io_lcd_assert_rs();

    write_nibble(byte >> 4);
    write_nibble(byte & 0xF);

    io_lcd_deassert_rs();

    while(hd44780_status() & HD44780_STATUS_BF);   
}

unsigned char hd44780_read(void) {
    io_lcd_assert_rs();
    io_lcd_assert_rw();

    unsigned char byte = read_nibble() << 4;
    byte |= read_nibble();

    io_lcd_deassert_rw();
    io_lcd_deassert_rs();

    while(hd44780_status() & HD44780_STATUS_BF);
    
    return byte;      
}

void hd44780_init(void) {
    io_lcd_deassert_e();
    io_lcd_deassert_rw();
    io_lcd_deassert_rs();

    vTaskDelay(100 / portTICK_RATE_MS);
    
    write_nibble(3);
    vTaskDelay(5 / portTICK_RATE_MS);

    write_nibble(3);
    vTaskDelay(1 / portTICK_RATE_MS);

    write_nibble(3);
    vTaskDelay(1 / portTICK_RATE_MS);

    write_nibble(2);
    vTaskDelay(1 / portTICK_RATE_MS);

    hd44780_command(HD44780_FUNCTION_SET | HD44780_FUNCTION_SET_N | HD44780_FUNCTION_SET_F);
    hd44780_command(HD44780_ENTRY_MODE_SET | HD44780_ENTRY_MODE_SET_ID);
    hd44780_command(HD44780_CLEAR_DISPLAY);
    hd44780_command(HD44780_DISPLAY_ONOFF | HD44780_DISPLAY_ONOFF_C | HD44780_DISPLAY_ONOFF_D);
}

