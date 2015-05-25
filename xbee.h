/*
 * Orignal author: Stephen Crane
 * https://gist.github.com/jscrane/1853230
 */
int open_device(char *device, int speed);

#define AT_REQUEST 0x08
#define REMOTE_AT_REQUEST 0x17
#define AT_RESPONSE 0x88
#define REMOTE_AT_RESPONSE 0x97

struct at_response {
    uint8_t api_id, frame_id;
    uint8_t cmd[2];
    uint8_t status;
    uint8_t data[0];
};

struct remote_at_response {
    uint8_t api_id, frame_id;
    uint8_t addr64[8];
    uint8_t addr16[2];
    uint8_t cmd[2];
    uint8_t status;
    uint8_t data[0];
};

int recv_response(int fd, uint8_t *data, uint16_t *n);

int send_at_command_request(int fd, char *cmd, uint8_t *data, uint16_t n);

int send_remote_at_command_request(int fd, uint8_t *addr64, char *cmd, uint8_t *data, uint16_t n);
