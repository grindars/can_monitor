#ifndef __TTY__H__
#define __TTY__H__

void ttyInit(void);
void ttyPut(char ch);
int ttyPrint(const char *fmt, ...);

#endif
