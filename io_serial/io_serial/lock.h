/*
 * lock.h
 *
 *  Created on: 28 Mar 2011
 *      Author: Sergey Kolotsey
 *      E-Mail: kolotsey@gmail.com
 */

#ifndef LOCK_H_
#define LOCK_H_

#ifndef __WIN32__
#define UUCP_LOCK_DIR "/var/lock"

int uucp_lock( const char *file);
int uucp_unlock(void);

#endif

#endif /* LOCK_H_ */
