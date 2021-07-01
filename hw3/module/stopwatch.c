/* Headers */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>

// device name
#define DEV_NAME "stopwatch"
// fnd device address
#define FND_ADDR 0x08000004

/* Variables */
static int stopwatch_major=242, stopwatch_minor=0;
static int result;
static dev_t stopwatch_dev;
static struct cdev stopwatch_cdev;
static int stopwatch_usage=0;
int now_time;
static unsigned char *fnd_ptr;

// Flags
int home_pressed = 0;
int back_pressed = 0;
int reset_pressed = 0;
int terminate = 0;

// Timer
typedef struct _TIMER_DATA {
	struct timer_list timer;
	int count;
} TIMER_DATA;

TIMER_DATA stopwatch_timer;
TIMER_DATA end_timer;

/* Functions: FOPS */
static int inter_open(struct inode *, struct file *);
static int inter_release(struct inode *, struct file *);
static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

/* Functions: Timer */
void timer_func(unsigned long timeout);
void timer_clear(unsigned long timeout);

/* Functions: FND control */
void FND_print(void);

/* Functions: Interrupt Handlers (Top Half) */
// HOME
irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg);
// BACK
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg);
// VOL UP
irqreturn_t inter_handler3(int irq, void* dev_id, struct pt_regs* reg);
// VOL DOWN
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg);

/* Functions: Interrupt Handlers (Bottom Half) */
void app_terminate(void);

// bottom half tasklet
DECLARE_TASKLET(end_task, app_terminate, 0);

// Wait queue declare
wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);

// File operations: open, write, release
static struct file_operations stopwatch_fops =
{
	.open = inter_open,
	.write = inter_write,
	.release = inter_release,
};

void app_terminate(void){
	// wake up and terminate
	__wake_up(&wq_write, 1, 1, NULL);
	printk("wake up\n");
}

void timer_func(unsigned long timeout){
	TIMER_DATA *p_data = (TIMER_DATA*)timeout;
	if (terminate)
		return;

	// 0.1sec added (when not paused)
	if (!back_pressed)
		p_data -> count++;

	// reset -> time reset
	if (reset_pressed){
		now_time = 0;
		p_data -> count = 0;
		reset_pressed = 0;
	}

	// 0.1sec * 10 => 1 sec, time added
	if (p_data -> count == 10){
		p_data -> count = 0;
		now_time++;
	}

	// FND time print
	FND_print();

	// time print
	printk("time: %d sec - %d\n", now_time, p_data -> count);

	// timer data setting
	stopwatch_timer.timer.expires = get_jiffies_64() + (HZ / 10);
	stopwatch_timer.timer.data = (unsigned long)&stopwatch_timer;
	stopwatch_timer.timer.function = timer_func;

	// add timer
	add_timer(&stopwatch_timer.timer);
}

void timer_clear(unsigned long timeout){
	terminate = 1;

	// time 0000
	now_time = 0;
	FND_print();

	// Bottom Half
	tasklet_schedule(&end_task);
}

void FND_print(void){
	int min, sec;
	int val = 0;

	// caculate miniutes & seconds
	min = now_time / 60;
	sec = now_time % 60;

	// FND data calculate
	val += (min / 10) << 12;
	val += (min % 10) << 8;
	val += (sec / 10) << 4;
	val += (sec % 10);

	// FND data print
	outw(val, (unsigned int)fnd_ptr);
}

// HOME: Start
irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "Start\n");
	// start button is already pressed
	if (home_pressed && !back_pressed)
		return IRQ_HANDLED;
	if (home_pressed && back_pressed){
		back_pressed = 0;
		return IRQ_HANDLED;
	}

	// mark start button is pressed
	home_pressed = 1;
	back_pressed = 0;
	reset_pressed = 0;

	// stopwatch time init
	now_time = 0;

	// timer data initialize
	stopwatch_timer.timer.expires = get_jiffies_64() + (HZ / 10); // 소수점 첫째자리 까지
	stopwatch_timer.timer.function = timer_func;
	stopwatch_timer.timer.data = (unsigned long)&stopwatch_timer;

	// add timer
	add_timer(&stopwatch_timer.timer);

	return IRQ_HANDLED;
}

// BACK: Pause
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "Pause\n");
	// back button is already pressed
	if (!home_pressed)
		return IRQ_HANDLED;
	if (back_pressed)
		return IRQ_HANDLED;

	back_pressed = 1;
	
	return IRQ_HANDLED;
}

