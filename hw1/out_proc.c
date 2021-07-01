#include "out_proc.h"

void outproc_main (int out_data) {
    int fnd_data; 
    unsigned char led_data, init_flag;
	bool proc_end;

    OUTPUT_BUF *out_buf = (OUTPUT_BUF *)shmat(out_data, 0, 0);

    memset (out_buf->dot_data, 0, sizeof(char) * 10); 

    while (!proc_end) {
        if (out_buf->is_end == true) {
            proc_end = true;
            out_buf->fnd_data = 0;
            out_buf->led_data = 0;
            memset(out_buf->dot_data, 0, sizeof(char) * 10); 
            memset(out_buf->text_data, ' ', sizeof(char) * TEXT_BUF_SIZE);
        }

        int out_mode = out_buf->mode;

        if (out_buf->is_init == true) {
            out_buf->is_init = false;
            out_buf->led_data = 0;
            if (out_mode) out_buf->fnd_data = 0;
            memset (out_buf->text_data, ' ', sizeof(char) * TEXT_BUF_SIZE);
            memset (out_buf->dot_data, 0, sizeof(char) * 10);
                            

            if (out_mode == 0)
                out_buf->led_data = 128;
            else if (out_mode == 1)
                out_buf->led_data = 64;
            else if (out_mode == 2)
                memcpy (out_buf->dot_data, dot_alphabet, sizeof(char) * 10); 

            led_mmap(out_buf->led_data);
            fnd_device_driver(out_buf->fnd_data);
            text_device_driver(out_buf->text_data);
            dot_device_driver(out_buf->dot_data);
        }

        fnd_device_driver(out_buf->fnd_data);
        if (out_mode == 0 || out_mode == 1 || out_mode == 3)
            led_mmap(out_buf->led_data);
        if (out_mode == 2 || out_mode == 3)
            dot_device_driver(out_buf->dot_data);
        if (out_mode == 2)
            text_device_driver(out_buf->text_data);

        usleep(100000);
    }    

    shmdt((char *)out_buf);
}
