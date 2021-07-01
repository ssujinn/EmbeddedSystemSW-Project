#include "out_proc.h"

void fnd_device_driver (int fnd_data) {

	int dev;
	unsigned char data[4];
	unsigned char retval;
	int i;
    
    dev = open(DEVICE_FND, O_RDWR);
    if (dev < 0) {
        printf("Device open error : %s\n", DEVICE_FND);
    }

    for (i=3; i >= 0; i--) {
        data[i] = fnd_data % 10;
        fnd_data /= 10;
    }

    retval = write(dev, &data, 4);	
    if(retval < 0) {
        printf("Write Error!\n");
        return;
    }

    close(dev);
}

void led_mmap (unsigned char led_data) {

    int fd, i;
    
    unsigned long *fpga_addr = 0;
    unsigned char *led_addr =0;
    unsigned char data;

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
            perror("/dev/mem open error");
    }

    fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, FPGA_BASE_ADDRESS);
    if (fpga_addr == MAP_FAILED)
    {
            printf("mmap error!\n");    
            close(fd);
    }
    led_addr=(unsigned char *)((void *) fpga_addr + LED_ADDR);
    *led_addr=led_data; //write to led

    munmap(led_addr, 4096);
    close(fd);
    
    usleep(1000);
    return;
}

void dot_device_driver (unsigned char fpga_dot_set[10]) {

    int fd; 

	fd = open(DEVICE_DOT_MATRIX, O_WRONLY);
    if (fd<0) {
		printf("Device open error : %s\n", DEVICE_DOT_MATRIX);
	}

    write(fd, fpga_dot_set, 10);

	close(fd);
}


void text_device_driver (unsigned char *text_data) {

    int i, fd;
    int len;
    unsigned char string[32];
    
    fd = open (DEVICE_TEXT_LCD, O_WRONLY);
    if (fd < 0) {
		printf("Device open error : %s\n", DEVICE_TEXT_LCD);
        return;
    }

    usleep(10000);
    memset (string, ' ', 32);
    strncpy (string, text_data, 32);
    write (fd, string, 32);
    close (fd);
}
