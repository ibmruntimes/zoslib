#ifndef __ISPF_READER_H__
#define __ISPF_READER_H__

#include <stdio.h>
#include "ispf.h"

/*
 * Read ISPF statistics for a PDS member
 * 
 * Parameters:
 *   fp          - FILE pointer to the open PDS
 *   member_name - Name of the member (up to 8 characters)
 *   stats       - Output structure to receive ISPF statistics
 * 
 * Returns:
 *   0 on success
 *   -1 if ISPF stats not available or error occurred
 */
int read_member_ispf_stats(FILE* fp, const char* member_name, struct ispf_stats* stats);

#endif /* __ISPF_READER_H__ */

// Made with Bob
