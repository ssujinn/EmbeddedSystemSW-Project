#include "main.h"

int mode = 0;
bool is_sw1_clicked = false;


int main(){
    int in_pid, out_pid, in_data, out_data;
    
    in_data = shmget (KEY_IN, sizeof(INPUT_BUF), 0600 | IPC_CREAT);
    if(in_data == -1){
        printf("Error : INPUT shared memory get failed!\n");
        exit(-1);
    }
    out_data = shmget (KEY_OUT, sizeof(OUTPUT_BUF), 0600 | IPC_CREAT);
    if(out_data == -1){
        printf("Error : OUTPUT shared memory get failed!\n");
        exit(-1);
    }

    in_pid = fork();
    if (in_pid < 0){
        printf("Error : Input Process fork() failed!\n");
        exit(-1);
    }
    else if (in_pid == 0){
        inproc_main(in_data);
    }
    else {
        out_pid = fork();
        if (out_pid < 0){
            printf("Error : Output Process fork() failed!\n");
            exit(-1);
        }
        else if (out_pid == 0){
            outproc_main(out_data);
        }
        else main_proc(in_data, out_data);
    }

    wait(in_pid);
    wait(out_pid);
    shmctl(in_data, IPC_RMID, NULL);
    shmctl(out_data, IPC_RMID, NULL);

    return 0;
}

void main_proc (int in_data, int out_data) { 
    int sec_count=0, min_count=0;
	bool proc_end;
    int input_readkey, prev_readkey;
    INPUT_BUF *in_buf;
    OUTPUT_BUF *out_buf;

    in_buf = (INPUT_BUF *)shmat(in_data, 0, 0);
    out_buf = (OUTPUT_BUF *)shmat(out_data, 0, 0);
    
    in_buf->is_end = false;
    in_buf->input_type = 0;
    in_buf->readkey_code = -1;
    memset(in_buf->switch_code, 0, sizeof(unsigned char) * 9);

    out_buf->is_end = false;
    out_buf->mode = 0;
    out_buf->fnd_data = get_time();
    out_buf->led_data = 128;
    out_buf->is_init = true;
	memset(out_buf->text_data, ' ', sizeof(char) * TEXT_BUF_SIZE);
	memset(out_buf->dot_data, 0, sizeof(char) * 10); 

    draw_x = 0;
    draw_y = 0;
    len = 0;
    same_count = 0;
    counter_num = 0;

  	clock_mode = 0;
  	counter_mode = 0;
  	draw_mode = 0;
  	text_mode = 0;
    proc_end = false;

    while (!proc_end) {
	    prev_readkey = input_readkey;
	    input_readkey = in_buf->readkey_code;
                
        if (prev_readkey != input_readkey) {
            switch (input_readkey) {
                case READKEY_VUP:
                    mode_select(out_buf, 1);
                    break;
                case READKEY_VDOWN:
                    mode_select(out_buf, 0);
                    break;
                case READKEY_BACK:
                    proc_end = true;
                    in_buf->is_end = true;
                    out_buf->is_end = true;
                    break;
                default:
                    break;
            }  
        } 
        
        usleep(1000);
    		sec_count++;

  		if (sec_count == 1000) {
              if (mode == 0)
          				clock_proc(out_buf, in_buf->switch_code, BLINK);
  
              if (mode == 3 && draw_mode == 0)
                  draw_proc(out_buf, in_buf->switch_code, BLINK);
  
  			sec_count = 0;
  			min_count++;
  		}
       
        if (min_count == 60) {
			if (mode == 0)
                clock_proc(out_buf, in_buf->switch_code, ADD_MINUTE);
			min_count = 0;
        }

        switch (mode) {
            case 0:
                clock_proc(out_buf, in_buf->switch_code, 0);
                break;
            case 1:
                counter_proc(out_buf, in_buf->switch_code);
                break;
            case 2:
                text_proc(out_buf, in_buf->switch_code);
                break;
            case 3:
                draw_proc(out_buf, in_buf->switch_code, 0);
                break;
            default:
                break;
        }
    }

    shmdt((char *)in_buf);
    shmdt((char *)out_buf);
}

