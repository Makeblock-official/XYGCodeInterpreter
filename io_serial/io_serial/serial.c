/*
 * serial.c
 *
 *  Created on: 25.03.2011
 *      Author: Sergey Kolotsey
 *      E-Mail: kolotsey@gmail.com
 */



#include <stdio.h>
#include <stdlib.h>
#ifdef __WIN32__
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include "serial.h"
//#include "log.h"

#ifdef __WIN32__
//#define BUFFERED_STDOUT
#define IMMEDIATE_READ
#endif


//extern int log_enabled;

#ifndef max
#define max(a, b) ((a)>(b)? (a) : (b))
#endif


#define ERROR_LENGTH 1024
struct serial_s{
#ifdef __WIN32__
	HANDLE handle;
	OVERLAPPED osReader;
#ifdef IMMEDIATE_READ
	OVERLAPPED comEvent;
#endif
#else
	int fd;
#endif
	char error[ERROR_LENGTH];
	int terminated;
};




static int valid_speed[]={50, 75, 110, 134, 150, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400};
int speed_is_valid( int speed){
	int i;
	for( i=0; i< sizeof( valid_speed)/sizeof(valid_speed[0]); i++){
		if( valid_speed[i]==speed){
			return 1;
		}
	}
	return 0;
}




#ifndef __WIN32__
static speed_t get_speed( int speed){
	switch ( speed) {
		case 0:		return B0;
		case 50:	return B50;
		case 75:	return B75;
		case 110:	return B110;
		case 134:	return B134;
		case 150:	return B150;
		case 300:	return B300;
		case 600:	return B600;
		case 1200:	return B1200;
		case 1800:	return B1800;
		case 2400:	return B2400;
		case 4800:	return B4800;
		case 9600:	return B9600;
		case 19200:	return B19200;
		case 38400:	return B38400;
		case 57600:	return B57600;
		case 115200:return B115200;
		case 230400:return B230400;
		default:	return B0;
	}
}
#endif


char *serial_error( serial_t *serial){
	return serial->error;
}

serial_t *serial_init(){
	serial_t *serial;
	if( NULL !=(serial=calloc( 1, sizeof( serial_t)))){
#ifdef __WIN32__
		serial->handle=INVALID_HANDLE_VALUE;
		serial->osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (serial->osReader.hEvent == NULL){
			free( serial);
			return NULL;
		}
		serial->osReader.Offset=0;
		serial->osReader.OffsetHigh=0;
#ifdef IMMEDIATE_READ
		serial->comEvent.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (serial->comEvent.hEvent == NULL){
			free( serial);
			CloseHandle( serial->osReader.hEvent);
			return NULL;
		}
		serial->comEvent.Offset=0;
		serial->comEvent.OffsetHigh=0;
#endif
#else
		serial->fd=-1;
#endif
		serial->error[0]=0;
		serial->terminated=0;
	}
	return serial;
}

void serial_destroy( serial_t *serial){
#ifdef __WIN32__
	CloseHandle(serial->osReader.hEvent);
#ifdef IMMEDIATE_READ
	CloseHandle(serial->comEvent.hEvent);
#endif
#endif
	free( serial);
}

#ifdef __WIN32__
	static void set_error( serial_t *serial, DWORD code){
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, code, 0, (LPTSTR)serial->error, ERROR_LENGTH, NULL);
		serial->error[ERROR_LENGTH-1]=0;
	}
#else
	static void set_error( serial_t *serial, int code){
		strncpy( serial->error, strerror( code), ERROR_LENGTH);
		serial->error[ERROR_LENGTH-1]=0;
	}
#endif


