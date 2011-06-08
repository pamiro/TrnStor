/*
 *
 * Implementation
 *
 *  Created on: Jun 5, 2011
 *      Author: Pavel Mironchyk
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crc8.h"
#include "trnstor.h"

const char * TrnRCToString(TransactionError err)
{
	switch (err)
	{
	case TRN_RC_SUCCESS:
		return "Success";
	case TRN_RC_FAILURE:
		return "Incorrect situation due to wrong parameters";
	case TRN_RC_SYSFAIL:
		return "Hardware failure";
	case TRN_RC_TRX_FAIL:
		return "Failure on opening new transaction";
	case TRN_RC_TRX_UNFINISHED:
		return "Transaction is on-going";
	case TRN_RC_TRX_DISRUPTED:
		return "Transaction commit is disrupted";
	case TRN_RC_CORRUPTED:
		return "Non-initialized properly of destroy";
	}
	return "No description";
}

/* Storage Configuration */
/* Data */
#define EEPROM_DATA_OFFSET   (0)
#define EEPROM_DATA_SIZE     (EEPROM_SIZE / 2)

/* Log parameters */
#define EEPROM_LOG_OFFSET      (EEPROM_SIZE / 2)
#define EEPROM_LOG_HEADER_PRIMARY   (EEPROM_LOG_OFFSET)
#define EEPROM_LOG_HEADER_SECONDARY (EEPROM_LOG_OFFSET + 128)
#define EEPROM_LOG_HEADER_SIZE (256)
#define EEPROM_LOG_AREA        (EEPROM_LOG_OFFSET + EEPROM_LOG_HEADER_SIZE)
#define EEPROM_WRITE_LIMIT     (0x100)
#define EEPROM_LOG_SIZE        ((EEPROM_SIZE / 2) - EEPROM_LOG_HEADER_SIZE - EEPROM_WRITE_LIMIT)
#define EEPROM_BACKUP_AREA     (EEPROM_LOG_OFFSET + EEPROM_LOG_HEADER_SIZE + EEPROM_LOG_SIZE)

enum {
	TRX_STATE_UNKNOWN = 0,
	TRX_STATE_CLOSED = 0x100,
	TRX_STATE_OPENED = 0x101,
	TRX_STATE_COMMITING  = 0x102
};

#define EEPROM_ENTRY_BACKUP  0x1

#pragma pack(push,1)

const char TRNSignature[4] = "TRNX";

typedef struct trx_log_header_s
{
	char signature[4];
	short int state;
	short int flags;
	unsigned long
	log_entry_offset_head,
	log_entry_offset_tail;
	/* ... */
	unsigned char crc8;
} trx_log_header;

typedef struct trx_log_entry_s {
	unsigned long  offset;
	unsigned long  size;
	unsigned  char data[0];
} trx_log_entry;
#pragma pack(pop)


trx_log_header _log_header;

TransactionError convert_eeprom_error(int rc)
{
	switch(rc) {
	case EEPROM_RC_SUCCESS:
		return TRN_RC_SUCCESS;
	case EEPROM_RC_FAILURE:
		return TRN_RC_FAILURE;
	default:
	case EEPROM_RC_SYSFAIL:
		return TRN_RC_SYSFAIL;
	}
}

/*
 * Fail safe function for updating header
 */
TransactionError update_log_header()
{
	int rc  = 0;
	_log_header.crc8 = compute_crc8((unsigned char*)&_log_header, sizeof(_log_header) - 1);
	/* 1. Update primary header, secondary header is in consistent state */
	rc = eeprom_write(EEPROM_LOG_HEADER_PRIMARY, &_log_header, sizeof(_log_header));
	if(rc != EEPROM_RC_SUCCESS)
	{
		return TRN_RC_SYSFAIL;
	}

	/* 2. Primary header is in consistent state, secondary header is to be updated */
	rc = eeprom_write(EEPROM_LOG_HEADER_SECONDARY, &_log_header, sizeof(_log_header));
	if(rc != EEPROM_RC_SUCCESS)
	{
		return TRN_RC_SYSFAIL;
	}
	return TRN_RC_SUCCESS;
}

