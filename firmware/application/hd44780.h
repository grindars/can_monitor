#ifndef __HD44780__H__
#define __HD44780__H__

#define HD44780_STATUS_BF      0x80
#define HD44780_STATUS_ADDRESS 0x7F

#define HD44780_CLEAR_DISPLAY   1
#define HD44780_RETURN_HOME     2
#define HD44780_ENTRY_MODE_SET  4
#define HD44780_ENTRY_MODE_SET_S    1
#define HD44780_ENTRY_MODE_SET_ID   2
#define HD44780_DISPLAY_ONOFF   8
#define HD44780_DISPLAY_ONOFF_B     1
#define HD44780_DISPLAY_ONOFF_C     2
#define HD44780_DISPLAY_ONOFF_D     4
#define HD44780_SHIFT           16
#define HD44780_SHIFT_RL            4
#define HD44780_SHIFT_SC            8
#define HD44780_FUNCTION_SET    32
#define HD44780_FUNCTION_SET_F      4
#define HD44780_FUNCTION_SET_N      8
#define HD44780_FUNCTION_SET_DL     16
#define HD44780_SET_CGRAM       64
#define HD44780_SET_CGRAM_MASK      63
#define HD44780_SET_DDRAM       128
#define HD44780_SET_DDRAM_MASK      127

void hd44780_init(void);

void hd44780_command(unsigned char command);
unsigned char hd44780_status(void);

void hd44780_write(unsigned char byte);
unsigned char hd44780_read(void);

#endif