int serial_open( serial_t *serial, char *filename, int speed, int stopbits, int databits, int parity, int flowcontrol){
#ifdef __WIN32__
	DCB dcb;
	COMMTIMEOUTS timeouts;
	HANDLE hComm;
	char com[128];

	snprintf( com, 128, "\\\\.\\%s", filename);
	com[127]=0;

	hComm = CreateFile( com, GENERIC_READ | GENERIC_WRITE,
                    0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

	if( INVALID_HANDLE_VALUE==hComm){
		set_error( serial, GetLastError());
		return -1;
	}

	if(0==SetupComm( hComm, BUFSIZ, BUFSIZ)){
		set_error( serial, GetLastError());
		return -1;
	}

	timeouts.ReadIntervalTimeout = 1;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 5000;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 100;

	if ( !SetCommTimeouts(hComm, &timeouts)){
		set_error( serial, GetLastError());
		CloseHandle( hComm);
		return -1;
	}

	dcb.DCBlength = sizeof(dcb);
	memset(&dcb, sizeof(dcb), 0);

	if (!GetCommState( hComm, &dcb)){
		set_error( serial, GetLastError());
		CloseHandle( hComm);
		return -1;
	}

	dcb.BaudRate = speed;
	dcb.fParity = (parity == SERIAL_PARITY_NONE) ? FALSE : TRUE;

	dcb.fDsrSensitivity = FALSE;
	dcb.fErrorChar = FALSE;
	dcb.fNull = FALSE;
	dcb.fAbortOnError = FALSE;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.ByteSize = databits;

	switch( flowcontrol){
		case SERIAL_FLOW_HARDWARE:
			dcb.fOutxCtsFlow = TRUE;
			dcb.fOutxDsrFlow = TRUE;
			dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
			dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
			dcb.fOutX = FALSE;
			dcb.fInX = FALSE;
			dcb.fTXContinueOnXoff = FALSE;
			break;

		case SERIAL_FLOW_SOFTWARE:
			dcb.fOutxCtsFlow = FALSE;
			dcb.fOutxDsrFlow = FALSE;
			dcb.fRtsControl = RTS_CONTROL_DISABLE;
			dcb.fDtrControl = DTR_CONTROL_DISABLE;
			dcb.fOutX = TRUE;
			dcb.fInX = TRUE;
			break;

		default:
			dcb.fOutxCtsFlow = FALSE;
			dcb.fOutxDsrFlow = FALSE;
			dcb.fRtsControl = RTS_CONTROL_DISABLE;
			dcb.fDtrControl = DTR_CONTROL_DISABLE;
			dcb.fOutX = FALSE;
			dcb.fInX = FALSE;
			break;
	}

	switch (parity){
		case SERIAL_PARITY_ODD: dcb.Parity = ODDPARITY; break;
		case SERIAL_PARITY_EVEN: dcb.Parity = EVENPARITY; break;
		default: dcb.Parity = NOPARITY; break;//SERIAL_PARITY_NONE: ignore parity
	}

	switch (stopbits){
		case 2: dcb.StopBits = TWOSTOPBITS; break;
		default: dcb.StopBits = ONESTOPBIT; break;//1 stopbit
	}

	if (!SetCommState(hComm, &dcb)){
		set_error( serial, GetLastError());
		CloseHandle( hComm);
		return -1;
	}

	serial->handle=hComm;
	return 0;

#else
	struct termios t;
	int fd;

	if( B0==( speed=get_speed( speed))){
		set_error( serial, EINVAL);
		return -1;
	}

	if ( -1==(fd = open(filename, O_RDWR| O_NONBLOCK))){
		set_error( serial, errno);
		return -1;
	}
	if ( 0!=tcgetattr(fd, &t)){
		close(fd);
		return -1;
	}

	cfmakeraw(&t);
	/* one byte at a time, no timer */
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;

	if ( 0!=cfsetospeed(&t, speed)) {
		set_error( serial, errno);
		close(fd);
		return -1;
	}

	if ( 0!=cfsetispeed(&t, speed)) {
		set_error( serial, errno);
		close(fd);
		return -1;
	}

	switch ( stopbits) {
		case 2:
			t.c_cflag |= CSTOPB;
			break;
		default://1 stop bit
			t.c_cflag &= ~CSTOPB;
			break;
	}

	switch ( databits) {
		case 5:
			t.c_cflag = (t.c_cflag & ~CSIZE) | CS5;
			break;
		case 6:
			t.c_cflag = (t.c_cflag & ~CSIZE) | CS6;
			break;
		case 7:
			t.c_cflag = (t.c_cflag & ~CSIZE) | CS7;
			break;
		default:
			t.c_cflag = (t.c_cflag & ~CSIZE) | CS8;
			break;
	}

	switch ( parity) {
		case SERIAL_PARITY_EVEN:
			t.c_cflag &= ~PARODD;
            t.c_cflag |= PARENB;
            t.c_iflag |= INPCK;
			break;

		case SERIAL_PARITY_ODD:
			t.c_cflag |= PARENB | PARODD;
			t.c_iflag |= INPCK;
			break;

		default://SERIAL_PARITY_NONE: ignore parity
			t.c_cflag &= ~(PARENB | PARODD);
			t.c_iflag |= IGNPAR;
			break;
	}

	switch( flowcontrol){
		case SERIAL_FLOW_HARDWARE:
			t.c_cflag |= CRTSCTS;
			t.c_iflag &= ~(IXON | IXOFF | IXANY);
			break;
		case SERIAL_FLOW_SOFTWARE:
			t.c_cflag &= ~(CRTSCTS);
			t.c_iflag |= IXON | IXOFF;
			break;
		default://NONE
			t.c_cflag &= ~(CRTSCTS);
			t.c_iflag &= ~(IXON | IXOFF | IXANY);
			break;
	}

	t.c_cflag |= CLOCAL;

	if ( 0 !=tcsetattr(fd, TCSANOW, &t)) {
		set_error( serial, errno);
		close(fd);
		return -1;
	}

	serial->fd=fd;
	return 0;
#endif
}








#ifdef __WIN32__


static DWORD WINAPI read_stdin(LPVOID data){
	serial_t *serial=(serial_t *)data;
	HANDLE stdinfd = GetStdHandle(STD_INPUT_HANDLE);
	char buf[BUFSIZ];
	DWORD buf_len, written;
	OVERLAPPED osWrite = {0};
	HANDLE hComm=serial->handle;
	DWORD n;

	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osWrite.hEvent == NULL){
		serial->terminated=2;
		SetEvent( serial->osReader.hEvent);
		return 0;
	}

	while( !serial->terminated){
		BOOL ret = ReadFile(stdinfd, buf, BUFSIZ, &buf_len, NULL);
		if (ret &&  buf_len == 0){
			//EOF, normal exit
			serial->terminated=1;
			break;

		}else if( !ret){
			serial->terminated=1;
			break;

		}else{
			if(buf_len){
//				if( log_enabled) log_on_com_write( (unsigned char *)buf, buf_len);
				if (!WriteFile(hComm, buf, buf_len, &written, &osWrite)) {
					if (GetLastError() != ERROR_IO_PENDING) {
						serial->terminated=2;
						break;

					}else{
						// Write is pending.
						n = WaitForSingleObject(osWrite.hEvent, INFINITE);
						switch(n){
							// OVERLAPPED structure's event has been signaled.
							case WAIT_OBJECT_0:
								if (!GetOverlappedResult(hComm, &osWrite, &written, FALSE)){
									serial->terminated=2;

								}else{
									// Write operation completed successfully.
								}
								break;

							case WAIT_TIMEOUT:
								serial->terminated=2;
								break;

							default:
								serial->terminated=2;
								break;
						}
					}
				}else{
					// WriteFile completed immediately.
				}
			}
		}
	}
	CloseHandle(osWrite.hEvent);
	SetEvent( serial->osReader.hEvent);
#ifdef IMMEDIATE_READ
	SetEvent( serial->comEvent.hEvent);
#endif
	return 0;
}



