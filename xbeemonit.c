#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>

#include "xbee.h"

#define SERIAL_DEVICE "/dev/ttyUSB0"
#define BAUD_RATE 9600

// https://code.google.com/p/xbee-api/wiki/FAQ
// Maximum payload size for Series 2 XBee is 84 bytes
#define MAX_FRAME_LENGTH 84

int main (int argc, char *argv[]) {
    uint8_t frame_data[MAX_FRAME_LENGTH] = {0};
    uint16_t frame_data_len = 0;

    //memset(frame_data, 0, MAX_FRAME_LENGTH);

    int xb_fd = open_device(SERIAL_DEVICE, BAUD_RATE);
    while (recv_response(xb_fd, frame_data, &frame_data_len) >= 0) {
        struct io_ds_rx rx_data;
        parse_data(frame_data, frame_data_len, &rx_data);
        int i;
        for (i = 0; i < (frame_data_len - 16); i++) {
            printf("%02x ", rx_data.samples[i]);
        }
        usleep(1000);
        printf("\n");
    }

    return 0;
}
