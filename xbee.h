/*
 * Orignal author: Stephen Crane
 * https://gist.github.com/jscrane/1853230
 */
int open_device(char *device, int speed);

#define AT_REQUEST 0x08
#define REMOTE_AT_REQUEST 0x17
#define AT_RESPONSE 0x88
#define REMOTE_AT_RESPONSE 0x97

// Digital channel mask
#define D00 0x0001
#define D01 0x0002
#define D02 0x0004
#define D03 0x0008
#define D04 0x0010
#define D05 0x0020
#define D06 0x0040
#define D07 0x0080
#define D10 0x0400
#define D11 0x0800
#define D12 0x1000

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


struct io_ds_rx {
    uint8_t api_id;         // IO Data Sample Rx Indicator
    uint8_t addr64[8];      // Sender 64-bit (MAC/EUI64) address.
    uint8_t addr16[2];      // Sender 16-bit network address, if known. Set to 0xFFFE if unknown.
    /* 
     * 0x01 - Packet Acknowledged 
     * 0x02 - Packet was a broadcast packet 
     * 0x20 - Packet encrypted with APS encryption 
     * 0x40 - Packet was sent from an end device (if known)
     */
    uint8_t rcv_options;
    uint8_t num_samples;    // Number of sample sets in the payload.
    uint16_t digital_mask;  // Bitmask field that indicates which digital IO lines on the remote have sampling enabled (if any).
    uint8_t analog_mask;    // Bitmask field that indicates which analog IO lines on the remote have sampling enabled (if any).
    /*
     * If the sample set includes any digital IO lines (Digital Channel Mask != 0 ), 
     * then the first two bytes contain samples for all enabled digital IO lines.
     * DIO lines that do not have sampling enabled return 0.
     * Bits in these 2 bytes map the same as they do in the Digital Channels Mask field.
     * If the sample set includes any analog input lines (Analog Channel Mask != 0), 
     * each enabled analog input returns a 2-byte value indicating the A/D measurement of that input. 
     * Analog samples are ordered sequentially from AD0/DIO0 to AD3/DIO3, to the supply voltage.
     */
    uint8_t *samples;       
};


int recv_response(int fd, uint8_t *data, uint16_t *n);

int send_at_command_request(int fd, char *cmd, uint8_t *data, uint16_t n);

int send_remote_at_command_request(int fd, uint8_t *addr64, char *cmd, uint8_t *data, uint16_t n);

void parse_data (uint8_t *data, uint16_t data_len, struct io_ds_rx *frame);
