/*
 * debug.h
 *
 *  Created on: 10.09.2010
 *      Author: Sergey Kolotsey
 *      E-mail: kolotsey@gmail.com
 */

#ifndef DEBUG_H_
#define DEBUG_H_


#define __DEBUG_ENABLED
#define __DEBUG_LINEFEED "\n"  //define this parameter to "" if no automatic line breaks are needed
#define __DEBUG_FNAME_LEN "15" //change to the maxumum length of your function names
//#define __DEBUG_USE_QNX_SLOGINFO //comment this line to use stderr in QNX instead of native sloginfo




#ifdef __DEBUG_ENABLED
#ifndef __WIN32__
#include <pthread.h>
#endif

#ifdef __QNXNTO__
#define __DEBUG_PTHREAD_T_SIZE ""
#else
#ifdef __WIN32__
#define __DEBUG_PTHREAD_T_SIZE "l"
#else
#define __DEBUG_PTHREAD_T_SIZE "l"
#endif
#endif



#if defined __DEBUG_USE_QNX_SLOGINFO && defined __QNXNTO__
	//Support for sloginfo in QNX
	#include <sys/slog.h>
	#include <sys/slogcodes.h>
	#define DEBUG(fmt,...) slogf( _SLOG_SETCODE( _SLOG_SYSLOG, 1), _SLOG_DEBUG1, "%" __DEBUG_FNAME_LEN "s (%u): " fmt, __func__, pthread_self(), ##__VA_ARGS__)
#else
	#ifdef __WIN32__
		//No support for thread number in Windows
/*		#define LOG_MESSAGE_MAX_LEN 1024
		#define DEBUG(fmt,...) {\
			char buf[LOG_MESSAGE_MAX_LEN];\
			snprintf( buf, LOG_MESSAGE_MAX_LEN, "%" __DEBUG_FNAME_LEN "s: " fmt __DEBUG_LINEFEED, __func__, ##__VA_ARGS__);\
			buf[LOG_MESSAGE_MAX_LEN-1]=0;\
			OutputDebugString(buf);\
		}*/
		#define DEBUG(fmt,...) fprintf ( stderr, "%" __DEBUG_FNAME_LEN "s (%" __DEBUG_PTHREAD_T_SIZE "u): " fmt __DEBUG_LINEFEED, __func__, GetCurrentThreadId(), ##__VA_ARGS__)
	#else
//#include <syslog.h>
		#define DEBUG(fmt,...) fprintf ( stderr, "%" __DEBUG_FNAME_LEN "s (%" __DEBUG_PTHREAD_T_SIZE "u): " fmt __DEBUG_LINEFEED, __func__, pthread_self(), ##__VA_ARGS__)
		//#define DEBUG(fmt,...) syslog ( LOG_INFO, "%" __DEBUG_FNAME_LEN "s (%" __DEBUG_PTHREAD_T_SIZE "u): " fmt __DEBUG_LINEFEED, __func__, pthread_self(), ##__VA_ARGS__)
	#endif//#ifdef __WIN32__
#endif //#if defined USE_QNX_SLOGINFO && defined __QNXNTO__

/**
 * HEXPRINT prints hexademical interpretation of data
 *
 * @param char *message Message to print before hex data. Pass NULL if you dont want to print any message
 * @param unsigned char *data Data to print
 * @param size_t len Length of data buffer
 */
#define HEXPRINT(message, data, len) {\
	int i;\
	unsigned char *d=(unsigned char *)data;\
	size_t l=len;\
	size_t retlen=len*3+1024, rlen=retlen;\
	char ret[retlen], *r=ret;\
	char fname[ strlen( __func__)+1];\
\
	strncpy( fname, __func__, sizeof( fname));\
	fname[sizeof( fname)-1]=0;\
\
	sprintf( r, "%s%s", (message? message : ""), (message? ": " : ""));\
	rlen-=strlen( r);\
	r+=strlen( r);\
	for(i=0; i<l; i++){\
		snprintf( r, rlen, "%s%02x", i? "-":"", d[i]);\
		r+=i? 3 : 2;\
		rlen-=i? 3 : 2;\
		if( rlen<=0) break;\
	}\
	ret[retlen-1]=0;\
	DEBUG( "%s", ret);\
}





#else
#define DEBUG(fmt,...) {}
#define HEXPRINT (message, data, len) {}
#endif //__DEBUG_ENABLED


#endif /* DEBUG_H_ */
