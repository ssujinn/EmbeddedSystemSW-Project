#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define DEV_NAME "/dev/stopwatch"

int main(){
	int fd;
	unsigned char buf;

	// device file open
	fd = open(DEV_NAME, O_RDWR);
	// if failed, print error message
	if (fd < 0) {
		printf("Error: device file open failed\n");
		exit(-1);
	}

	// write device (sleep status)
	write(fd, &buf, sizeof(buf));

	// close device (user program end)
	close(fd);

	return 0;
}
