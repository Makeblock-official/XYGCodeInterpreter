/*
 * serial.h
 *
 *  Created on: 25.03.2011
 *      Author: Sergey Kolotsey
 *      E-Mail: kolotsey@gmail.com
 */

#ifndef SERIAL_H_
#define SERIAL_H_


#define SERIAL_PARITY_NONE	1
#define SERIAL_PARITY_EVEN	2
#define SERIAL_PARITY_ODD	3

#define SERIAL_FLOW_NONE 1
#define SERIAL_FLOW_SOFTWARE 2
#define SERIAL_FLOW_HARDWARE 3

int speed_is_valid( int speed);
char *serial_error();

typedef struct serial_s serial_t;

int serial_open( serial_t *serial, char *filename, int speed, int stopbits, int databits, int parity, int flowcontrol);
int serial_loop( serial_t *serial);
void serial_stop( serial_t *serial);
void serial_close( serial_t *serial);
serial_t *serial_init();
void serial_destroy(serial_t *serial);

#endif /* SERIAL_H_ */
