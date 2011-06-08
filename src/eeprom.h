/*
 * eemprom.h
 *
 *  Created on: Jun 5, 2011
 *      Author: Pavel Mironchyk
 */

#ifndef EEMPROM_H_
#define EEMPROM_H_

enum {
	EEPROM_RC_SUCCESS = 0,
	EEPROM_RC_FAILURE,
	EEPROM_RC_SYSFAIL
};

#define EEPROM_SIZE  (0x1000)

int eeprom_open();
int eeprom_read(unsigned long offset, void* data, unsigned long size);
int eeprom_write(unsigned long offset, void* data, unsigned long size);
int eeprom_();

#endif /* EEMPROM_H_ */
