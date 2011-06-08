/*
 * Trivial EEPROM emulator
 *
 *  Created on: Jun 5, 2011
 *      Author: Pavel Mironchyk
 */

#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "eeprom.h"

const char* eeprom_filename = "/tmp/eeprom.bin";
int 		eeprom_fd       = 0;

int eeprom_open()
{
	int err = 0;
	eeprom_fd = open(eeprom_filename, O_CREAT | O_RDWR | O_TRUNC,  S_IRUSR | S_IWUSR );
	err 	  = errno;

	if(eeprom_fd != -1)
	{
		off_t offs = 0;
		/* preallocate */
		offs = lseek(eeprom_fd, 0, SEEK_END);
		err  = errno;
		if(offs == -1) {
			(void)close(eeprom_fd);
			fprintf(stderr, "file : %s, error : %s\n", eeprom_filename, strerror(errno));
			return EEPROM_RC_SYSFAIL;
		}
		else
		{
			if(offs != EEPROM_SIZE)
			{
				int rc = ftruncate(eeprom_fd, EEPROM_SIZE);
				err    = errno;
				if(rc == -1) {
					/* Do not care about syscall here */
					(void)close(eeprom_fd);
					fprintf(stderr, "truncation of file : %s has failed with error : %s\n", eeprom_filename, strerror(err));
					return EEPROM_RC_SYSFAIL;
				}
			}
		}
	}
	else
	{
		fprintf(stderr, "file (%s) open error : %s\n", eeprom_filename, strerror(errno));
		return EEPROM_RC_SYSFAIL;
	}

	return EEPROM_RC_SUCCESS;
}

int eeprom_read(unsigned long offset, void* data, unsigned long size)
{
	int rc = 0, err = 0;
	if((offset + size) > EEPROM_SIZE)
	{
		return EEPROM_RC_FAILURE;
	}

	rc  = lseek(eeprom_fd, offset, SEEK_SET);
	err = errno;
	if(rc == -1)
	{
		fprintf(stderr, "file (%s) seeking (%d) error : %s\n", eeprom_filename, offset, strerror(err));
		return EEPROM_RC_SYSFAIL;
	}

	rc  = read(eeprom_fd, data, size);
	err = errno;
	if(rc == -1)
	{
		fprintf(stderr, "file (%s) writing error : %s\n", eeprom_filename, strerror(errno));
		return EEPROM_RC_SYSFAIL;
	}
	return EEPROM_RC_SUCCESS;
}

int eeprom_write(unsigned long offset, void* data, unsigned long size)
{
	int rc = 0, err = 0;
	if((offset + size) > EEPROM_SIZE)
	{
		return EEPROM_RC_FAILURE;
	}
	rc  = lseek(eeprom_fd, offset, SEEK_SET);
	err = errno;
	if(rc == -1)
	{
		fprintf(stderr, "file (%s) seeking (%d) error : %s\n", eeprom_filename, offset, strerror(err));
		return EEPROM_RC_SYSFAIL;
	}

	rc  = write(eeprom_fd, data, size);
	err = errno;
	if(rc == -1)
	{
		fprintf(stderr, "file (%s) writing error : %s\n", eeprom_filename, strerror(errno));
		return EEPROM_RC_SYSFAIL;
	}

	fsync(eeprom_fd);

	return EEPROM_RC_SUCCESS;
}

int eeprom_close()
{
	int rc = 0, err = 0;
	rc = close(eeprom_fd);
	err = errno;
	if(rc == -1)
	{
		fprintf(stderr, "file (%s) open error : %s\n", eeprom_filename, strerror(errno));
		return EEPROM_RC_FAILURE;
	}
	return EEPROM_RC_SUCCESS;
}

