/*
 *
 * The simplest transactional storage manager
 * EEPROM devices
 *
 *  Created on: Jun 5, 2011
 *      Author: Pavel Mironchyk
 */

#ifndef TRANSACTIONAL_STORAGE_EEPROM_H_
#define TRANSACTIONAL_STORAGE_EEPROM_H_

#include "eeprom.h"

enum {
	TRN_RC_SUCCESS = 0,
	TRN_RC_FAILURE,        /* Indicates incorrect situation due to wrong parameters */
	TRN_RC_SYSFAIL,        /* Hardware failure */
	TRN_RC_TRX_FAIL,       /* Failure on opening new transaction */
	TRN_RC_TRX_UNFINISHED, /* Transaction is on-going */
	TRN_RC_TRX_DISRUPTED,  /* Transaction commit is disrupted */
	TRN_RC_CORRUPTED,      /* Non-initialized properly of destroy */
};

typedef int TransactionError;

const char* TrnRCToString(TransactionError);

/*
 * Format Storage
 */
TransactionError TrnFormat(void);


/*
 * Initiate Transactional Update
 */
TransactionError TrnBegin(void);

/*
 * Perform writing action
 */
TransactionError TrnWrite(void * destinationAddress, void * data, int size);


/*
 * Commit changes to device
 */
TransactionError TrnCommit(void);


/*
 * Read from device data area
 */
TransactionError TrnDirectRead(void * destinationAddress, void * data, int size);


/*
 * Abort current commit
 */
TransactionError TrnCommitAbort(void);


#endif
