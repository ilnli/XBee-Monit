#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>

#include "xbee.h"
#include "util.h"

#define SERIAL_DEVICE "/dev/ttyUSB0"
#define BAUD_RATE 9600

#define BYTE_SIZE 8

// https://code.google.com/p/xbee-api/wiki/FAQ
// Maximum payload size for Series 2 XBee is 84 bytes
#define MAX_FRAME_LENGTH 84

int main (int argc, char *argv[]) {
    
    if(argc < 2) {
        printf("ERROR: Mail argument missing!\n");
        exit(1);
    }

    char *to = argv[1];
    uint8_t frame_data[MAX_FRAME_LENGTH] = {0};
    uint16_t frame_data_len = 0;

    //memset(frame_data, 0, MAX_FRAME_LENGTH);

    int xb_fd = open_device(SERIAL_DEVICE, BAUD_RATE);
    uint16_t prev_digiout = 0xFFFF;

    while (recv_response(xb_fd, frame_data, &frame_data_len) >= 0) {
        struct io_ds_rx rx_data;
        uint16_t curr_digiout = 0x0000;

        parse_data(frame_data, frame_data_len, &rx_data);
        curr_digiout = ((uint16_t) rx_data.samples[0] << 8) | rx_data.samples[1]; // digiout pins value

        /* Debug */
        int i;
        char s[BYTE_SIZE];
        for (i = 0; i < (frame_data_len - 16); i++) {
            printf("%02x ", rx_data.samples[i]);
            print_binary(rx_data.samples[i]);
            sprint_binary(rx_data.samples[i], s, BYTE_SIZE);
            printf(" [%s] ", s);
            printf("\n");
        }
        printf("\n");
        printf("Temperature C: %f\n", calculate_tempC(xbee_volt(rx_data.samples[2], rx_data.samples[3])));
        /***/

        if(curr_digiout != prev_digiout) {
            char msg[1024];
            char p[BYTE_SIZE*2+1];
            char c[BYTE_SIZE*2+1];
            sprint_binary(prev_digiout, p, BYTE_SIZE*2+1);
            sprint_binary(curr_digiout, c, BYTE_SIZE*2+1);
            sprintf(msg, "\npre: %s\ncur: %s", p, c);
            printf("Mail will be:\n %s\n", msg);
            mail(to, msg);
        }


        prev_digiout = curr_digiout;
        free(rx_data.samples);
        usleep(1000);
    }
    return 0;
}


