/*
 * lock.c
 *
 *  Created on: 28 Mar 2011
 *      Author: Sergey Kolotsey
 *      E-Mail: kolotsey@gmail.com
 */

#include "lock.h"

#ifdef UUCP_LOCK_DIR

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <signal.h>

/* use HDB UUCP locks  .. see
 * <http://www.faqs.org/faqs/uucp-internals> for details
 */

static char lockname[PATH_MAX] = "";

static int uucp_lockname(const char *dir, const char *file) {
	char *p, *cp;
	struct stat sb;

	if (!dir || *dir == '\0' || stat(dir, &sb) != 0) {
		return -1;
	}

	/* cut-off initial "/dev/" from file-name */
	p = strchr(file + 1, '/');
	p = p ? p + 1 : (char *) file;
	/* replace '/'s with '_'s in what remains (after making a copy) */
	p = cp = strdup(p);
	do {
		if (*p == '/')
			*p = '_';
	} while (*p++);
	/* build lockname */
	snprintf(lockname, sizeof(lockname), "%s/LCK..%s", dir, cp);
	/* destroy the copy */
	free(cp);

	return 0;
}

int uucp_lock(const char *file) {
	int r, fd;
	pid_t pid;
	char buf[16];
	mode_t m;

	uucp_lockname(UUCP_LOCK_DIR, file);

	if (lockname[0] == '\0')
		return 0;

	fd = open(lockname, O_RDONLY);
	if (fd >= 0) {
		r = read(fd, buf, sizeof(buf));
		close(fd);
		/* if r == 4, lock file is binary (old-style) */
		if(r==4){
			int *p=(int *) buf;
			pid=*p;
		}else{
			pid=strtol(buf, NULL, 10);
		}
		if (pid > 0 && kill( pid, 0) < 0 && errno == ESRCH) {
			/* stale lock file */
			//sleep(1);
			unlink(lockname);
		} else {
			lockname[0] = '\0';
			errno = EEXIST;
			return -1;
		}
	}
	/* lock it */
	m = umask(022);
	fd = open(lockname, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (fd < 0) {
		lockname[0] = '\0';
		return -1;
	}
	umask(m);
	snprintf(buf, sizeof(buf), "%04d\n", getpid());
	r = write(fd, buf, strlen(buf));
	close(fd);

	return 0;
}

int uucp_unlock(void) {
	if (lockname[0]){
		unlink(lockname);
	}
	return 0;
}

#endif /* of UUCP_LOCK_DIR */

