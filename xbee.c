/*
 * Original author: Stephen Crane
 * https://gist.github.com/jscrane/1853043
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "xbee.h"

static unsigned int bitrate_conversion[][2] = {
    { 50, B50 },
    { 75, B75 },
    { 110, B110 },
    { 134, B134 },
    { 150, B150 },
    { 200, B200 },
    { 300, B300 },
    { 600, B600 },
    { 1200, B1200 },
    { 1800, B1800 },
    { 2400, B2400 },
    { 4800, B4800 },
    { 9600, B9600 },
    { 19200, B19200 },
    { 38400, B38400 }
#if defined(B57600)
    , { 57600, B57600 }
#endif
#if defined(B115200)
    , { 115200, B115200 }
#endif
#if defined(B230400)
    , { 230400, B230400 }
#endif
};

#define SAMPLE_START_BYTE 16

static int convert_bitrate(int speed)
{
    int i;
    for (i = 0; i < sizeof(bitrate_conversion); i++)
        if (bitrate_conversion[i][0] == speed)
            return bitrate_conversion[i][1];
    return -1;
}

int open_device(char *device, int speed)

{
    int s = convert_bitrate(speed);
    struct termios tc;
    int fd = open(device, O_RDWR | O_SYNC | O_NOCTTY);
    if (0 > fd) {
        perror("open");
        return -1;
    }
    if (0 > s) {
        fprintf(stderr, "unknown bitrate %d\n", speed);
        goto fail;
    }
    if (0 > tcgetattr(fd, &tc)) {
        perror("tcgetattr");
        goto fail;
    }
    cfmakeraw(&tc);
    // Set baud rate
    cfsetspeed(&tc, s);
    // 8N1
    tc.c_cflag &= ~PARENB;
    tc.c_cflag &= ~CSTOPB;
    tc.c_cflag &= ~CSIZE;
    tc.c_cflag |= CS8;
#if (defined CRTSCTS)
    // Enable HW flow control
    tc.c_cflag |= CRTSCTS; 
#endif
    if (tcsetattr(fd, TCSANOW, &tc)) {
        perror("tcsetattr");
        goto fail;
    }
    return fd;
fail:
    close(fd);
    return -1;
}

#define START_BYTE 0x7e

int recv_response(int fd, uint8_t *data, uint16_t *n)
{
    uint8_t p = 0, checksum = 0;
    uint16_t off = 0, frame_len;

    for (;;) {
        uint8_t b = 0;
        int r = read(fd, &b, 1);
        if (0 > r) {
            perror("read");
            return -1;
        }
        if (0 == r)
            continue;
        switch (p) {
            case 0:
                if (b == START_BYTE)
                    p++;
                break;

            case 1:
                frame_len = ((uint16_t)b) << 8;
                p++;
                break;
            case 2:
                frame_len += b;
                p++;
                break;
            default:
                checksum += b;
                if (p < frame_len+3) {
                    data[off++] = b;
                    p++;
                    break;
                }
                *n = off;
                return (checksum == 0xff)? 0: -1;
        }
    }
}

static int send_byte(int fd, uint8_t b)
{
    int n = write(fd, &b, 1);
    return n;
}

static int send_frame(int fd, uint8_t api_id, uint8_t *frame, uint16_t frame_len)
{
    static uint8_t frame_id = 1;
    uint8_t checksum = api_id + frame_id;
    int i;

    send_byte(fd, START_BYTE);
    send_byte(fd, (frame_len + 2) / 256);
    send_byte(fd, (frame_len + 2) % 256);
    send_byte(fd, api_id);
    send_byte(fd, frame_id);

    for (i = 0; i < frame_len; i++) {
        uint8_t b = frame[i];
        send_byte(fd, b);
        checksum += b;
    }
    checksum = 0xff - checksum;
    send_byte(fd, checksum);

    if (++frame_id == 0) ++frame_id;

    for (i = 0; i < frame_len; i++) {
        uint8_t b = frame[i];
        send_byte(fd, b);
        checksum += b;
    }
    checksum = 0xff - checksum;
    send_byte(fd, checksum);

    if (++frame_id == 0) ++frame_id;
    return 0;       // FIXME
}

int send_remote_at_command_request(int fd, uint8_t *remote64, char *cmd, uint8_t *data, uint16_t n)
{
    uint8_t frame[256];

    frame[0] = remote64[0];
    frame[1] = remote64[1];
    frame[2] = remote64[2];
    frame[3] = remote64[3];
    frame[4] = remote64[4];
    frame[5] = remote64[5];
    frame[6] = remote64[6];
    frame[7] = remote64[7];
    frame[8] = 0xff;
    frame[9] = 0xfe;
    frame[10] = 0x02;
    frame[11] = cmd[0];
    frame[12] = cmd[1];
    memcpy(frame + 13, data, n);

    send_frame(fd, REMOTE_AT_REQUEST, frame, n + 13);
    return 0;
}

int send_at_command_request(int fd, char *cmd, uint8_t *data, uint16_t n)
{
    uint8_t frame[256];

    frame[0] = cmd[0];
    frame[1] = cmd[1];
    memcpy(frame + 2, data, n);

    send_frame(fd, AT_REQUEST, frame, n + 2);
    return 0;
}


void parse_data (uint8_t *data, uint16_t data_len, struct io_ds_rx *frame) {

    frame->api_id = data[0];
    frame->addr64[0] = data[1];
    frame->addr64[1] = data[2];
    frame->addr64[2] = data[3];
    frame->addr64[3] = data[4];
    frame->addr64[4] = data[5];
    frame->addr64[5] = data[6];
    frame->addr64[6] = data[7];
    frame->addr64[7] = data[8];
    frame->addr16[0] = data[9];
    frame->addr16[1] = data[10];
    frame->rcv_options = data[11];
    frame->num_samples = data[12];
    frame->digital_mask = ((uint16_t) data[13] << 8) | data[14];
    frame->analog_mask = data[15];
    frame->samples = (uint8_t *) malloc(data_len - SAMPLE_START_BYTE); // TODO: Free resourse after use!!!
    if (frame->samples) {
        memcpy((uint8_t*) frame->samples, (uint8_t*) data + SAMPLE_START_BYTE, data_len - SAMPLE_START_BYTE);
    } else {
        perror("parse_data");
        exit (2);
    }
}

