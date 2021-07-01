// libs
#include <asm/io.h>
#include <asm/ioctl.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>

// device info
#define DEV_NAME "dev_driver"
#define DEV_MAJOR 242

// IO device address info
#define LED_ADDR 0x08000016
#define FND_ADDR 0x08000004
#define DOT_ADDR 0x08000210
#define TEXT_LCD_ADDR 0x08000090

// user data
typedef struct _USER_DATA {
	unsigned char interval;
	unsigned char cnt;
	unsigned char init[4];
}USER_DATA;

// timer data
typedef struct _TIMER_DATA {
	struct timer_list timer;
	int count;
}TIMER_DATA;

// ioctl commands
#define SET_OPTION _IOW(DEV_MAJOR, 0, USER_DATA*)
#define COMMAND _IOW(DEV_MAJOR, 1, int)

// device usage
static int device_usage = 0;

/* -----------------------VARIABLES-----------------------*/
// timer var
int interval;
int cnt;
unsigned char init[4];

// struct var
USER_DATA udata;
TIMER_DATA tdata;

// IO device address
static unsigned char *led_addr;
static unsigned char *fnd_addr;
static unsigned char *dot_addr;
static unsigned char *text_lcd_addr;

// Device output value
unsigned char fnd_val[4];
int rot_cnt;
int fnd_digit;
unsigned char text_lcd_val[32];
int text_up_dir = 1;
int text_up_pos = 0;
int text_down_dir = 1;
int text_down_pos = 0;

// Dot matrix number
unsigned char blanks[10] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

unsigned char dot_number[10][10] = {
	{0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
	{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
	{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
	{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
	{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
	{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
	{0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03} // 9
};

/* --------------------------FUNCTION-----------------------*/
// device driver module functions
int device_open(struct inode *, struct file *);
int device_release(struct inode *, struct file *);
long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);
// timer function
static void timer_func(unsigned long timeout);
// device clear
void device_end(void);
// device output
void device_out(void);
