/////////////////////////////////////////////////////////////////////////////////
//// Licensed Materials - Property of IBM
//// ZOSLIB
//// (C) Copyright IBM Corp. 2022. All Rights Reserved.
//// US Government Users Restricted Rights - Use, duplication
//// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
/////////////////////////////////////////////////////////////////////////////////

#define _AE_BIMODAL 1

#include "zos-base.h"

#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <_Nascii.h>

#define SignalList \
  SigInfo(0,     "Signal 0"), \
  SigInfo(HUP,   "SIGHUP"), \
  SigInfo(INT,   "SIGINT"), \
  SigInfo(ABRT,  "SIGABRT"), \
  SigInfo(ILL,   "SIGILL"), \
  SigInfo(5,     "Signal 5"), \
  SigInfo(6,     "Signal 6"), \
  SigInfo(STOP,  "SIGSTOP"), \
  SigInfo(FPE,   "SIGFPE"), \
  SigInfo(KILL,  "SIGKILL"), \
  SigInfo(10,    "Signal 10"), \
  SigInfo(SEGV,  "SIGSEGV"), \
  SigInfo(12,    "Signal 12"), \
  SigInfo(PIPE,  "SIGPIPE"), \
  SigInfo(ALRM,  "SIGALRM"), \
  SigInfo(TERM,  "SIGTERM"), \
  SigInfo(USR1,  "SIGUSR1"), \
  SigInfo(USR2,  "SIGUSR2"), \
  SigInfo(ABND,  "SIGABND"), \
  SigInfo(CONT,  "SIGCONT"), \
  SigInfo(CHLD,  "SIGCHLD"), \
  SigInfo(TTIN,  "SIGTTIN"), \
  SigInfo(TTOU,  "SIGTTOU"), \
  SigInfo(IO,    "SIGIO"), \
  SigInfo(QUIT,  "SIGQUIT"), \
  SigInfo(TSTP,  "SIGTSTP"), \
  SigInfo(TRAP,  "SIGTRAP"), \
  SigInfo(IOERR, "SIGIOERR"), \
  SigInfo(28, "Signal 28"), \
  SigInfo(29, "Signal 29"), \
  SigInfo(30, "Signal 30"), \
  SigInfo(31, "Signal 31"), \
  SigInfo(32, "Signal 32"), \
  SigInfo(33, "Signal 33"), \
  SigInfo(34, "Signal 34"), \
  SigInfo(35, "Signal 35"), \
  SigInfo(36, "Signal 36"), \
  SigInfo(37, "Signal 37"), \
  SigInfo(DCE, "SIGDCE"), \
  SigInfo(DUMP, "SIGDUMP")

#define SigInfo(N, D) sig##N
enum Sigs {
  SignalList,
  sigTotal
};
#undef SigInfo

#define SigInfo(N, D) D
#pragma convert("IBM-1047")
static char *signalTextEBCDIC[] = {
  SignalList
};
#pragma convert(pop)
#pragma convert("ISO8859-1")
static char *signalTextASCII[] = {
  SignalList
};
#pragma convert(pop)
#undef SigInfo

#define SigInfo(N, D) #N
#pragma convert("IBM-1047")
static const char *signalAbbrevEBCDIC[] = {
  SignalList
};
#pragma convert(pop)
#pragma convert("ISO8859-1")
static const char *signalAbbrevASCII[] = {
  SignalList
};
#pragma convert(pop)
#undef SigInfo

#define lookup(arr, n) (__isASCII()? arr##ASCII[n] : arr##EBCDIC[n])

char *strsignal(int signum) {
  if (signum < sigTotal)
    return lookup(signalText,signum);
  errno=EDOM;
  return NULL;
}

const char *sigdescr_np(int signum) {
  return strsignal(signum);
}

const char *sigabbrev_np(int signum) {
  if (signum < sigTotal)
    return lookup(signalAbbrev,signum);
  errno=EDOM;
  return NULL;
}

size_t strnlen(const char *str, size_t maxlen) {
  char *op1 = (char *)str + maxlen;
  char *op2 = (char *)str;
  asm volatile(" SRST %0,%1\n"
               " jo *-4"
               : "+r"(op1), "+r"(op2)
               : NR("",r0)(0)
               :);
  return op1 - str;

}

char *strpcpy(char *dest, const char *src) {
  char *ptr = strcpy(dest, src);
  return ptr + strlen(ptr);
}

char *strndup(const char *s, size_t n) {
  size_t len = strnlen(s, n);
  char *dupStr = malloc(len + 1);
  if (dupStr != NULL) {
    strncpy(dupStr, s, len);
    dupStr[len] = '\0';
  }
  return dupStr;
}

#if TEST
int main() {
  printf("values: segv=%d total=%d\n", SIGSEGV, sigTotal);
  printf("signal segv: abbrev='%s' str='%s'\n", sigabbrev_np(SIGSEGV), strsignal(SIGSEGV));
}
#endif
