CC = arm-none-linux-gnueabi-gcc
CFLAGS = -static
OBJ = main.o in_proc.o out_proc.o device_ctrl.o 
TARGET = 20171640

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

main.o : main.c main.h
	$(CC) $(CFLAGS) -c main.c

inp_proc.o : in_proc.c in_proc.h main.h
	$(CC) $(CFLAGS) -c in_proc.c

out_proc.o : out_proc.c out_proc.h main.h
	$(CC) $(CFLAGS) -c out_proc.c

device_ctrl.o : device_ctrl.c device_ctrl.h main.h
	$(CC) $(CFLAGS) -c device_ctrl.c

clean:
	rm -rf $(TARGET) $(OBJ)

push:
	make
	adb push $(TARGET) /data/local/tmp
