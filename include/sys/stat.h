///////////////////////////////////////////////////////////////////////////////
////// Licensed Materials - Property of IBM
////// ZOSLIB
////// (C) Copyright IBM Corp. 2022. All Rights Reserved.
////// US Government Users Restricted Rights - Use, duplication
////// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////////

#ifndef _ZOS_STAT_H
#define _ZOS_STAT_H

#include_next <sys/stat.h>

#ifndef S_TYPEISMQ  
#define S_TYPEISMQ  (__x)   (0) /* Test for a message queue */
#define S_TYPEISSEM(__x)    (0) /* Test for a semaphore     */
#define S_TYPEISSHM(__x)    (0) /* Test for a shared memory */
#endif

#endif
