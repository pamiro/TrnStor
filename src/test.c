/*
 * test.c
 *
 *  Created on: Jun 5, 2011
 *      Author: Pavel Mironchyk
 */

#include <stdio.h>

#include "crc8.h"
#include "eeprom.h"


int main(int argc, char* argv[])
{
	int rc = 0;
	if(eeprom_open() == EEPROM_RC_SUCCESS)
	{
		TrnFormat();
		while(1)
		{
			const char *msg1  = "Hello 1",
				       *msg2 = "Hello 234567",
				       *msg3 = "Hello 34";
			TrnBegin();
			TrnWrite((void*)0x100, (void*)msg1,  strlen(msg1));
			TrnWrite((void*)0x150, (void*)msg2, strlen(msg2));
			TrnWrite((void*)0x200, (void*)msg3, strlen(msg3));

			TrnCommit();
			break;

		}
		eeprom_close();
	}
	else {
		fprintf(stderr, "failed to initialize EEPROM\n");
		rc = 1;
	}
	return rc;
}