void mode_select(OUTPUT_BUF *buf, bool up_down) {
    if (mode == 0 && !is_sw1_clicked)
		clock_prev = buf->fnd_data;
	else
		is_sw1_clicked = false;

    if (up_down == 1){
        mode++;
        if (mode > 3) mode = 0;
    }
    else {
        mode--;
        if (mode < 0) mode = 3;
    }

    buf->mode = mode;
    buf->is_init = true;
    switch (mode) {
        case 0:
            clock_mode = 0;
            buf->fnd_data = clock_prev;
            break;
        case 1:
            counter_mode = 0;
			counter_num = 0;
            break;
        case 2:
            text_mode = 0;
            same_count = 0;
			len = 0;
			input_text = 0;
            break;
        case 3:
			memset(map_arr, 0, sizeof(map_arr));
            draw_mode = 0;
            draw_x = 0;
            draw_y = 0;
            break;
    }
}

/* CLOCK MODE */
void clock_proc(OUTPUT_BUF *buf, unsigned char *switch_code, int flag){
    if (flag == ADD_MINUTE) {
        time_calculate(buf, MIN_MODI);
        return;
	}

	if (flag == BLINK && clock_mode == 1) {
		if (buf->led_data == 32)
			buf->led_data = 16;
		else
			buf->led_data = 32;
		return;
	}
  
    if (switch_code[0]) {
		is_sw1_clicked = true;
        if (!clock_mode){
            clock_prev = buf->fnd_data;
            clock_mode = 1;
            buf->led_data = 16;
            switch_code[0] = 0;
        }
        else {
            clock_prev = buf->fnd_data;
            clock_mode = 0;
            buf->led_data = 128;
            switch_code[0] = 0;
        }
    } 
    else if (switch_code[1]) {
        buf->fnd_data = get_time();
        switch_code[1] = 0;
    } 
    else if (switch_code[2] && clock_mode == 1) {
        time_calculate(buf, HOUR_MODI);
        switch_code[2] = 0;
    } 
    else if (switch_code[3] && clock_mode ==1) {
        time_calculate(buf, MIN_MODI);
        switch_code[3] = 0;
    }
}

void time_calculate (OUTPUT_BUF *buf, int modi) {
    switch (modi){
        case MIN_MODI:
            if (buf->fnd_data % 100 == 59){
                if (buf->fnd_data / 100 == 23)
                    buf->fnd_data = 0;
                else
                    buf->fnd_data += 41;
            }
            else
                buf->fnd_data++;
            break;
        case HOUR_MODI:
            if ((buf->fnd_data / 100) == 23)
                buf->fnd_data %= 100;
            else
                buf->fnd_data += 100;
            break;
    }
}

int get_time() {
    time_t t = time(NULL);
    struct tm tt = *localtime (&t);

    return 100 * tt.tm_hour + tt.tm_min;
}

/* COUNTER MODE */
void counter_proc (OUTPUT_BUF* buf, unsigned char* switch_code) {
    if (switch_code[0]) {
        switch_code[0] = 0;
        counter_mode++;
        if (counter_mode == 4) counter_mode = 0;
        
    	if (counter_mode == 0) buf->led_data = 64;
    	else if (counter_mode == 1) buf->led_data = 32;
    	else if (counter_mode == 2) buf->led_data = 16;
    	else if (counter_mode == 3) buf->led_data = 128;   
        counter_ctrl(0, buf);
    } 
    else if (switch_code[1]) {
        switch_code[1] = 0;
        counter_ctrl(1, buf);
    } 
    else if (switch_code[2]) {
        switch_code[2] = 0;
        counter_ctrl(2, buf);
    } 
    else if (switch_code[3]) {
        switch_code[3] = 0;
        counter_ctrl(3, buf);
    }
    
     counter_ctrl(0, buf);
 
}

