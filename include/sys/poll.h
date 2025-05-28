///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

// Some projects expect sys/poll.h but z/OS does not provide it
// So we include poll.h

#ifndef __SYS_POLL_H_
#define ___SYS_POLL_H_

#include <poll.h>

#endif 
