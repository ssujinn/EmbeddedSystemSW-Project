#include "main.h"

#define MAX_DIGIT 4

#define FPGA_BASE_ADDRESS 0x08000000 //fpga_base address
#define LED_ADDR 0x16

// Output Device
#define DEVICE_DOT_MATRIX "/dev/fpga_dot"
#define DEVICE_FND "/dev/fpga_fnd"
#define DEVICE_TEXT_LCD "/dev/fpga_text_lcd"

void fnd_device_driver(int);
void led_mmap(unsigned char);
void dot_device_driver(unsigned char *);
void text_device_driver(unsigned char *);
