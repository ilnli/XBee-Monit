#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "util.h"

void print_binary(uint8_t n) {
    int i;
    for (i = sizeof(uint8_t)*8; i > 0; i--) { 
        if (n & 0x80) {  // 10000000 - compare and print the left most bit
            printf("1");
        } else {
            printf("0");
        }
        n <<= 1;
    }
}

void sprint_binary(uint16_t n, char *s, int len) {  // len should usually be 8 i.e., byte size
    int i;

    for (i = 0; i < len ; i++) { 
        if (n & 0x8000) {  // 10000000 - compare and print the left most bit
            s[i] = 0x31;  // ASCII 1
        } else {
            s[i] = 0x30;  // ASCII 0
        }
        n <<= 1;
    }
    s[i-1] = '\0';
}



float xbee_volt (uint8_t msb, uint8_t lsb) {
    uint16_t analog_reading = ((uint16_t) msb << 8 ) | lsb;   //Turn the two bytes into an integer value
    printf("analog reading %i\n", analog_reading);
    float volt = ((float) analog_reading / 1023) * 1.75;      //Convert the analog value to a voltage value
    return volt;
}

float calculate_tempC(float v1) { 
    printf("volt: %f\n", v1);
    float temp = 0;
    //calculate temp in C, .75 volts is 25 C. 10mV per degree
    if (v1 < .75) { temp = 25 - ((.75-v1)/.01); } //if below 25 C
    else if (v1 == .75) {temp = 25; }
    else { temp = 25 + ((v1 -.75)/.01); } //if above 25
    //convert to F
    //temp =((temp*9)/5) + 32;
    return temp;
}

int mail(const char *to, const char *msg) { 
    int rc = -1;
    char mail_cmd[1024];

    sprintf(mail_cmd, "/bin/mail -s xbeemonit %s", to);

    FILE *mpipe = popen(mail_cmd, "w");
    if (!mpipe) {
        perror ("mail");
        return rc;
    }
    fprintf(mpipe, "%s\n", msg);
    fprintf(mpipe, ".\n");
    pclose(mpipe);
}
