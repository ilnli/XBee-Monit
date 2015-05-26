#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <signal.h>

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
 * 3. Logging to syslog - DONE
 * 4. Config file
 * 5. Filter based on API
 */

void signal_handler(int sig) {
    switch(sig) {
        case SIGTERM:
            syslog(LOG_WARNING, "Received SIGTERM signal.");
            break;
        case SIGINT:
            syslog(LOG_WARNING, "Received SIGINT signal.");
            break;
    }
    exit(sig);
}
int main (int argc, char *argv[]) {
    
    if(argc < 3) {
        printf("ERROR: Mail argument missing (i.e., to, to_sms)!\n");
        exit(1);
    }

    time_t t;
    struct tm tm;
    char *to = argv[1];
    char *to_sms = argv[2];
    uint8_t frame_data[MAX_FRAME_LENGTH] = {0};
    uint16_t frame_data_len = 0;

    int xb_fd = open_device(SERIAL_DEVICE, BAUD_RATE);
    uint16_t prev_digiout = 0x1C1E; // Initialise with the digital mask 

    syslog(LOG_INFO, "Running");

    // Catch signal for logging purpose
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    // Main loop
    while (recv_response(xb_fd, frame_data, &frame_data_len) >= 0) {
        struct io_ds_rx rx_data;
        uint16_t curr_digiout = 0x0000;

        parse_data(frame_data, frame_data_len, &rx_data);
        curr_digiout = ((uint16_t) rx_data.samples[0] << 8) | rx_data.samples[1]; // digiout pins value

        if(curr_digiout != prev_digiout) {
            char msg[1024] = {0};
            uint8_t sms_mail_flag = 0;

            // Get time and date
            t = time(NULL);
            tm = *localtime(&t);
            sprintf(msg, "%d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, 
                tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

            // Check pins
            if ((curr_digiout ^ rx_data.digital_mask) & D1) {
                strcat(msg, "D1 - Fire Alarm");
                syslog(LOG_INFO, "D1 - Fire Alarm");
            } 
            if ((curr_digiout ^ rx_data.digital_mask) & D2) {
                strcat(msg, "D2 - PA Alarm");
                syslog(LOG_WARNING, "D1 - PA Alarm");
                sms_mail_flag = 1;
            } 
            if ((curr_digiout ^ rx_data.digital_mask) & D3) {
                strcat(msg, "D3 - Alarm");
                syslog(LOG_WARNING, "D3 - Alarm");
                sms_mail_flag = 1;
            } 
            if ((curr_digiout ^ rx_data.digital_mask) & D4) {
                strcat(msg, "D4 - Armed");
                syslog(LOG_INFO, "D4 - Armed");
            } 
            if ((curr_digiout ^ rx_data.digital_mask) & D5) {
                strcat(msg, "D5 - Zone Locked Out");
                syslog(LOG_INFO, "D5 - Zone Locked Out");
            } 
            if ((curr_digiout ^ rx_data.digital_mask) & D6) {
                strcat(msg, "D6 - Fault Present");
                syslog(LOG_INFO, "D6 - Fault Present");
            } 
            if ((curr_digiout ^ rx_data.digital_mask) & D7) {
                strcat(msg, "D7 - Confirmed Alarm");
                syslog(LOG_WARNING, "D7 - Confirmed Alarm");
                sms_mail_flag = 1;
            }
            // Pin restore
            if (!(curr_digiout ^ rx_data.digital_mask)) {
                strcat(msg, "System - Restored");
                syslog(LOG_INFO, "System - Restored");
            }

            if(sms_mail_flag) { 
                mail(to_sms, msg);
            }
            mail(to, msg);
        }

        prev_digiout = curr_digiout;
        free(rx_data.samples);
        usleep(1000);
    }
    return 0;
}

