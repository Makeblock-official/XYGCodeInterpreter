/*
 * log.c
 *
 *  Created on: 30.03.2011
 *      Author: Sergey Kolotsey
 *      E-Mail: kolotsey@gmail.com
 */


#include <stdio.h>
#ifdef __WIN32__
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <dirent.h>
#include <syslog.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>

#define FPSFRAMES 100

static uint64_t wmseconds=0;
static int wfps=0;
static uint64_t wfps_array[FPSFRAMES];
static int wfps_pos=0;

static uint64_t rmseconds=0;
static int rfps=0;
static uint64_t rfps_array[FPSFRAMES];
static int rfps_pos=0;




static uint64_t milliseconds(){
	struct timeval tv;
	gettimeofday( &tv, NULL);
	return tv.tv_sec*1000+tv.tv_usec/1000;
}


#ifdef __WIN32__
	static HANDLE fdout=INVALID_HANDLE_VALUE;
	static HANDLE fdin=INVALID_HANDLE_VALUE;
#else
	static int fdout=-1;
	static int fdin=-1;
#endif

static int logopened=0;


#ifdef __WIN32__
	static void log_write( HANDLE *fd, char *buffer){
		DWORD written;
		if( FALSE==WriteFile( *fd, buffer, strlen( buffer), &written, NULL)){
			*fd=INVALID_HANDLE_VALUE;
			fprintf( stderr, "Log write error\n");
		}
	}
#else
	static void log_write( int *fd, char *buffer){
		if( -1==write( *fd, buffer, strlen( buffer))){
			*fd=-1;
			fprintf( stderr, "Log write error: %s\n", strerror( errno));
		}
	}
#endif


static void open_log(){
	wmseconds=milliseconds();
	wfps=0;
	wfps_pos=0;
	memset( wfps_array, 0, sizeof( wfps_array));

	rmseconds=milliseconds();
	rfps=0;
	rfps_pos=0;
	memset( rfps_array, 0, sizeof( rfps_array));

#ifdef __WIN32__
	if(INVALID_HANDLE_VALUE ==(fdout=CreateFile("com-write.txt", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL))){
		fprintf(stderr, "Log open error\n");
	}else{
		log_write( &fdout, "opened\n");
	}
	if(INVALID_HANDLE_VALUE ==(fdin=CreateFile("com-read.txt", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL))){
		fprintf(stderr, "Log open error\n");
	}else{
		log_write( &fdin, "opened\n");
	}
#else
	if( -1==(fdout=open( "com-write.txt", O_WRONLY |O_CREAT |O_TRUNC, S_IRUSR |S_IRGRP |S_IROTH |S_IWUSR |S_IWGRP |S_IWOTH))){
		fprintf(stderr, "Log open error: %s\n", strerror( errno));
	}else{
		log_write( &fdout, "opened\n");
	}
	if( -1==(fdin=open( "com-read.txt", O_WRONLY |O_CREAT |O_TRUNC, S_IRUSR |S_IRGRP |S_IROTH |S_IWUSR |S_IWGRP |S_IWOTH))){
		fprintf(stderr, "Log open error: %s\n", strerror( errno));
	}else{
		log_write( &fdin, "opened\n");
	}
#endif

	logopened=1;
}



void log_on_com_write( unsigned char *buffer, size_t length){
	char buf[2048], *pbuf;
	int i;
	uint64_t m, f;
	float fps;

	if( !logopened){
		open_log();
	}
#ifdef __WIN32__
	if( INVALID_HANDLE_VALUE ==fdout) return;
#else
	if( -1 ==fdout) return;
#endif

	f=0;
	for( i=0; i<FPSFRAMES; i++){
		f+=wfps_array[i];
	}
	m=milliseconds();
	if( f ==0){
		fps=0;
	}else{
		fps=1000.0*(float)FPSFRAMES/((float)f);
	}
	pbuf=buf;
	sprintf( pbuf, "abs=%lld", m);
	pbuf+=strlen( pbuf);
	sprintf( pbuf, ", rel=%lld", m-wmseconds);
	pbuf+=strlen( pbuf);
	sprintf( pbuf, ", fps=%0.1f", fps);
	pbuf+=strlen( pbuf);
	sprintf( pbuf, "len=%d, pkt=", length);
	pbuf+=strlen(pbuf);
	for(i=0; i<length; i++){
		sprintf( pbuf, "%02x", buffer[i] & 0xff);
		pbuf+=2;
	}
	*pbuf++='\n';
	*pbuf++=0;
	wfps_array[wfps_pos]=m-wmseconds;
	wfps_pos++;
	if( wfps_pos>=FPSFRAMES) wfps_pos=0;

	wmseconds=m;

	log_write( &fdout, buf);
}

void log_on_com_read( unsigned char *buffer, size_t length){
	char buf[2048], *pbuf;
	int i;
	uint64_t m, f;
	float fps;

	if( !logopened){
		open_log();
	}
#ifdef __WIN32__
	if( INVALID_HANDLE_VALUE ==fdin) return;
#else
	if( -1 ==fdin) return;
#endif

	f=0;
	for( i=0; i<FPSFRAMES; i++){
		f+=rfps_array[i];
	}
	m=milliseconds();
	if( f ==0){
		fps=0;
	}else{
		fps=1000.0*(float)FPSFRAMES/((float)f);
	}
	pbuf=buf;
	sprintf( pbuf, "abs=%lld", m);
	pbuf+=strlen( pbuf);
	sprintf( pbuf, ", rel=%lld", m-rmseconds);
	pbuf+=strlen( pbuf);
	sprintf( pbuf, ", fps=%0.1f", fps);
	pbuf+=strlen( pbuf);
	sprintf( pbuf, "len=%d, pkt=", length);
	pbuf+=strlen(pbuf);
	for(i=0; i<length; i++){
		sprintf( pbuf, "%02x", buffer[i] & 0xff);
		pbuf+=2;
	}
	*pbuf++='\n';
	*pbuf++=0;
	rfps_array[rfps_pos]=m-rmseconds;
	rfps_pos++;
	if( rfps_pos>=FPSFRAMES) rfps_pos=0;

	rmseconds=m;

	log_write( &fdin, buf);
}


void log_cleanup(){

#ifdef __WIN32__
	if( INVALID_HANDLE_VALUE !=fdout){
		log_write( &fdout, "closed\n");
		CloseHandle( fdout);
	}
	if( INVALID_HANDLE_VALUE !=fdin){
		log_write( &fdin, "closed\n");
		CloseHandle( fdin);
	}

#else
	if( -1 !=fdout){
		log_write( &fdout, "closed\n");
		close( fdin);
	}
	if( -1 !=fdin){
		log_write( &fdin, "closed\n");
		close( fdout);
	}
#endif
}