#ifdef BUFFERED_STDOUT
static HANDLE go;
static HANDLE ghMutex;

typedef struct queue_s{
	struct queue_s *next;
	struct queue_s *prev;
	int len;
	unsigned char buf[0];
}queue_t;
static queue_t *queue_head=NULL;
static queue_t *queue_tail=NULL;
static int queue_size=0;


static DWORD WINAPI write_stdin(LPVOID data) {
	serial_t *serial=(serial_t *)data;
	DWORD written, w;
	queue_t *item;
	HANDLE stdoutfd = GetStdHandle(STD_OUTPUT_HANDLE);

	while( !serial->terminated) {
		WaitForSingleObject( ghMutex, INFINITE);
		while (queue_size == 0 && !serial->terminated) {
			ReleaseMutex(ghMutex);
			WaitForSingleObject(go, INFINITE);
			WaitForSingleObject( ghMutex, INFINITE);
			ResetEvent(go);
		}
		ReleaseMutex(ghMutex);
		if (serial->terminated && queue_size == 0) {
			break;
		}

		item = queue_tail;
		if( item->prev) {
			item->prev->next=NULL;
			queue_tail=item->prev;
		} else {
			queue_head=queue_tail=NULL;
		}
		queue_size--;

		written=0;
		while( item->len>0){
			if( !WriteFile( stdoutfd, item->buf+written, item->len, &w, NULL)){
				set_error( serial, GetLastError());
				serial->terminated = 2;
				break;
			}
			item->len-=w;
			written+=w;
		}
		free( item);
	}
	return 0;
}
#endif