void counter_ctrl (int digit, OUTPUT_BUF* buf) {
    int base, digit_mul = 1;

    switch (counter_mode) {
        case 0:
            base = 10;
            break;
        case 1:
            base = 8;
            break;
        case 2:
            base = 4;
            break;
        case 3:
            base = 2;
            break;
        default:
            break;
    }

    if (digit > 0 && digit <= 3){
		if (digit < 3)
        	digit_mul *= base;
        if (digit < 2)
            digit_mul *= base;
		counter_num += digit_mul;
    }
    

    int one = counter_num % base;
    int ten = (counter_num / base) % base;
    int hundred = ((counter_num / base) / base) % base;

    buf->fnd_data = hundred * 100 + ten * 10 + one;
}

/* TEXT EDITOR MODE */
void text_proc (OUTPUT_BUF* buf, unsigned char* switch_code) { 
    int prev_input, tmpvar;
    char arr_tmp[32]={'0'};
	
    if (switch_code[1] == 1 && switch_code[2] == 1) {
        switch_code[1] = switch_code[2] = 0;
        text_mode = 0;
        len = 0; same_count = 0; input_text = 0;
        memset(buf->text_data, ' ', sizeof(char) * TEXT_BUF_SIZE);
		memcpy(buf->dot_data, dot_alphabet, sizeof(unsigned char) * 10);
        return;
    }
    else if (switch_code[4] == 1 && switch_code[5] == 1) {
        switch_code[4] = switch_code[5] = 0; 
        if (!text_mode) text_mode = 1;
        else text_mode = 0;

        if (text_mode == 0)
            memcpy(buf->dot_data, dot_alphabet, sizeof(unsigned char) * 10);
        else 
            memcpy(buf->dot_data, dot_number, sizeof(unsigned char) * 10);
        
        input_text = 0;
        buf->fnd_data++;
        if (buf->fnd_data >= 9999) buf->fnd_data = 0;
        return;
    } 
    else if (switch_code[7] == 1 && switch_code[8] == 1) {
        switch_code[7] = switch_code[8] = 0;
        input_text = 0;
        same_count = 0;
        if (len < 32) 
            buf->text_data[len++] = ' ';
        else if (len == 32) {
            memset (arr_tmp, 0, sizeof(char) * TEXT_BUF_SIZE);
            strncpy (arr_tmp, buf->text_data + 1, sizeof(char) * (TEXT_BUF_SIZE - 1));
            memset (buf->text_data, 0, sizeof(char) * TEXT_BUF_SIZE);
            strncpy (buf->text_data, arr_tmp, sizeof(char) * (TEXT_BUF_SIZE - 1));
            buf->text_data[len - 1] = ' ';
        }

        buf->fnd_data++;
        if (buf->fnd_data >= 9999) buf->fnd_data = 0;
        return;
    }

	int i;

    for (i = 0; i < 9; i++) {
        if (switch_code[i]){
            buf->fnd_data++;
            if (buf->fnd_data >= 9999) buf->fnd_data = 0;
            prev_input = input_text;
            input_text = i + 1;
            switch_code[i] = 0; 
            tmpvar = i;     
            memset (arr_tmp, 0, sizeof(char) * TEXT_BUF_SIZE);
            if (!text_mode) {
                if (prev_input == input_text) {
                    same_count++;
                    if (same_count > 2) same_count = 0;
                    buf->text_data[len-1] = keypad[tmpvar][same_count];
                }         
                else if (prev_input != input_text) {
                    same_count = 0;
                    if (len == TEXT_BUF_SIZE) {
                        strncpy (arr_tmp, buf->text_data + 1, sizeof(char) * (TEXT_BUF_SIZE - 1));
                        strncpy (buf->text_data, arr_tmp, sizeof(char) * (TEXT_BUF_SIZE - 1));
                        buf->text_data[len - 1] = keypad[tmpvar][same_count];
                    }
                    else if (len < TEXT_BUF_SIZE){
                        buf->text_data[len] = keypad[tmpvar][same_count];
						len++;
					}
                }
            }
            else {
                same_count = 0;
                if (len == TEXT_BUF_SIZE) {
                    strncpy (arr_tmp, buf->text_data + 1, sizeof(char) * (TEXT_BUF_SIZE - 1));
                    strncpy (buf->text_data, arr_tmp, sizeof(char) * (TEXT_BUF_SIZE - 1));
                    buf->text_data[len - 1] = (tmpvar + 1) + 48;
                }
                else if (len < (TEXT_BUF_SIZE)){
                    buf->text_data[len] = (tmpvar + 1) + 48;
					len++;
				}

            }

            break;
        }
        switch_code[i] = 0;
    }

}

