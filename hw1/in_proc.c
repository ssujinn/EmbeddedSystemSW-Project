#include "in_proc.h"

int proc_end;
void inproc_main(int in_data) {
    int fd_readkey, fd_switch;
    INPUT_BUF *in_buf;
    
    in_buf = (INPUT_BUF *)shmat(in_data, 0, 0);

    if((fd_readkey = open(PATH_READKEY, O_RDONLY | O_NONBLOCK)) == -1){
        printf("Error : READ_KEY open failed!\n");
        exit(-1);
    }
    
    if((fd_switch = open(PATH_SWITCH, O_RDWR)) == -1){
        printf("Error : SWITCH open failed!\n");
        exit(-1);
    }

    while (!proc_end){
        in_readkey(fd_readkey, in_buf);
        in_switch(fd_switch, in_buf);

        usleep(100000);
    }

    in_buf->is_end = true;

    close(fd_readkey);
    close(fd_switch);
    shmdt((char*)in_buf);
}

void in_readkey(int fd, INPUT_BUF *buf){
    struct input_event ev;

    if(read(fd, (void*)&ev, sizeof(ev)) < sizeof(ev)){          
        printf("Error : READ_KEY read failed!\n");
        return;
    }

    if (ev.value == 1){
        switch(ev.code){
            case READKEY_BACK:
                proc_end = true;
                buf->input_type = 1;
                buf->readkey_code = ev.code;
                exit(0);
                break;
            case READKEY_VUP:
                buf->input_type = 1;
                buf->readkey_code = ev.code;                
                break;
            case READKEY_VDOWN:
                buf->input_type = 1;
                buf->readkey_code = ev.code;
                break;
            default :
                break;
        }
    }
    else
        buf->readkey_code = -1;
}

void in_switch(int fd, INPUT_BUF *buf){
    unsigned char sw_buf[9], prev_sw_buf[9], sw_data[9];
	int i, j;

    memset(sw_data, 0, sizeof(sw_data));
    for (i = 0; i < 5000; i++) {
        memcpy(prev_sw_buf, sw_buf, sizeof(sw_buf));
        read(fd, sw_buf, sizeof(sw_buf));

        for (j = 0; j < 9; j++) {
            if (sw_buf[j] - prev_sw_buf[j] == 1) {
                sw_data[j] = 1;
            }
        }
    }
    
    memcpy(buf->switch_code, sw_data, sizeof(sw_data));
}
