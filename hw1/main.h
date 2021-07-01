#ifndef _MAIN_H_
#define _MAIN_H_

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <linux/input.h>
#include <sys/mman.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#define KEY_IN (key_t) 0x10 
#define KEY_OUT (key_t) 0x15 
#define KEY_SEM (key_t) 0x20 

#define BLINK -1
#define ADD_MINUTE -2
#define MIN_MODI 1
#define HOUR_MODI 2

#define READKEY_HOME 102
#define READKEY_BACK 158
#define READKEY_PROG 116
#define READKEY_VUP 115
#define READKEY_VDOWN 114

#define TEXT_BUF_SIZE 32

const static unsigned char dot_alphabet[10] = {
    0x1c,0x36,0x63,0x63,0x63,0x7f,0x7f,0x63,0x63,0x63  // A
};

const static unsigned char dot_number[10] = {
	0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e // 1
};

const static char* keypad[] = {
    ".QZ",  
    "ABC",
    "DEF",
    "GHI",
    "JKL",
    "MNO",
    "PRS",
    "TUV",
    "WXY"
};

typedef struct _INPUT_BUF_ {
    bool is_end;
    int input_type;
    int readkey_code;
    unsigned char switch_code[9];
} INPUT_BUF;

typedef struct _OUTPUT_BUF_ {
    bool is_init;
    bool is_end;
    char text_data[32];
    unsigned char led_data;
    unsigned char dot_data[10];
    int fnd_data;
    int mode;
} OUTPUT_BUF;

int clock_prev;
int counter_num;

// functional modes
int clock_mode;
int counter_mode;
int draw_mode;
int text_mode;


int input_text;
int len;
int same_count;
int draw_x;
int draw_y;
unsigned char map_arr[10];

void main_proc(int, int);
void mode_select(OUTPUT_BUF *, bool);
void clock_proc(OUTPUT_BUF *, unsigned char *, int);
void time_calculate(OUTPUT_BUF*, int);
int get_time();
void counter_proc(OUTPUT_BUF *, unsigned char*);
void counter_ctrl(int, OUTPUT_BUF*);
void text_proc(OUTPUT_BUF*, unsigned char*);
void draw_proc(OUTPUT_BUF*, unsigned char*, int);

#endif