// VOL+: Reset
irqreturn_t inter_handler3(int irq, void* dev_id,struct pt_regs* reg) {
	printk(KERN_ALERT "Reset\n");
	// flag modification
	home_pressed = 1;
	back_pressed = 1;
	now_time = 0;
	reset_pressed = 1;

	return IRQ_HANDLED;
}

// VOL-: Stop
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg) {
	unsigned int is_pressed = gpio_get_value(IMX_GPIO_NR(5, 14));

	printk(KERN_ALERT "Stop\n");

	// VOL- pressed: timer set
	if (!is_pressed){
		end_timer.timer.expires = get_jiffies_64() + (3 * HZ);
		end_timer.timer.function = timer_clear;
		add_timer(&end_timer.timer);
	}
	// VOL- unpressed: timer del
	else {
		del_timer(&end_timer.timer);
	}

	return IRQ_HANDLED;
}


static int inter_open(struct inode *minode, struct file *mfile){
	int ret;
	int irq;

	if (stopwatch_usage)
		return -EBUSY;
	stopwatch_usage++;

	printk(KERN_ALERT "Open Module\n");
	now_time = 0;
	home_pressed = 0;
	back_pressed = 0;
	reset_pressed = 0;
	terminate = 0;


	// int1
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler1, IRQF_TRIGGER_FALLING, "home", 0);

	// int2
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler2, IRQF_TRIGGER_FALLING, "back", 0);

	// int3
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler3, IRQF_TRIGGER_FALLING, "volup", 0);

	// int4
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler4, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "voldown", 0);

	// FND device 0000
	FND_print();

	// timer init
	init_timer(&(stopwatch_timer.timer));
	init_timer(&(end_timer.timer));

	return 0;
}

static int inter_release(struct inode *minode, struct file *mfile){
	stopwatch_usage--;
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);

	// FND data 0000
	now_time = 0;
	FND_print();

	// variables => terminated
	home_pressed = 0;
	back_pressed = 0;
	reset_pressed = 0;
	terminate = 1;

	// timer delete
	del_timer_sync(&stopwatch_timer.timer);
	del_timer_sync(&end_timer.timer);

	printk(KERN_ALERT "Release Module\n");
	return 0;
}

static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){
	// Write: sleep
	printk("sleep on\n");
	interruptible_sleep_on(&wq_write);

	printk("write\n");
	
	return 0;
}

static int inter_register_cdev(void)
{
	int error;

	// mkdev
	if(stopwatch_major) {
		stopwatch_dev = MKDEV(stopwatch_major, stopwatch_minor);
		error = register_chrdev_region(stopwatch_dev,1,"stopwatch");
	}else{
		error = alloc_chrdev_region(&stopwatch_dev, stopwatch_minor,1,"stopwatch");
		stopwatch_major = MAJOR(stopwatch_dev);
	}

	// warning print
	if(error<0) {
		printk(KERN_WARNING "inter: can't get major %d\n", stopwatch_major);
		return result;
	}

	printk(KERN_ALERT "major number = %d\n", stopwatch_major);

	// device init
	cdev_init(&stopwatch_cdev, &stopwatch_fops);
	stopwatch_cdev.owner = THIS_MODULE;
	stopwatch_cdev.ops = &stopwatch_fops;

	// error print
	error = cdev_add(&stopwatch_cdev, stopwatch_dev, 1);
	if(error)
	{
		printk(KERN_NOTICE "Stopwatch Register Error %d\n", error);
	}

	return 0;
}

static int __init inter_init(void) {
	int result;

	// register device
	if((result = inter_register_cdev()) < 0 )
		return result;

	printk(KERN_ALERT "Init Module Success \n");
	printk(KERN_ALERT "Device : /dev/stopwatch, Major Num : 242 \n");

	// FND device mapping
	fnd_ptr = ioremap(FND_ADDR, 0x4);

	stopwatch_usage = 0;
	
	return 0;
}

static void __exit inter_exit(void) {
	// device delete
	cdev_del(&stopwatch_cdev);

	// device unregistration
	unregister_chrdev_region(stopwatch_dev, 1);

	// FND device unmapping
	iounmap(fnd_ptr);

	printk(KERN_ALERT "Remove Module Success \n");
}

module_init(inter_init);
module_exit(inter_exit);

MODULE_LICENSE("GPL");
