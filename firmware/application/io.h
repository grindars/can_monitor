#ifndef __IO__H__
#define __IO__H__

#include <stm32f10x.h>

/*
 * Pin mapping:
 * PA0  IN  -- WKUP
 * PA1  AN  -- VOLPOT
 * PA2  IN  PU
 * PA3  IN  PU
 * PA4  AF  -- SD_SS
 * PA5  AF  -- SD_SCK
 * PA6  IN  PU SD_MISO
 * PA7  AF  -- SD_MOSI
 *
 * PA8  IN  -- SD_DETECT
 * PA9  AF  -- UART_TX
 * PA10 IN  PU UART_RX
 * PA11 IN  PU (USB_D-)
 * PA12 IN  PU (USB_D+)
 * PA13 AF  -- SWDIO
 * PA14 AF  -- SWCLK
 * PA15 IN  PU
 *
 * PB0  IN  PU 
 * PB1  IN  PU
 * PB2  IN  PU (BOOT1)
 * PB3  IN  PU
 * PB4  IN  PU
 * PB5  IN  PU
 * PB6  IN  PU
 * PB7  IN  PU
 *
 * PB8  IN  PU CAN_R (LED0)
 * PB9  AF  -- CAN_D (LED1)
 * PB10 IN  PU (LED2)
 * PB11 IN  PU (LED3)
 * PB12 IN  PU (LED4)
 * PB13 IN  PU (LED5)
 * PB14 IN  PU (LED6)
 * PB15 IN  PU (LED7)
 *
 * PC0  IN  PU LCD_DB7
 * PC1  IN  PU LCD_DB6
 * PC2  IN  PU LCD_DB5
 * PC3  IN  PU LCD_DB4
 * PC4  IN  PU
 * PC5  IN  PU
 * PC6  IN  PU
 * PC7  IN  PU
 *
 * PC8  IN  PU
 * PC9  IN  PU
 * PC10 OUT -- LCD_E
 * PC11 OUT -- LCD_RW
 * PC12 OUT -- LCD_RS
 * PC13 IN  -- TAMP
 * PC14 AN  --
 * PC15 AN  --
 * 
 * PD0  AN  --
 * PD1  AN  --
 * PD2  IN  PU (USB_PRES)
 */

static inline void io_init(void) {
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_1 | AFIO_MAPR_CAN_REMAP_REMAP2;

    GPIOA->CRL = 0xb8b34408;
    GPIOA->CRH = 0x8bb888b8;
    GPIOB->CRL = 0x88888888;
    GPIOB->CRH = 0x888888b8;
    GPIOC->CRL = 0x88888888;
    GPIOC->CRH = 0x00422288;
    GPIOD->CRL = 0x00000800;
    GPIOA->BSRR = 1 << 6;
}

static inline void io_lcd_assert_e(void) {
    GPIOC->BSRR = (1 << 10);
}

static inline void io_lcd_deassert_e(void) {
    GPIOC->BSRR = (1 << (16 + 10));
}

static inline void io_lcd_assert_rw(void) {
    GPIOC->BSRR = (1 << 11);
}

static inline void io_lcd_deassert_rw(void) {
    GPIOC->BSRR = (1 << (16 + 11));
}

static inline void io_lcd_assert_rs(void) {
    GPIOC->BSRR = (1 << 12);
}

static inline void io_lcd_deassert_rs(void) {
    GPIOC->BSRR = (1 << (16 + 12));
}

static inline void io_lcd_assert_bus(unsigned char nibble) {
    GPIOC->CRL  = (GPIOC->CRL & 0xFFFF0000) | 0x00002222;
    GPIOC->BSRR = (0x0F << 16) | nibble;    
}

static inline void io_lcd_release_bus(void) {
    GPIOC->BSRR = 0x0F << 16;
    GPIOC->CRL  = (GPIOC->CRL & 0xFFFF0000) | 0x00008888; 
}

static inline unsigned char io_lcd_read_bus(void) {
    return GPIOC->IDR & 0x000F;
}

static inline int io_sd_present(void) {
    return (GPIOA->IDR & (1 << 8)) == 0;
}

static inline void io_sd_assert_cs(void) {
    GPIOA->BSRR = 1 << (16 + 4);
}

static inline void io_sd_deassert_cs(void) {
    GPIOA->BSRR = 1 << 4;
}

#endif
