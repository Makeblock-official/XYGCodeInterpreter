/*
 * serial-io.c
 *
 *  Created on: 25.03.2011
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
#include "serial.h"
#include "lock.h"


#ifdef __WIN32__
#define DEFAULT_SERIAL "COM1"
#else
#define DEFAULT_SERIAL "/dev/ttyS0"
#endif
#define DEFAULT_SPEED 115200
#define DEFAULT_STOPBITS 1
#define DEFAULT_DATABITS 8
#define DEFAULT_PARITY SERIAL_PARITY_NONE
#define DEFAULT_PARITY_STRING (DEFAULT_PARITY==SERIAL_PARITY_EVEN? "even" : DEFAULT_PARITY==SERIAL_PARITY_ODD? "odd" : "none")
#define DEFAULT_FLOW_CONTROL SERIAL_FLOW_NONE
#define DEFAULT_FLOW_STRING (DEFAULT_FLOW_CONTROL==SERIAL_FLOW_HARDWARE? "hardware (CTS/RTS)" : DEFAULT_FLOW_CONTROL==SERIAL_FLOW_SOFTWARE? "software (XON/XOFF)" : "none")

static char *program_name="serial-io";
//int log_enabled=0;


void usage( FILE *out){

	fprintf( out,
			"Usage:\n"
			"    %s [argument=value] [SERIAL]\n"
			"Supported arguments are:\n"
			"    SERIAL                  name of the serial port (default %s)\n"
			"    speed=BAUD              baud rate for connection (default %d)\n"
			"    stopbits=1|2            number of stop bits (default %d)\n"
			"    databits=5..8           number of data bits (default %d)\n"
			"    parity=none|odd|even    parity (default %s)\n"
			"    flow=hard|soft|none     flow control (default %s)\n"
			"    init=INIT               init string (default %d,%d,%s,%d)\n"
			, program_name,
			DEFAULT_SERIAL, DEFAULT_SPEED, DEFAULT_STOPBITS, DEFAULT_DATABITS, DEFAULT_PARITY_STRING, DEFAULT_FLOW_STRING,
			DEFAULT_SPEED, DEFAULT_DATABITS, DEFAULT_PARITY_STRING, DEFAULT_STOPBITS
	);
}




int  serial_enum(){
#ifdef __WIN32__
#define MAX_VALUE_NAME 64
	LONG lResult;
	HKEY hKey;
	TCHAR  name_buf[MAX_VALUE_NAME], value_buf[MAX_VALUE_NAME];
    DWORD name_len, value_len, ret;
	int i;

	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hKey);
    if (lResult == ERROR_SUCCESS){
    	i=0;
    	do{
    		name_len = MAX_VALUE_NAME;
            name_buf[0] = 0;
            value_len = MAX_VALUE_NAME;
            value_buf[0] = 0;
            ret = RegEnumValue(hKey, i,
                name_buf,
                &name_len,
                NULL,
                NULL,
                value_buf,
                &value_len);

            if (ret == ERROR_SUCCESS ){
            	value_buf[MAX_VALUE_NAME-1]=0;
                printf("%s\n", value_buf);
            }
            i++;
    	}while(ret == ERROR_SUCCESS);
    	RegCloseKey(hKey);
    }

#else
	DIR* dirp;
	struct dirent* direntp;
	struct stat stat_buf;
	char filename[PATH_MAX];
	struct termios t;
	int fd;

	dirp = opendir("/dev");
	if (dirp != NULL) {
		while((direntp = readdir(dirp))){
			if( 0==strncmp( direntp->d_name, "tty", 3)){
				snprintf( filename, PATH_MAX, "/dev/%s", direntp->d_name);
				filename[PATH_MAX-1]=0;
fprintf( stderr, "Test %s: ", filename);
				if( -1 !=stat( filename, &stat_buf)){
					if( S_ISCHR(stat_buf.st_mode)){
						if( -1!=(fd=open( filename, O_RDONLY | O_NONBLOCK))){
							if ( 0==tcgetattr(fd, &t)){
								printf("%s\n", filename);
							}
							close( fd);
						}
					}else{
fprintf( stderr, "Not a char dev\n");
					}
				}else{
fprintf( stderr, "couldn't stat\n");
				}
			}
		}
		closedir(dirp);
	}

#endif
	return 0;
}





static serial_t *serial=NULL;

void signal_handler( int sig){
	serial_stop( serial);
}


int main( int argc, char *argv[]){
	int i;
	int ret;
	char *tok;
	//serial port settings
	int speed=DEFAULT_SPEED;
	char port[PATH_MAX]=DEFAULT_SERIAL;
	int stopbits=DEFAULT_STOPBITS;
	int databits=DEFAULT_DATABITS;
	int parity=DEFAULT_PARITY;
	int flowcontrol=DEFAULT_FLOW_CONTROL;

	if( argc){
		program_name=argv[0];
		argc--;
		argv++;
	}

	for( i=0; i<argc; i++){
		if(( tok=strchr( argv[i], '='))){
			*tok=0;
			tok++;
			if( 0==strcasecmp( argv[i], "speed") || 0==strcasecmp( argv[i], "baud")){
				speed=atoi( tok);
			}else if( 0==strcasecmp( argv[i], "stopbits")){
				stopbits=atoi( tok);
			}else if( 0==strcasecmp( argv[i], "databits")){
				databits=atoi(tok);
			}else if( 0==strcasecmp( argv[i], "parity")){
				if( 0==strcasecmp( tok, "odd") ||0==strcasecmp( tok, "o")){
					parity=SERIAL_PARITY_ODD;
				}else if( 0==strcasecmp( tok, "even") ||0==strcasecmp( tok, "e")){
					parity=SERIAL_PARITY_EVEN;
				}else if( 0==strcasecmp( tok, "none") ||0==strcasecmp( tok, "n") ||0==strlen(tok)){
					parity=SERIAL_PARITY_NONE;
				}else{
					parity=0;//unknown value
				}
			}else if( 0==strcasecmp( argv[i], "flow")){
				if( 0==strncasecmp( tok, "software", 4)){
					flowcontrol=SERIAL_FLOW_SOFTWARE;
				}else if( 0==strncasecmp( tok, "hardware", 4)){
					flowcontrol=SERIAL_FLOW_HARDWARE;
				}else if( 0==strcasecmp( tok, "none")){
					flowcontrol=SERIAL_FLOW_NONE;
				}else{
					flowcontrol=0;
				}
			}else if( 0==strcasecmp( argv[i], "serial")||  0==strcasecmp( argv[i], "port")){
				strncpy( port, tok, PATH_MAX);
				port[PATH_MAX-1]=0;

			}else if( 0==strcasecmp( argv[i], "init")){
				char *p;

				do{
					if((p=strchr( tok, ','))){
						*p=0;
						p++;
					}
					if( 0==strcasecmp( tok, "odd") ||0==strcasecmp( tok, "o")){
						parity=SERIAL_PARITY_ODD;
					}else if( 0==strcasecmp( tok, "even") ||0==strcasecmp( tok, "e")){
						parity=SERIAL_PARITY_EVEN;
					}else if( 0==strcasecmp( tok, "none") ||0==strcasecmp( tok, "n") ||0==strcasecmp( tok, " ") ||0==strlen(tok)){
						parity=SERIAL_PARITY_NONE;
					}else{
						int b=atoi(tok);
						if( b==1 || b==2){
							stopbits=b;
						}else if( b>=5 && b<=8){
							databits=b;
						}else if( speed_is_valid( b)){
							speed=b;
						}else{
							fprintf( stderr, "Invalid init string. Unknown parameter %s\n", tok);
							return 1;
						}
					}
					tok=p;
				}while( tok);

//			}else if( 0==strncasecmp( argv[1], "log", 4)){
//				if( 0==strncasecmp( tok, "true", 4)){
//					log_enabled=1;
//				}

			}else{
				fprintf( stderr, "Unsupported argument %s\n", argv[i]);
			}

		}else if( 0==strcasecmp( argv[i], "help") || 0==strcasecmp( argv[i], "--help") ||
				  0==strcasecmp( argv[i], "-help") || 0==strcasecmp( argv[i], "-h")){
			usage( stdout);
			return 0;

		}else if(0==strcasecmp( argv[i], "enum")){
			return serial_enum();

		}else{
			//assume its a port name
			strncpy( port, argv[i], PATH_MAX);
			port[PATH_MAX-1]=0;
		}
	}


	//check if speed is valid
	if( !speed_is_valid( speed)){
		fprintf( stderr, "Speed %d is invalid\n", speed);
		return 1;
	}
	//check if stopbits variable is valid
	if( !(stopbits==1 || stopbits==2)){
		fprintf( stderr, "Stop bits number is invalid. Valid numbers are 1 and 2\n");
		return 1;
	}
	//check if databits variable is valid
	if( !(databits>=5 && databits<=8)){
		fprintf( stderr, "Data bits number is invalid. Valid numbers are 5..8\n");
		return 1;
	}
	//check if parity is OK
	if( !(parity==SERIAL_PARITY_EVEN || parity==SERIAL_PARITY_ODD || parity== SERIAL_PARITY_NONE)){
		fprintf( stderr, "Parity is invalid. Parity must be either odd or even or none\n");
		return 1;
	}
	//check if flow control is OK
	if( !(flowcontrol==SERIAL_FLOW_HARDWARE || flowcontrol==SERIAL_FLOW_SOFTWARE || flowcontrol== SERIAL_FLOW_NONE)){
		fprintf( stderr, "Flow control is invalid. Flow control must be either hardware or software or none\n");
		return 1;
	}

#ifdef UUCP_LOCK_DIR
	if( -1==uucp_lock( port)){
		fprintf( stderr, "Could not lock serial port for exclusive access\n");
		return 1;
	}
#endif

	if( NULL==(serial=serial_init())){
		fprintf( stderr, "Memory error\n");
#ifdef UUCP_LOCK_DIR
		uucp_unlock();
#endif
		return 1;
	}

#ifdef __WIN32__

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGABRT, signal_handler);

#else
	{
		struct sigaction act;
		sigset_t set;

		sigemptyset( &set);
		act.sa_flags = 0;
		act.sa_mask = set;
		act.sa_flags = SA_SIGINFO;
		act.sa_handler = SIG_IGN;
		for( i = 1; i < SIGRTMAX; ++i){
			if( i!=SIGTERM ){
				sigaction( i, &act, NULL);
			}
		}
		act.sa_handler = signal_handler;
		sigaction( SIGTERM, &act, NULL);

	}
#endif

	if(-1==serial_open( serial, port, speed, stopbits, databits, parity, flowcontrol)){
		fprintf( stderr, "Could not open serial port: %s\n", serial_error());
		serial_destroy( serial);
#ifdef UUCP_LOCK_DIR
		uucp_unlock();
#endif
		return 1;

	}else{
		ret=0;
		if(-1==serial_loop( serial)){
			fprintf( stderr, "I/O error: %s\n", serial_error());
			ret=1;
		}


		serial_close( serial);
		serial_destroy( serial);

#ifdef UUCP_LOCK_DIR
		uucp_unlock();
#endif
		return ret;
	}
}

