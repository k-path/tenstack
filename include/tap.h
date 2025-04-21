#ifndef TAP_H
#define TAP_H

/* creates and configures a TAP dev */
//int alloc_tap(char *dev);

/* Completes network interface setup with naming options */
int setup_network_if(char *tap_name, int choose_name, char* cidr);

/* Read raw data from TAP device */
int tap_read(int tapfd, unsigned char *buffer, int len);

/* Write raw data to a TAP device */
int tap_write(int tapfd, unsigned char *buffer, int len);

/* Close and clean up a TAP device */
int close_tap(int tapfd);

#endif