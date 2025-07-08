#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <termios.h>
#define RANDOM_H_IMPLEMENTATION
#include "random.h"

uint8_t leds = 0;
uint8_t run = 1;
#define BAUD_RATE B9600
#define MSG_SIZE 256

float get_temp() {
    return dist_uniformf_dense(-10.0, 45.0, trng_u32, NULL);
}

uint8_t get_status() {
    return leds;
}

void set_led(uint8_t led) {
    leds = leds ^ led;
}

void reset() {
    leds = 0;
}

int setup_tty(char *port_name) {
    int serial_port;
    struct termios tty;

    serial_port = open(port_name, O_RDWR);

    if (serial_port < 0) {
        fprintf(stderr, "open error: %s\n", strerror(errno));
        return -1;
    }

    if (tcgetattr(serial_port, &tty) != 0) {
        fprintf(stderr, "tcgetattr error: %s\n.", strerror(errno));
        return -1;
    }

    tty.c_cflag &= ~PARENB; // Clear parity bit
    tty.c_cflag &= ~CSTOPB; // Clear stop field
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;     // 8 bits per byte
    tty.c_cflag &= CRTSCTS; // Disable hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Read, ignore ctrl lines
    
    tty.c_lflag &= ~ICANON; // Disable cannonical mode
    tty.c_lflag &= ~(ECHO | ECHOE | ECHONL);   // Disable echo (just in case)
    tty.c_lflag &= ~ISIG;   // Disable signal chars interpretation

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow control
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
                            // Disable special handling of received bytes

    tty.c_oflag &= ~(OPOST | ONLCR);

    tty.c_cc[VTIME] = 255; // wait up to 1s, returning as soon as data is rcvd
    tty.c_cc[VMIN] = 0;

    cfsetspeed(&tty, BAUD_RATE);

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        fprintf(stderr, "tcsetattr error: %s\n", strerror(errno));
        return -1;
    }

    return serial_port;
}

void parse_message(int fd, char msg[MSG_SIZE]) {
    char *token;

    token = strtok(msg, " \n");

    while (token) {
        if (strncmp(token, "GET_TEMP", MSG_SIZE) == 0) {
            char temp[7];
            snprintf(temp, sizeof(temp), "%.1f\n", get_temp());

            write(fd, temp, strlen(temp));
        } else if (strncmp(token, "STATUS", MSG_SIZE) == 0) {
            char status[4];
            snprintf(status, sizeof(status), "%d\n", get_status());

            write(fd, status, strlen(status));
        } else if (strncmp(token, "RESET", MSG_SIZE) == 0) {
            reset();
        } else if (strncmp(token, "SET_LED", MSG_SIZE) == 0) {
            int led;
            char *led_code = strtok(NULL, "\n");

            int items = sscanf(led_code, "%d", &led);

            if (items == EOF && errno) {
                fprintf(stderr, "sscanf error: %s\n", strerror(errno));
                write(fd, "ERROR\n", 6);
            }

            if (items == EOF || led > 255) {
                write(fd, "ERROR\n", 6);
            } else {
                set_led((uint8_t) led);
                write(fd, "OK\n", 3);
            }
        } else if (strncmp(token, "EXIT", MSG_SIZE) == 0) {
            run = 0; 
            return;
        } else {
            write(fd, "ERROR\n", 6);
        }
        token = strtok(NULL, " \n");
    }

}

int main(int argc, char **argv) {
    FILE *stream;
    int spfd, num_bytes;
    char read_buf[MSG_SIZE];

    if (argc == 2) {
        stream = stdout;
    } else if (argc > 2) {
        stream = fopen(argv[2], "a");

        if (stream == NULL) {
            fprintf(stderr, "fopen error: %s\n", strerror(errno));
            return 1;
        }
    } else {
        fprintf(stderr, "No arguments provided.\n");
        return 1;
    }

    if ((spfd = setup_tty(argv[1])) < 0) {
        if (argc > 2) {
            fclose(stream);
        }

        return 1;
    }

    while(run) {
        memset(&read_buf, '\0', sizeof(read_buf));
        if((num_bytes = read(spfd, &read_buf, sizeof(read_buf))) > 0) {
            parse_message(spfd, read_buf);
        } else {
            fprintf(stderr, "read error: %s\n", strerror(errno));
            run = 0;
        } 
    }

    close(spfd);
    fclose(stream);
}
