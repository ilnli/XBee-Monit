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

/*
 * TODOs:
 * 1. Daemonized
 * 2. Monitor child process
 * 3. Logging to syslog
 * 4. Config file
 */

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
    uint16_t prev_digiout = 0x1C1E;

    while (recv_response(xb_fd, frame_data, &frame_data_len) >= 0) {
        struct io_ds_rx rx_data;
        uint16_t curr_digiout = 0x0000;

        parse_data(frame_data, frame_data_len, &rx_data);
        curr_digiout = ((uint16_t) rx_data.samples[0] << 8) | rx_data.samples[1]; // digiout pins value

        if(curr_digiout != prev_digiout) {
            char msg[1024] = {0};

            if ((curr_digiout ^ rx_data.digital_mask) & D1) {
                strcat(msg, "D1 - Fire Alarm\n");
            } 
            if ((curr_digiout ^ rx_data.digital_mask) & D2) {
                strcat(msg, "D2 - PA Alarm\n");
            } 
            if ((curr_digiout ^ rx_data.digital_mask) & D3) {
                strcat(msg, "D3 - Alarm\n");
            } 
            if ((curr_digiout ^ rx_data.digital_mask) & D4) {
                strcat(msg, "D4 - Armed\n");
            } 
            if ((curr_digiout ^ rx_data.digital_mask) & D5) {
                strcat(msg, "D5 - Zoned Locked Out\n");
            } 
            if ((curr_digiout ^ rx_data.digital_mask) & D6) {
                strcat(msg, "D6 - Fault Present\n");
            } 
            if ((curr_digiout ^ rx_data.digital_mask) & D7) {
                strcat(msg, "D7 - Alarm Abort\n");
            }

            printf("Mail will be:\n%s\n", msg);
            mail(to, msg);
        }

        prev_digiout = curr_digiout;
        free(rx_data.samples);
        usleep(1000);
    }
    return 0;
}


