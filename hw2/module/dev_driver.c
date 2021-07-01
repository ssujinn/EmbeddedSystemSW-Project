#include "dev_driver.h"

// file operations
static struct file_operations dev_fops =
{
	open: device_open,
	release: device_release,
	unlocked_ioctl: device_ioctl,
};

int device_release(struct inode *minode, struct file *mfile) {
	printk("device_release\n");
	device_usage = 0;
	return 0;
}

int device_open(struct inode *minode, struct file *mfile) {
	printk("device_open\n");
	if (device_usage != 0) {
		return -EBUSY;
	}
	device_usage = 1;
	return 0;
}

long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param) {
	int i;

	switch (ioctl_num){
		// save timer data form user data
		case SET_OPTION:
			// copy data to kernel memory
			if (copy_from_user(&udata, (USER_DATA *)ioctl_param, sizeof(USER_DATA)))
					return -EFAULT;

			// save timer option
			interval = (int)udata.interval;
			cnt = (int)udata.cnt;
			strncpy(fnd_val, udata.init, 4);

			break;

		// timer run
		case COMMAND:
			// fnd data init
			rot_cnt = 0;
			for (i = 0; i < 4; i++){
				fnd_val[i] = fnd_val[i] - '0';

				if (fnd_val[i] != 0)
					fnd_digit = i;
			}

			// text_lcd_data init
			memset(text_lcd_val, ' ', sizeof(text_lcd_val));
			strncpy(text_lcd_val, "20171640", 8);
			strncpy(text_lcd_val + 16, "Sujin Park", 10);
			text_up_dir = 1; text_up_pos = 0;
			text_down_dir = 1; text_down_pos = 0;

			// timer set
			tdata.count = cnt;
			tdata.timer.expires = jiffies + (interval * HZ / 10);
			tdata.timer.data = (unsigned long)&tdata;
			tdata.timer.function = timer_func;
			add_timer(&tdata.timer);

			break;
	}

	return 0;
} 

static void timer_func(unsigned long timeout) {
	TIMER_DATA *p_data = (TIMER_DATA *)timeout;

	// count down
	p_data -> count--;

	// program end
	if (p_data -> count < 0){
		device_end();
		return;
	}

	// IO device output
	device_out();

	// timer set
	tdata.timer.expires = get_jiffies_64() + (interval * HZ / 10);
	tdata.timer.data = (unsigned long)&tdata;
	tdata.timer.function = timer_func;
	add_timer(&tdata.timer);
}

void device_end(void){
	unsigned short tmp = 0;
	int i;

	// LED device
	outw(tmp, (unsigned int)led_addr);

	// FND device
	outw(tmp, (unsigned int)fnd_addr);

	// Dot device
	for (i = 0; i < 10; i++){
		tmp = blanks[i] & 0x7F;
		outw(tmp, (unsigned int)dot_addr + i * 2);
	}

	// TEXT LCD device
	for (i = 0; i < 32; i += 2){
		tmp = (' ' << 8) + ' ';
		outw(tmp, (unsigned int)text_lcd_addr + i);
	}
}

void device_out(void){
	unsigned short out_val = 0;
	int i;
	int fnd_num_prev;
	unsigned char up_text[16], down_text[16];
	unsigned char blank_char = ' ';

	/* ----------LED device---------- */
	out_val = 0x80 >> (fnd_val[fnd_digit] - 1);
	outw(out_val, (unsigned int)led_addr);

	/* ----------Dot Device---------- */
	for (i = 0; i < 10; i++) {
		out_val = dot_number[fnd_val[fnd_digit]][i] & 0x7F;
		outw(out_val, (unsigned int)dot_addr + i * 2);
	}

	/* ----------FND devcie---------- */
	fnd_num_prev = fnd_val[fnd_digit];

	// output value set
	out_val = 0;
	for (i = 0; i < 4; i++){
		out_val += fnd_val[i];
		if (i != 3) out_val <<= 4;
	}

	// fnd print
	outw(out_val, (unsigned int)fnd_addr);
	printk("FND output: %d%d%d%d\n", fnd_val[0], fnd_val[1], fnd_val[2], fnd_val[3]);

	// rotation count increase
	rot_cnt++;
	if (rot_cnt == 8) {
		rot_cnt = 0;
		fnd_digit = (fnd_digit + 1) % 4;
	}

	// set new fnd val
	memset(fnd_val, 0, sizeof(fnd_val));
	if (++fnd_num_prev > 8)
		fnd_num_prev = 1;
	fnd_val[fnd_digit] = fnd_num_prev;

	
	/* ----------Text LCD device---------- */
	// output value set and print
	for (i = 0; i < 32; i += 2){
		out_val = ((text_lcd_val[i] & 0xFF) << 8) + (text_lcd_val[i + 1] & 0xFF);
		outw(out_val, (unsigned int)text_lcd_addr + i);
	}
	// up text init
	memset(up_text, blank_char, sizeof(up_text));

	// change direction
	text_up_pos += text_up_dir;
	if (text_up_pos == 0)
		text_up_dir = 1;
	if (text_up_pos == 8)
		text_up_dir = -1;

	strncpy(up_text + text_up_pos, "20171640", 8);
	
	// down text init
	memset(down_text, blank_char, sizeof(down_text));

	// change direction
	text_down_pos += text_down_dir;
	if (text_down_pos == 0)
		text_down_dir = 1;
	if (text_down_pos == 6)
		text_down_dir = -1;

	strncpy(down_text + text_down_pos, "Sujin Park", 10);

	strncpy(text_lcd_val, up_text, 16);
	strncpy(text_lcd_val + 16, down_text, 16);
}

int __init device_init(void)
{
	int ret;
	
	printk("kernel_timer_init\n");

	// device register (name: "dev_driver", major: 242)
	ret = register_chrdev(DEV_MAJOR, DEV_NAME, &dev_fops);
	if (ret < 0){
		printk("Error: device registration failed\n");
		return ret;
	}

	printk("dev_file : /dev/%s , major : %d\n", DEV_NAME, DEV_MAJOR);

	// timer init
	init_timer(&(tdata.timer));

	// IO device mapping
	led_addr = ioremap(LED_ADDR, 0x1);
	fnd_addr = ioremap(FND_ADDR, 0x4);
	dot_addr = ioremap(DOT_ADDR, 0x10);
	text_lcd_addr = ioremap(TEXT_LCD_ADDR, 0x32);

	printk("init module\n");
	return 0;
}

void __exit device_exit(void)
{
	printk("device_exit\n");
	device_usage = 0;

	// IO device unmapping
	iounmap(led_addr);
	iounmap(fnd_addr);
	iounmap(dot_addr);
	iounmap(text_lcd_addr);

	// timer del
	del_timer_sync(&tdata.timer);

	// device unregister
	unregister_chrdev(DEV_MAJOR, DEV_NAME);
}

module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("author");
