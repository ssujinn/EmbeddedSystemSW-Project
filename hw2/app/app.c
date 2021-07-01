// libs
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <asm/ioctl.h>
#include <string.h>
#include <unistd.h>

// device info
#define DEV_NAME "/dev/dev_driver"
#define DEV_MAJOR 242

// user data struct
typedef struct _USER_DATA{
	unsigned char interval;
	unsigned char cnt;
	unsigned char init[4];
}USER_DATA;

// ioctl define
#define SET_OPTION _IOW(DEV_MAJOR, 0, USER_DATA *)
#define COMMAND _IOW(DEV_MAJOR, 1, int)

// application main function
int main(int argc, char **argv){
	USER_DATA udata;
	int fd;
	int i, nonzero_cnt, ret;

	// Usage error
	if (argc != 4) {
		printf("Usage: ./app TIMER_INTERVAL[1-100] TIMER_CNT[1-100] TIMER_INIT[0001-8000]\n");
		return -1;
	}

	// TIMER_INIT format error
	for (i = 0; i < 4; i++)
		if (argv[3][i] != '0') nonzero_cnt++;

	if (nonzero_cnt != 1) {
		printf("Error: TIMER_INIT format is inappropriate\n");
		return -1;
	}

	// user data allocation
	udata.interval = atoi(argv[1]);
	udata.cnt = atoi(argv[2]);
	strcpy(udata.init, argv[3]);

	// Range error
	if (udata.interval < 1 || udata.interval > 100 || udata.cnt < 1 || udata.cnt >100 || atoi(udata.init) < 1 || atoi(udata.init) > 8000){
		printf("Error: Out of Range\n");
		return -1;
	}

	// device file open
	fd = open(DEV_NAME, O_WRONLY);

	if (fd < 0){
		printf("ERROR: device open failed: %d\n", fd);
		return -1;
	}

	// ioctl SET_OPTION
	ret = ioctl(fd, SET_OPTION, &udata);

	if (ret < 0){
		printf("ERROR: ioctl SET_OPTION failed: %d\n", ret);
		close(fd);
		return -1;
	}

	// ioctl COMMAND
	ret = ioctl(fd, COMMAND);

	if (ret < 0) {
		printf("ERROR: ioctl COMMAND failed: %d\n", ret);
		close(fd);
		return -1;
	}

	// device file close
	close(fd);

	return 0;
}