/* DRAW BOARD MODE */
void draw_proc(OUTPUT_BUF* buf, unsigned char* switch_code, int flag) {
    if (flag == BLINK) 
        buf->dot_data[draw_y] ^= (0x40 >> draw_x);
    
    if (switch_code[0]) {
        buf->fnd_data = 0;
        draw_x = 0;
        draw_y = 0;
        memset(map_arr, 0, sizeof(unsigned char) * 10);
        memset(buf->dot_data, 0, sizeof(unsigned char) * 10);
        switch_code[0] = 0;
    } 
    else if (switch_code[1]) {
        draw_y--;
        if (draw_y < 0) draw_y = 9;
        memcpy(buf->dot_data, map_arr, sizeof(unsigned char) * 10);
        buf->fnd_data++;
        if (buf->fnd_data >= 9999) buf->fnd_data = 0;
        switch_code[1] = 0;
    }
    else if (switch_code[2]) {
        if (draw_mode == 0) 
            draw_mode = 1;
        else
            draw_mode = 0;
        memcpy (buf->dot_data, map_arr, sizeof(unsigned char) * 10);
        buf->fnd_data++;
        if (buf->fnd_data >= 9999) buf->fnd_data = 0;
        switch_code[2] = 0;
    }
    else if (switch_code[3]) {
        draw_x--;
        if (draw_x < 0) draw_x = 6;
        memcpy(buf->dot_data, map_arr, sizeof(unsigned char) * 10);
        buf->fnd_data++;
        if (buf->fnd_data >= 9999) buf->fnd_data = 0;
        switch_code[3] = 0;
    } 
    else if (switch_code[4]) {
        map_arr[draw_y] ^= (0x40 >> draw_x);
        memcpy(buf->dot_data, map_arr, sizeof(unsigned char) * 10);
        buf->fnd_data++;
        if (buf->fnd_data >= 9999) buf->fnd_data = 0;
        switch_code[4] = 0;  
    }
    else if (switch_code[5]) {
        draw_x++;
        if (draw_x > 6) draw_x = 0;
        memcpy(buf->dot_data, map_arr, sizeof(unsigned char) * 10);
        buf->fnd_data++;
        if (buf->fnd_data >= 9999) buf->fnd_data = 0;
        switch_code[5] = 0;
    } 
    else if (switch_code[6]) {
        memset(map_arr, 0, sizeof(unsigned char) * 10);
        memcpy(buf->dot_data, map_arr, sizeof(unsigned char) * 10);
        buf->fnd_data++;
        if (buf->fnd_data >= 9999) buf->fnd_data = 0;
        switch_code[6] = 0;
    } 
    else if (switch_code[7]) {
        draw_y++;
        if (draw_y > 9) draw_y = 0;
        memcpy(buf->dot_data, map_arr, sizeof(unsigned char) * 10);
        buf->fnd_data++;
        if (buf->fnd_data >= 9999) buf->fnd_data = 0;
        switch_code[7] = 0;
    }
    else if (switch_code[8]) {
		int i;
        for (i = 0; i < 10; i++)
            map_arr[i] ^= 0xFF;
        memcpy (buf->dot_data, map_arr, sizeof(unsigned char) * 10);
        buf->fnd_data++;
        if (buf->fnd_data >= 9999) buf->fnd_data = 0;
        switch_code[8] = 0;
    }
}
