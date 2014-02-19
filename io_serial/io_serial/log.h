/*
 * log.h
 *
 *  Created on: 30.03.2011
 *      Author: Sergey Kolotsey
 *      E-Mail: kolotsey@gmail.com
 */

#ifndef LOG_H_
#define LOG_H_


void log_on_com_read( unsigned char *buffer, size_t length);
void log_on_com_write( unsigned char *buffer, size_t length);
void log_cleanup();

#endif /* LOG_H_ */