/*
 * Fail safe function for retrieving header
 */
TransactionError retrive_log_header()
{
	int rc = 0;
	rc = eeprom_read(EEPROM_LOG_HEADER_PRIMARY, &_log_header, sizeof(_log_header));
	if(rc != EEPROM_RC_SUCCESS)
	{
		return convert_eeprom_error(rc);
	}

	unsigned int crc8 = compute_crc8((unsigned char*)&_log_header, sizeof(_log_header) - 1);
	if((memcmp(_log_header.signature, TRNSignature, 4) != 0) || (crc8 != _log_header.crc8))
	{
		/* Primary header is not ok */
		rc = eeprom_read(EEPROM_LOG_HEADER_SECONDARY, &_log_header, sizeof(_log_header) - 1);
		if(rc != EEPROM_RC_SUCCESS)
		{
			return convert_eeprom_error(rc);
		}

		crc8 = compute_crc8((unsigned char*)&_log_header, sizeof(_log_header));
		if((memcmp(_log_header.signature, TRNSignature, 4) != 0) || (crc8 != _log_header.crc8))
		{
			/* Secondary header is not ok also */
			return TRN_RC_CORRUPTED;
		}
	}
	return TRN_RC_SUCCESS;
}

/*
 * Format Device
 */
TransactionError TrnFormat()
{
	_log_header.state = TRX_STATE_CLOSED;
	memcpy(_log_header.signature, TRNSignature, 4);
	_log_header.log_entry_offset_head = EEPROM_LOG_AREA;
	_log_header.log_entry_offset_tail = EEPROM_LOG_AREA;
	return update_log_header();
}

/*
 * Open new transaction
 */
TransactionError TrnBegin()
{
	int rc = 0;
	/**/
	rc = retrive_log_header();
	if(rc != TRN_RC_SUCCESS)
	{
		return rc;
	}

	if(_log_header.state == TRX_STATE_CLOSED)
	{
		_log_header.state = TRX_STATE_OPENED;
		_log_header.log_entry_offset_tail = EEPROM_LOG_AREA;
		_log_header.log_entry_offset_head = EEPROM_LOG_AREA;

		rc = update_log_header();
		if(rc != TRN_RC_SUCCESS)
		{
			return rc;
		}
	}
	else
	{
		/* Check for state */
		switch(_log_header.state)
		{
		case TRX_STATE_OPENED:
			return TRN_RC_TRX_UNFINISHED;
			break;
		case TRX_STATE_COMMITING:
			return TRN_RC_TRX_DISRUPTED;
			break;
		}
		/* return TRN_RC_TRX_FAIL;*/
	}
	return TRN_RC_SUCCESS;
}

TransactionError TrnWrite(void * destinationAddress, void * data, int size)
{
	int rc = 0;
	unsigned long offset = (unsigned long)destinationAddress;
	trx_log_entry new_entry;

	if((offset + size) > EEPROM_DATA_SIZE)
	{
		/* writing over boundaries */
		return TRN_RC_FAILURE;
	}

	new_entry.offset = offset;
	new_entry.size   = size;

	/* 1. add log entry */
	if(size > EEPROM_WRITE_LIMIT)
	{
		/* Size of operation is over permitted limit */
		return TRN_RC_FAILURE;
	}

	if((size + offset + sizeof(new_entry)) > (EEPROM_LOG_SIZE - _log_header.log_entry_offset_tail))
	{
		/* Transaction log is full */
		return TRN_RC_FAILURE;
	}

	rc = eeprom_write(_log_header.log_entry_offset_tail, &new_entry, sizeof(new_entry));
	_log_header.log_entry_offset_tail += sizeof(new_entry);
	rc = eeprom_write(_log_header.log_entry_offset_tail, data, size);
	_log_header.log_entry_offset_tail += size;


	return update_log_header();
}

