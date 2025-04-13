#ifndef TAP_H
#define TAP_H

/* creates and configures a TAP dev */
int alloc_tap(char *dev);

/* Completes TAP dev setup (address, route, bring up) */
int configure_tap(char *dev, char *cidr);

/* Completes network interface setup with naming options */
int setup_network_if(char *tap_name, int choose_name, char* cidr);


#endif