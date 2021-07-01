#ifndef _IN_PROC_H_
#define _IN_PROC_H_

#include "main.h"

// define path of device or device driver
#define PATH_READKEY "/dev/input/event0"
#define PATH_SWITCH "/dev/fpga_push_switch"

// code of READ_KEY_BUTTON
#define READKEY_HOME 102
#define READKEY_BACK 158
#define REAKEY_PROG 116
#define READKEY_VUP 115
#define READKEY_VDOWN 114

void inproc_main(int);
void in_readkey(int, INPUT_BUF *);
void in_switch(int, INPUT_BUF *);

#endif