TransactionError TrnCommit(void)
{
	int rc = 0;
	unsigned char buffer[EEPROM_WRITE_LIMIT + sizeof(trx_log_entry)];
	trx_log_entry *log_entry = (trx_log_entry *)buffer;

	if(_log_header.state == TRX_STATE_CLOSED)
	{
		return TRN_RC_SUCCESS;
	}

	/* 1. change state if required */
	if(_log_header.state == TRX_STATE_OPENED)
	{
		_log_header.state = TRX_STATE_COMMITING;
		rc = update_log_header();
		if(rc != TRN_RC_SUCCESS)
		{
			return rc;
		}
	}

	while(_log_header.log_entry_offset_head != _log_header.log_entry_offset_tail)
	{
		if((_log_header.flags & EEPROM_ENTRY_BACKUP) == 0)
		{
			/* Make a copy of data for robust recovery would be needed */
			/* Reading address for of data from log */
			rc = eeprom_read(_log_header.log_entry_offset_head, log_entry, sizeof(trx_log_entry));
			/* Reading data address of data area */
			rc = eeprom_read(EEPROM_DATA_OFFSET + log_entry->offset, log_entry->data, log_entry->size);

			/* 2. make a data backup */
			rc = eeprom_write(EEPROM_BACKUP_AREA, log_entry->data, log_entry->size);
			_log_header.flags |= EEPROM_ENTRY_BACKUP;

			rc = update_log_header();
			if(rc != TRN_RC_SUCCESS)
			{
				return rc;
			}
		}

		/* Reading data address of data area */
		rc = eeprom_read(_log_header.log_entry_offset_head + sizeof(*log_entry), log_entry->data, log_entry->size);
		if(rc != EEPROM_RC_SUCCESS)
		{
			return convert_eeprom_error(rc);
		}


		/* 3. serialize an entry */
		rc = eeprom_write(EEPROM_DATA_OFFSET + log_entry->offset, log_entry->data, log_entry->size);
		if(rc != EEPROM_RC_SUCCESS)
		{
			return convert_eeprom_error(rc);
		}

		/* 4. update header */
		_log_header.flags &= (~EEPROM_ENTRY_BACKUP);
		_log_header.log_entry_offset_head += sizeof(trx_log_entry) + log_entry->size;

		rc = update_log_header();
		if(rc != TRN_RC_SUCCESS)
		{
			return rc;
		}

	}
	_log_header.state = TRX_STATE_CLOSED;
	_log_header.log_entry_offset_tail = EEPROM_LOG_AREA;
	_log_header.log_entry_offset_head = EEPROM_LOG_AREA;

	return update_log_header();
}


TransactionError TrnDirectRead(void * destinationAddress, void * data, int size)
{
	int rc = 0;
	unsigned long offset = (unsigned long)destinationAddress;

	if((offset + size) > EEPROM_DATA_SIZE)
	{
		/* reading over boundaries */
		return TRN_RC_SUCCESS;
	}
	rc = eeprom_read(EEPROM_DATA_OFFSET + offset, data, size);
	if(rc != EEPROM_RC_SUCCESS)
	{
		return convert_eeprom_error(rc);
	}
	return rc;
}

/*
 * Abort commit
 */
TransactionError TrnCommitAbort(void)
{
	/* 1. change state if required */
	if(_log_header.state == TRX_STATE_OPENED)
	{
		_log_header.state = TRX_STATE_CLOSED;
		_log_header.log_entry_offset_head = EEPROM_LOG_AREA;
		_log_header.log_entry_offset_tail = EEPROM_LOG_AREA;
		return update_log_header();
	}

	if (_log_header.state == TRX_STATE_CLOSED)
		return TRN_RC_SUCCESS;

	return TRN_RC_TRX_DISRUPTED;
}
