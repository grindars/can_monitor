#ifndef __DELAY__H__
#define __DELAY__H__

extern void delay_loop(unsigned int cycles);

#define delay_cycles(cycles) delay_loop((cycles) / 3 - 2)

#endif