int serial_loop( serial_t *serial){
	HANDLE hComm=serial->handle;
	char buf[BUFSIZ];
	DWORD buf_len, bytes;
	HANDLE stdinfd = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE thread_read;
	DWORD ThreadReadID;

	_setmode( _fileno( stdin ), _O_BINARY );
    _setmode( _fileno( stdout ), _O_BINARY );

#ifdef IMMEDIATE_READ
    DWORD EventMask=0;
	DWORD CommEventMask = EV_BREAK | EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD | EV_RXCHAR | EV_RXFLAG | EV_TXEMPTY;
fprintf(stderr, "set mask\n");
	if(0==SetCommMask(hComm, CommEventMask)){
		set_error( serial, GetLastError());
		return -1;
	}
#endif


	thread_read = CreateThread(NULL, 0, read_stdin, serial, 0, &ThreadReadID);
	if (thread_read == NULL){
		set_error( serial, GetLastError());
		serial->terminated=2;
		return -1;
	}

#ifdef BUFFERED_STDOUT
	HANDLE thread_write;
	DWORD ThreadWriteID;
	queue_t *item;

	go=CreateEvent( NULL, TRUE, FALSE, NULL);
	ghMutex = CreateMutex( NULL, FALSE, NULL);

	if(  (NULL==(thread_write = CreateThread(NULL, 0, write_stdin, serial, 0, &ThreadWriteID)))){
		set_error( serial, GetLastError());
		serial->terminated=2;
		CloseHandle( stdinfd);
		if( WAIT_OBJECT_0==WaitForSingleObject( thread_read, INFINITE)){
			CloseHandle( thread_read);
		}
		return -1;
	}
#else
	HANDLE stdoutfd = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD written, w;
#endif


	while( !serial->terminated){
#ifdef IMMEDIATE_READ
fprintf(stderr, "wait event\n");
		if( !WaitCommEvent( hComm, &EventMask, &serial->comEvent)){
fprintf(stderr, "pending GetLastError() == ERROR_IO_PENDING: %d\n", GetLastError() == ERROR_IO_PENDING);
			if (GetLastError() != ERROR_IO_PENDING || WAIT_OBJECT_0 !=WaitForSingleObject(serial->comEvent.hEvent, INFINITE)){
fprintf(stderr, "wait error\n");
				set_error( serial, GetLastError());
				serial->terminated = 2;
				break;
			}
		}
fprintf(stderr, "wait ok\n");
		if((EventMask & EV_RXCHAR) && (!serial->terminated)){
			DWORD ErrorMask = 0;
			COMSTAT CStat;
			if( !ClearCommError(hComm, &ErrorMask, &CStat)){
				set_error( serial, GetLastError());
				serial->terminated = 2;
				break;
			}
			bytes=min( CStat.cbInQue, BUFSIZ); // bytes in input buffer
#else
		{
			bytes=BUFSIZ;
#endif
			if( !ReadFile(hComm, buf, bytes, &buf_len, &serial->osReader)){
				if ((GetLastError() != ERROR_IO_PENDING) ||
						(WAIT_OBJECT_0 != WaitForSingleObject(serial->osReader.hEvent, INFINITE)) ||
						(!GetOverlappedResult(hComm, &serial->osReader, &buf_len, FALSE))){
					set_error( serial, GetLastError());
					serial->terminated = 2;
					break;
				}
			}


			if( buf_len){
//				if( log_enabled) log_on_com_read( (unsigned char *)buf, buf_len);
#ifdef BUFFERED_STDOUT
				item=malloc( sizeof( item)+buf_len);
				item->len=buf_len;
				memcpy(item->buf, buf, buf_len);
				item->prev=NULL;
				WaitForSingleObject( ghMutex, INFINITE);
				if( queue_size){
					item->next=queue_head;
					queue_head->prev=item;
					queue_head=item;
				}else{
					queue_head=queue_tail=item;
					item->next=NULL;
				}
				queue_size++;
				SetEvent( go);
				ReleaseMutex(ghMutex);
#else
				written=0;
				while( buf_len){
					if( !WriteFile( stdoutfd, buf+written, buf_len, &w, NULL)){
						set_error( serial, GetLastError());
						serial->terminated = 2;
						break;
					}
					buf_len-=w;
					written+=w;
				}
#endif
			}
		}
	}



#ifdef BUFFERED_STDOUT
	WaitForSingleObject( ghMutex, INFINITE);
	SetEvent( go);
	ReleaseMutex(ghMutex);
#endif

	CloseHandle( stdinfd);
	if(WAIT_OBJECT_0==WaitForSingleObject( thread_read, INFINITE)){
		CloseHandle( thread_read);
	}
#ifdef BUFFERED_STDOUT
	if(WAIT_OBJECT_0==WaitForSingleObject( thread_write, INFINITE)){
		CloseHandle( thread_write);
	}
	CloseHandle( ghMutex);
	CloseHandle( go);
#endif
	return serial->terminated==2? -1 : 0;
}

#else

int serial_loop( serial_t *serial){
	char serial_buf[BUFSIZ], stdout_buf[BUFSIZ];
	size_t serial_len=0, stdout_len=0;
	fd_set rfds, wfds;
	int maxfd;
	struct timeval tv;
	int ret;
	int fd=serial->fd;


	while ( !serial->terminated) {
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		maxfd=0;
		if( serial_len){
			FD_SET(fd, &wfds);
			maxfd=max( maxfd, fd);
		}else{
			FD_SET(fileno(stdin), &rfds);
			maxfd=max( maxfd, fileno(stdin));
		}
		if( stdout_len){
			FD_SET(fileno(stdout), &wfds);
			maxfd=max( maxfd, fileno(stdout));
		}else{
			FD_SET(fd, &rfds);
			maxfd=max( maxfd, fd);
		}

		tv.tv_sec = 5;
		tv.tv_usec = 0;

		ret = select(maxfd + 1, &rfds, &wfds, NULL, &tv);
		if( -1==ret){
			if( !serial->terminated){
				set_error( serial, errno);
				return -1;
			}

		}else if( 0==ret){
			//Timeout

		}else{
			size_t w, written;
			if( serial_len){
				if( FD_ISSET( fd, &wfds)){
					written=0;
					while( serial_len>0){
						if( -1==(w=write( fd, serial_buf+written, serial_len))){
							set_error( serial, errno);
							return -1;
						}
						written+=w;
						serial_len-=w;
					}
//					if( log_enabled) log_on_com_write( (unsigned char *)serial_buf, written);
				}
			}else{
				if( FD_ISSET( fileno(stdin), &rfds)){
					if( -1==(serial_len=read( fileno( stdin), serial_buf, BUFSIZ))){
						set_error( serial, EBADF);
						return -1;
					}else if(0==serial_len){
						//stdin is closed unexpectedly
						set_error( serial, EBADF);
						return -1;
					}
				}
			}

			if( stdout_len){
				if( FD_ISSET( fileno(stdout), &wfds)){
					written=0;
					while( stdout_len>0){
						if( -1==(w=write( fileno(stdout), stdout_buf+written, stdout_len))){
							set_error( serial, EBADF);
							return -1;
						}
						written+=w;
						stdout_len-=w;
					}
					stdout_len=0;
				}
			}else{
				if( FD_ISSET( fd, &rfds)){
					if( -1==(stdout_len=read( fd, stdout_buf, BUFSIZ))){
						set_error( serial, errno);
						return -1;
					}
//					if( log_enabled) log_on_com_read( (unsigned char *)stdout_buf, stdout_len);
				}
			}
		}
	}

//	if( log_enabled) log_cleanup();

	return 0;
}

#endif


void serial_stop( serial_t *serial){
#ifdef __WIN32__
	CloseHandle(GetStdHandle(STD_INPUT_HANDLE));
#else
	serial->terminated=1;
	close( fileno( stdin));
#endif
}

void serial_close( serial_t *serial){
#ifdef __WIN32__
	CloseHandle( serial->handle);
	serial->handle=INVALID_HANDLE_VALUE;
	serial->osReader.hEvent=NULL;
#else
	close( serial->fd);
	serial->fd=-1;
#endif
}

