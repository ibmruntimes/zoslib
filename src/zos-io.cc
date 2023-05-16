///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#define _AE_BIMODAL 1
#include "zos-base.h"

#include <_Ccsid.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/inotify.h>
#include <utmpx.h>

#ifdef __cplusplus
extern "C" {
#endif

void __console(const void *p_in, int len_i) {
  const unsigned char *p = (const unsigned char *)p_in;
  int len = len_i;
  while (len > 0 && p[len - 1] == 0x15) {
    --len;
  }
  typedef struct wtob {
    unsigned short sz;
    unsigned short flags;
    unsigned char msgarea[130];
  } wtob_t;
  wtob_t *m = (wtob_t *)__malloc31(134);
  while (len > 126) {
    m->sz = 130;
    m->flags = 0x8000;
    memcpy(m->msgarea, p, 126);
    memcpy(m->msgarea + 126, "\x20\x00\x00\x20", 4);
    __asm volatile(" la  0,0 \n"
                   " lr  1,%0 \n"
                   " svc 35 \n"
                   :
                   : "r"(m)
                   : "r0", "r1", "r15");
    p += 126;
    len -= 126;
  }
  if (len > 0) {
    m->sz = len + 4;
    m->flags = 0x8000;
    memcpy(m->msgarea, p, len);
    memcpy(m->msgarea + len, "\x20\x00\x00\x20", 4);
    __asm volatile(" la  0,0 \n"
                   " lr  1,%0 \n"
                   " svc 35 \n"
                   :
                   : "r"(m)
                   : "r0", "r1", "r15");
  }
  free(m);
}

int __console_printf(const char *fmt, ...) {
  va_list ap;
  int len;
  va_start(ap, fmt);
  va_list ap1;
  va_list ap2;
  va_copy(ap1, ap);
  va_copy(ap2, ap);
  int bytes;
  int ccsid;
  int am;
  strlen_ae((const unsigned char *)fmt, &ccsid, strlen(fmt) + 1, &am);
  int mode;
  if (ccsid == 819) {
    mode = __ae_thread_swapmode(__AE_ASCII_MODE);
    bytes = __vsnprintf_a(0, 0, fmt, ap1);
    char buf[bytes + 1];
    len = __vsnprintf_a(buf, bytes + 1, fmt, ap2);
    __a2e_l(buf, len);
    va_end(ap2);
    va_end(ap1);
    va_end(ap);
    if (len > 0)
      __console(buf, len);
  } else {
    mode = __ae_thread_swapmode(__AE_EBCDIC_MODE);
    bytes = __vsnprintf_e(0, 0, fmt, ap1);
    char buf[bytes + 1];
    len = __vsnprintf_e(buf, bytes + 1, fmt, ap2);
    va_end(ap2);
    va_end(ap1);
    va_end(ap);
    if (len > 0)
      __console(buf, len);
  }
  __ae_thread_swapmode(mode);
  return len;
}

int vdprintf(int fd, const char *fmt, va_list ap) {
  int ccsid;
  int am;
  strlen_ae((const unsigned char *)fmt, &ccsid, strlen(fmt) + 1, &am);
  int mode;
  int len;
  int bytes;
  va_list ap1;
  va_list ap2;
  va_copy(ap1, ap);
  va_copy(ap2, ap);
  if (ccsid == 819) {
    mode = __ae_thread_swapmode(__AE_ASCII_MODE);
    bytes = __vsnprintf_a(0, 0, fmt, ap1);
    char buf[bytes + 1];
    len = __vsnprintf_a(buf, bytes + 1, fmt, ap2);
    if (len > 0)
      len = write(fd, buf, len);
  } else {
    mode = __ae_thread_swapmode(__AE_EBCDIC_MODE);
    bytes = __vsnprintf_e(0, 0, fmt, ap1);
    char buf[bytes + 1];
    len = __vsnprintf_e(buf, bytes + 1, fmt, ap2);
    if (len > 0)
      len = write(fd, buf, len);
  }
  __ae_thread_swapmode(mode);
  return len;
}

int dprintf(int fd, const char *fmt, ...) {
  va_list ap;
  int len;
  va_start(ap, fmt);
  va_list ap1;
  va_list ap2;
  va_copy(ap1, ap);
  va_copy(ap2, ap);
  int bytes;
  int ccsid;
  int am;
  strlen_ae((const unsigned char *)fmt, &ccsid, strlen(fmt) + 1, &am);
  int mode;
  if (ccsid == 819) {
    mode = __ae_thread_swapmode(__AE_ASCII_MODE);
    bytes = __vsnprintf_a(0, 0, fmt, ap1);
    char buf[bytes + 1];
    len = __vsnprintf_a(buf, bytes + 1, fmt, ap2);
    va_end(ap2);
    va_end(ap1);
    va_end(ap);
    if (len > 0)
      len = write(fd, buf, len);
  } else {
    mode = __ae_thread_swapmode(__AE_EBCDIC_MODE);
    bytes = __vsnprintf_e(0, 0, fmt, ap1);
    char buf[bytes + 1];
    len = __vsnprintf_e(buf, bytes + 1, fmt, ap2);
    va_end(ap2);
    va_end(ap1);
    va_end(ap);
    if (len > 0)
      len = write(fd, buf, len);
  }
  __ae_thread_swapmode(mode);
  return len;
}

void __dump_title(int fd, const void *addr, size_t len, size_t bw,
                  const char *format, ...) {
  static const unsigned char *atbl = (unsigned char *)"................"
                                                      "................"
                                                      " !\"#$%&'()*+,-./"
                                                      "0123456789:;<=>?"
                                                      "@ABCDEFGHIJKLMNO"
                                                      "PQRSTUVWXYZ[\\]^_"
                                                      "`abcdefghijklmno"
                                                      "pqrstuvwxyz{|}~."
                                                      "................"
                                                      "................"
                                                      "................"
                                                      "................"
                                                      "................"
                                                      "................"
                                                      "................"
                                                      "................";
  static const unsigned char *etbl = (unsigned char *)"................"
                                                      "................"
                                                      "................"
                                                      "................"
                                                      " ...........<(+|"
                                                      "&.........!$*);^"
                                                      "-/.........,%_>?"
                                                      ".........`:#@'=\""
                                                      ".abcdefghi......"
                                                      ".jklmnopqr......"
                                                      ".~stuvwxyz...[.."
                                                      ".............].."
                                                      "{ABCDEFGHI......"
                                                      "}JKLMNOPQR......"
                                                      "\\.STUVWXYZ......"
                                                      "0123456789......";
  const unsigned char *p = (const unsigned char *)addr;
  if (format) {
    va_list ap;
    va_start(ap, format);
    vdprintf(fd, format, ap);
    va_end(ap);
  } else {
    dprintf(fd, "Dump: \"Address: Content in Hexdecimal, ASCII, EBCDIC\"\n");
  }
  unsigned char line[2048];
  const unsigned char *buffer;
  long offset = 0;
  long b = 0;
  size_t sz = 0;
  size_t i;
  int c;
  __auto_ascii _a;
  while (len > 0) {
    sz = (len > (bw - 1)) ? bw : len;
    buffer = p + offset;
    b = 0;
    b += __snprintf_a((char *)line + b, 2048 - b, "%*p:", 16, buffer);
    for (i = 0; i < sz; ++i) {
      if ((i & 3) == 0)
        line[b++] = ' ';
      c = buffer[i];
      line[b++] = "0123456789abcdef"[(0xf0 & c) >> 4];
      line[b++] = "0123456789abcdef"[(0x0f & c)];
    }
    for (; i < bw; ++i) {
      if ((i & 3) == 0)
        line[b++] = ' ';
      line[b++] = ' ';
      line[b++] = ' ';
    }
    line[b++] = ' ';
    line[b++] = '|';
    for (i = 0; i < sz; ++i) {
      c = buffer[i];
      if (c == -1) {
        line[b++] = '*';
      } else {
        line[b++] = atbl[c];
      }
    }
    for (; i < bw; ++i) {
      line[b++] = ' ';
    }
    line[b++] = '|';
    line[b++] = ' ';
    line[b++] = '|';
    for (i = 0; i < sz; ++i) {
      c = buffer[i];
      if (c == -1) {
        line[b++] = '*';
      } else {
        line[b++] = etbl[c];
      }
    }
    for (; i < bw; ++i) {
      line[b++] = ' ';
    }
    line[b++] = '|';
    line[b++] = 0;
    dprintf(fd, "%-.*s\n", b, line);
    offset += sz;
    len -= sz;
  }
}

void __dump(int fd, const void *addr, size_t len, size_t bw) {
  __dump_title(fd, addr, len, bw, 0);
}

#if TRACE_ON // for debugging use

class Fdtype {
  char buffer[64];

public:
  Fdtype(int fd) {
    struct stat st;
    int rc = fstat(fd, &st);
    if (-1 == rc) {
      snprintf(buffer, 64, "fstat %d failed errno is %d", fd, errno);
      return;
    }
    if (S_ISBLK(st.st_mode)) {
      snprintf(buffer, 64, "fd %d is %s", fd, "S_ISBLK");
    } else if (S_ISDIR(st.st_mode)) {
      snprintf(buffer, 64, "fd %d is %s", fd, "S_ISDIR");
    } else if (S_ISCHR(st.st_mode)) {
      snprintf(buffer, 64, "fd %d is %s", fd, "S_ISCHR");
    } else if (S_ISFIFO(st.st_mode)) {
      snprintf(buffer, 64, "fd %d is %s", fd, "S_ISFIFO");
    } else if (S_ISREG(st.st_mode)) {
      snprintf(buffer, 64, "fd %d is %s", fd, "S_ISREG");
    } else if (S_ISLNK(st.st_mode)) {
      snprintf(buffer, 64, "fd %d is %s", fd, "S_ISLNK");
    } else if (S_ISSOCK(st.st_mode)) {
      snprintf(buffer, 64, "fd %d is %s", fd, "S_ISSOCK");
    } else if (S_ISVMEXTL(st.st_mode)) {
      snprintf(buffer, 64, "fd %d is %s", fd, "S_ISVMEXTL");
    } else {
      snprintf(buffer, 64, "fd %d st_mode is x%08x", fd, st.st_mode);
    }
  }
  const char *toString(void) { return buffer; }
};

// TODO(gabylb): replace 1024 and 1025 in this .cc by PATH_MAX...

void __fdinfo(int fd) {
  struct stat st;
  int rc;

  char buf[1024];
  struct tm tm;

  rc = fstat(fd, &st);
  if (-1 == rc) {
    __console_printf("fd %d invalid, errno=%d", fd, errno);
    return;
  }
  if (S_ISBLK(st.st_mode)) {
    __console_printf("fd %d IS_BLK", fd);
  } else if (S_ISDIR(st.st_mode)) {
    __console_printf("fd %d IS_DIR", fd);
  } else if (S_ISCHR(st.st_mode)) {
    __console_printf("fd %d IS_CHR", fd);
  } else if (S_ISFIFO(st.st_mode)) {
    __console_printf("fd %d IS_FIFO", fd);
  } else if (S_ISREG(st.st_mode)) {
    __console_printf("fd %d IS_REG", fd);
  } else if (S_ISLNK(st.st_mode)) {
    __console_printf("fd %d IS_LNK", fd);
  } else if (S_ISSOCK(st.st_mode)) {
    __console_printf("fd %d IS_SOCK", fd);
  } else if (S_ISVMEXTL(st.st_mode)) {
    __console_printf("fd %d IS_VMEXTL", fd);
  }
  __console_printf("fd %d perm %04x\n", fd, 0xffff & st.st_mode);
  __console_printf("fd %d ino %d", fd, st.st_ino);
  __console_printf("fd %d dev %d", fd, st.st_dev);
  __console_printf("fd %d rdev %d", fd, st.st_rdev);
  __console_printf("fd %d nlink %d", fd, st.st_nlink);
  __console_printf("fd %d uid %d", fd, st.st_uid);
  __console_printf("fd %d gid %d", fd, st.st_gid);
  __console_printf("fd %d atime %s", fd,
                   asctime_r(localtime_r(&st.st_atime, &tm), buf));
  __console_printf("fd %d mtime %s", fd,
                   asctime_r(localtime_r(&st.st_mtime, &tm), buf));
  __console_printf("fd %d ctime %s", fd,
                   asctime_r(localtime_r(&st.st_ctime, &tm), buf));
  __console_printf("fd %d createtime %s", fd,
                   asctime_r(localtime_r(&st.st_createtime, &tm), buf));
  __console_printf("fd %d reftime %s", fd,
                   asctime_r(localtime_r(&st.st_reftime, &tm), buf));
  __console_printf("fd %d auditoraudit %d", fd, st.st_auditoraudit);
  __console_printf("fd %d useraudit %d", fd, st.st_useraudit);
  __console_printf("fd %d blksize %d", fd, st.st_blksize);
  __console_printf("fd %d auditid %-.*s", fd, 16, st.st_auditid);
  __console_printf("fd %d ccsid %d", fd, st.st_tag.ft_ccsid);
  __console_printf("fd %d txt %d", fd, st.st_tag.ft_txtflag);
  __console_printf("fd %d blkcnt  %ld", fd, st.st_blocks);
  __console_printf("fd %d genvalue %d", fd, st.st_genvalue);
  __console_printf("fd %d fid %-.*s", fd, 8, st.st_fid);
  __console_printf("fd %d filefmt %d", fd, st.st_filefmt);
  __console_printf("fd %d fspflag2 %d", fd, st.st_fspflag2);
  __console_printf("fd %d seclabel %-.*s", fd, 8, st.st_seclabel);
}

void __perror(const char *str) {
  char buf[1024];
  int err = errno;
  int rc = strerror_r(err, buf, sizeof(buf));
  if (rc == EINVAL) {
    __console_printf("%s: %d is not a valid errno", str, err);
  } else {
    __console_printf("%s: %s", str, buf);
  }
  errno = err;
}

static int __eventinfo(char *buffer, size_t size, short poll_event) {
  size_t bytes = 0;
  if (size > 0 && ((poll_event & POLLRDNORM) == POLLRDNORM)) {
    bytes += snprintf(buffer + bytes, size - bytes, "%s ", "POLLRDNORM");
  }
  if (size > 0 && ((poll_event & POLLRDBAND) == POLLRDBAND)) {
    bytes += snprintf(buffer + bytes, size - bytes, "%s ", "POLLRDBAND");
  }
  if (size > 0 && ((poll_event & POLLWRNORM) == POLLWRNORM)) {
    bytes += snprintf(buffer + bytes, size - bytes, "%s ", "POLLWRNORM");
  }
  if (size > 0 && ((poll_event & POLLWRBAND) == POLLWRBAND)) {
    bytes += snprintf(buffer + bytes, size - bytes, "%s ", "POLLWRBAND");
  }
  if (size > 0 && ((poll_event & POLLIN) == POLLIN)) {
    bytes += snprintf(buffer + bytes, size - bytes, "%s ", "POLLIN");
  }
  if (size > 0 && ((poll_event & POLLPRI) == POLLPRI)) {
    bytes += snprintf(buffer + bytes, size - bytes, "%s ", "POLLPRI");
  }
  if (size > 0 && ((poll_event & POLLOUT) == POLLOUT)) {
    bytes += snprintf(buffer + bytes, size - bytes, "%s ", "POLLOUT");
  }
  if (size > 0 && ((poll_event & POLLERR) == POLLERR)) {
    bytes += snprintf(buffer + bytes, size - bytes, "%s ", "POLLERR");
  }
  if (size > 0 && ((poll_event & POLLHUP) == POLLHUP)) {
    bytes += snprintf(buffer + bytes, size - bytes, "%s ", "POLLHUP");
  }
  if (size > 0 && ((poll_event & POLLNVAL) == POLLNVAL)) {
    bytes += snprintf(buffer + bytes, size - bytes, "%s ", "POLLNVAL");
  }
  return bytes;
}

int __dpoll(void *array, unsigned int count, int timeout) {
  void *reg15 = __uss_base_address()[932 / 4]; // BPX4POL offset is 932
  int rv, rc, rn;
  int inf = (timeout == -1);
  int tid = (int)(pthread_self().__ & 0x7fffffffUL);

  typedef struct pollitem {
    int msg_fd;
    short events;
    short revents;
  } pollitem_t;

  pollitem_t *item;
  int fd_cnt = count & 0x0ffff;
  int msg_cnt = (count >> 16) & 0x0ffff;

  int cnt = 9999;
  if (inf)
    timeout = 60 * 1000;
  const void *argv[] = {&array, &count, &timeout, &rv, &rc, &rn};
  __asm volatile(" basr 14,%0\n"
                 : "+NR:r15"(reg15)
                 : "NR:r1"(&argv)
                 : "r0", "r14");
  if (rv != 0 && rv != -1) {
    int fd_res_cnt = rv & 0x0ffff;
  }
  while (rv == 0 && inf && cnt > 0) {
    char event_msg[128];
    char revent_msg[128];
    __console_printf("%s:%s:%d end tid %d count %08x timeout %d rv %08x rc %d "
                     "timeout count-down %d",
                     __FILE__, __FUNCTION__, __LINE__,
                     (int)(pthread_self().__ & 0x7fffffffUL), count, timeout,
                     rv, rc, cnt);
    pollitem_t *fds = (pollitem_t *)array;
    int i;
    i = 0;
    for (; i < fd_cnt; ++i) {
      if (fds[i].msg_fd != -1) {
        size_t s1 = __eventinfo(event_msg, 128, fds[i].events);
        size_t s2 = __eventinfo(revent_msg, 128, fds[i].revents);
        __console_printf("%s:%s:%d tid:%d ary-i:%d %s %d/0x%04x/0x%04x",
                         __FILE__, __FUNCTION__, __LINE__, tid, i, "fd",
                         fds[i].msg_fd, fds[i].events, fds[i].revents);
        __console_printf(
            "%s:%s:%d tid:%d ary-i:%d %s %d event:%-.*s revent:%-.*s", __FILE__,
            __FUNCTION__, __LINE__, tid, i, "fd", fds[i].msg_fd, s1, event_msg,
            s2, revent_msg);
      }
    }
    for (; i < (fd_cnt + msg_cnt); ++i) {
      if (fds[i].msg_fd != -1) {
        size_t s1 = __eventinfo(event_msg, 128, fds[i].events);
        size_t s2 = __eventinfo(revent_msg, 128, fds[i].revents);
        __console_printf("%s:%s:%d tid:%d ary-i:%d %s %d/0x%04x/0x%04x",
                         __FILE__, __FUNCTION__, __LINE__, tid, i, "msgq",
                         fds[i].msg_fd, fds[i].events, fds[i].revents);
        __console_printf(
            "%s:%s:%d tid:%d ary-i:%d %s %d event:%-.*s revent:%-.*s", __FILE__,
            __FUNCTION__, __LINE__, tid, i, "msgq", fds[i].msg_fd, s1,
            event_msg, s2, revent_msg);
      }
    }
    reg15 = __uss_base_address()[932 / 4]; // BPX4POL offset is 932
    __asm volatile(" basr 14,%0\n"
                   : "+NR:r15"(reg15)
                   : "NR:r1"(&argv)
                   : "r0", "r14");
    --cnt;
  }
  if (-1 == rv) {
    errno = rc;
    __perror("poll");
  }
  return rv;
}

// for debugging use
ssize_t __write(int fd, const void *buffer, size_t sz) {
  void *reg15 = __uss_base_address()[220 / 4]; // BPX4WRT offset is 220
  int rv, rc, rn;
  void *alet = 0;
  unsigned int size = sz;
  const void *argv[] = {&fd, &buffer, &alet, &size, &rv, &rc, &rn};
  __asm volatile(" basr 14,%0\n"
                 : "+NR:r15"(reg15)
                 : "NR:r1"(&argv)
                 : "r0", "r14");
  if (-1 == rv) {
    errno = rc;
    __perror("write");
  }
  if (rv > 0) {
    __console_printf("%s:%s:%d fd %d sz %d return %d type is %s\n", __FILE__,
                     __FUNCTION__, __LINE__, fd, sz, rv, Fdtype(fd).toString());
  } else {
    __console_printf("%s:%s:%d fd %d sz %d return %d errno %d type is %s\n",
                     __FILE__, __FUNCTION__, __LINE__, fd, sz, rv, rc,
                     Fdtype(fd).toString());
  }
  return rv;
}

// for debugging use
ssize_t __read(int fd, void *buffer, size_t sz) {
  void *reg15 = __uss_base_address()[176 / 4]; // BPX4RED offset is 176
  int rv, rc, rn;
  void *alet = 0;
  unsigned int size = sz;
  const void *argv[] = {&fd, &buffer, &alet, &size, &rv, &rc, &rn};
  __asm volatile(" basr 14,%0\n"
                 : "+NR:r15"(reg15)
                 : "NR:r1"(&argv)
                 : "r0", "r14");
  if (-1 == rv) {
    errno = rc;
    __perror("read");
  }
  if (rv > 0) {
    __console_printf("%s:%s:%d fd %d sz %d return %d type is %s\n", __FILE__,
                     __FUNCTION__, __LINE__, fd, sz, rv, Fdtype(fd).toString());
  } else {
    __console_printf("%s:%s:%d fd %d sz %d return %d errno %d type is %s\n",
                     __FILE__, __FUNCTION__, __LINE__, fd, sz, rv, rc,
                     Fdtype(fd).toString());
  }
  return rv;
}

// for debugging use
int __close(int fd) {
  void *reg15 = __uss_base_address()[72 / 4]; // BPX4CLO offset is 72
  int rv = -1, rc = -1, rn = -1;
  const char *fdtype = Fdtype(fd).toString();
  const void *argv[] = {&fd, &rv, &rc, &rn};
  __asm volatile(" basr 14,%0\n"
                 : "+NR:r15"(reg15)
                 : "NR:r1"(&argv)
                 : "r0", "r14");
  if (-1 == rv) {
    errno = rc;
    __perror("close");
  }
  __console_printf("%s:%s:%d fd %d return %d errno %d type was %s\n", __FILE__,
                   __FUNCTION__, __LINE__, fd, rv, rc, fdtype);
  return rv;
}

// for debugging use
int __open(const char *file, int oflag, int mode) asm("@@A00144");
int __open(const char *file, int oflag, int mode) {
  void *reg15 = __uss_base_address()[156 / 4]; // BPX4OPN offset is 156
  int rv, rc, rn, len;
  char name[1024];
  strncpy(name, file, 1024);
  __a2e_s(name);
  len = strlen(name);
  const void *argv[] = {&len, name, &oflag, &mode, &rv, &rc, &rn};
  __asm volatile(" basr 14,%0\n"
                 : "+NR:r15"(reg15)
                 : "NR:r1"(&argv)
                 : "r0", "r14");
  if (-1 == rv) {
    errno = rc;
    __perror("open");
  }
  __console_printf("%s:%s:%d fd %d errno %d open %s (part-1)\n", __FILE__,
                   __FUNCTION__, __LINE__, rv, rc, file);
  __console_printf("%s:%s:%d fd %d oflag %08x mode %08x type is %s (part-2)\n",
                   __FILE__, __FUNCTION__, __LINE__, rv, oflag, mode,
                   Fdtype(rv).toString());
  return rv;
}
#endif // if TRACE_ON - for debugging use

static int return_abspath(char *out, int size, const char *path_file) {
  char buffer[1025];
  char *res = 0;
  if (path_file[0] != '/')
    res = __realpath_a(path_file, buffer);
  return __snprintf_a(out, size, "%s", res ? buffer : path_file);
}

int __find_file_in_path(char *out, int size, const char *envvar,
                        const char *file) {
  char *start = (char *)envvar;
  char path[1025];
  char path_file[1025];
  char *p = path;
  int len = 0;
  struct stat st;
  while (*start && (p < (path + 1024))) {
    if (*start == ':') {
      p = path;
      ++start;
      if (len > 0) {
        for (; len > 0 && path[len - 1] == '/'; --len)
          ;
        __snprintf_a(path_file, 1025, "%-.*s/%s", len, path, file);
        if (0 == __stat_a(path_file, &st)) {
          return return_abspath(out, size, path_file);
        }
        len = 0;
      }
    } else {
      ++len;
      *p++ = *start++;
    }
  }
  if (len > 0) {
    for (; len > 0 && path[len - 1] == '/'; --len)
      ;
    __snprintf_a(path_file, 1025, "%-.*s/%s", len, path, file);
    if (0 == __stat_a(path_file, &st)) {
      return return_abspath(out, size, path_file);
    }
  }
  return 0;
}

int __chgfdccsid(int fd, unsigned short ccsid) {
  attrib_t attr;
  memset(&attr, 0, sizeof(attr));
  attr.att_filetagchg = 1;
  attr.att_filetag.ft_ccsid = ccsid;
  if (ccsid != FT_BINARY) {
    attr.att_filetag.ft_txtflag = 1;
  }
  return __fchattr(fd, &attr, sizeof(attr));
}

int __chgpathccsid(char* pathname, unsigned short ccsid) {
  attrib_t attr;
  memset(&attr, 0, sizeof(attr));
  attr.att_filetagchg = 1;
  attr.att_filetag.ft_ccsid = ccsid;
  if (ccsid != FT_BINARY) {
    attr.att_filetag.ft_txtflag = 1;
  }
  return __chattr(pathname, &attr, sizeof(attr));
}

int __setfdccsid(int fd, int t_ccsid) {
  attrib_t attr;
  memset(&attr, 0, sizeof(attr));
  attr.att_filetagchg = 1;
  attr.att_filetag.ft_txtflag = (t_ccsid >> 16);
  attr.att_filetag.ft_ccsid = (t_ccsid & 0x0ffff);
  return __fchattr(fd, &attr, sizeof(attr));
}

int __copyfdccsid(int sourcefd, int destfd) {
 struct stat src_statsbuf;
 fstat(sourcefd, &src_statsbuf);
 return __setfdccsid(destfd,
  (src_statsbuf.st_tag.ft_txtflag << 16) | src_statsbuf.st_tag.ft_ccsid);
}

int __setfdbinary(int fd) {
  return __chgfdccsid(fd, FT_BINARY);
}

int __setfdtext(int fd) {
  return __chgfdccsid(fd, 819);
}

int __disableautocvt(int fd) {
  struct f_cnvrt req = {SETCVTOFF, 0, 0};
  fcntl(fd, F_CONTROL_CVT, &req);
}

int __chgfdcodeset(int fd, char* codeset) {
  unsigned short ccsid = __toCcsid(codeset);
  if (!ccsid)
    return -1;

  return __chgfdccsid(fd, ccsid);
}

int __getfdccsid(int fd) {
  struct stat st;
  int rc;
  rc = fstat(fd, &st);
  if (rc != 0)
    return -1;
  unsigned short ccsid = st.st_tag.ft_ccsid;
  if (st.st_tag.ft_txtflag) {
    return 65536 + ccsid;
  }
  return ccsid;
}

// Defined in zos.cc, no need to expose it:
extern void __setLogMemoryUsage(bool value);

void __memprintf(const char *format, ...) {
  if (!__doLogMemoryUsage())
    return;

  va_list args;
  va_start(args, format);

  static const char *fname = __getMemoryUsageLogFile();
  static bool isstderr = !strcmp(fname, "stderr");
  static bool isstdout = !strcmp(fname, "stdout");
  static FILE *fp = isstderr ? stderr : \
                    isstdout ? stdout : \
                    fopen(fname, "a+");
  if (!fp) {
    va_end(args);
    perror(fname);
    __setLogMemoryUsage(false);
    return;
  }
  char buf[PATH_MAX*2];
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  fprintf(fp, "MEM pid=%d tid=%d: %s", getpid(), gettid(), buf);
}

// C Library Overrides
//-----------------------------------------------------------------
int __pipe_orig(int [2]) asm("pipe");
int __socketpair_orig(int domain, int type, int protocol, int sv[2]) asm("socketpair");
int __close_orig(int) asm("close");
int __open_orig(const char *filename, int opts, ...) asm("@@A00144");
int __mkstemp_orig(char *) asm("@@A00184");
FILE *__fopen_orig(const char *filename, const char *mode) asm("@@A00246");
int __mkfifo_orig(const char *pathname, mode_t mode) asm("@@A00133");
struct utmpx *__getutxent_orig(void) asm("getutxent");

int utmpxname(char * file) {
  char buf[PATH_MAX];
  size_t file_len = strnlen(file, PATH_MAX - 1);
  memcpy(buf, file, file_len);
  buf[file_len] = '\0';
  __a2e_s(buf);

  return __utmpxname(buf);
}

struct utmpx *__getutxent_ascii(void) {
  utmpx* utmpx_ptr = __getutxent_orig(); 
  if (!utmpx_ptr)
    return utmpx_ptr;

  //TODO: Investigate if it is legal to overwrite the data in utmpx struct members:
  // Currently converting the utmpx string members to ASCII in place.
  __e2a_s(utmpx_ptr->ut_user);
  __e2a_s(utmpx_ptr->ut_id);
  __e2a_s(utmpx_ptr->ut_line);
  __e2a_s(utmpx_ptr->ut_host);

  return utmpx_ptr;
}

int __open_ascii(const char *filename, int opts, ...) {
  va_list ap;
  va_start(ap, opts);
  int perms = va_arg(ap, int);
  struct stat sb;
  int is_new_file = stat(filename, &sb) != 0;
  int fd = __open_orig(filename, opts, perms);
  int old_errno = errno;

  if (fd >= 0) {
    // Tag new files as ASCII (819)
    if (is_new_file) {
      __chgfdccsid(fd, 819);
     /* Calling __chgfdccsid() should not clobber errno. */
     errno = old_errno;
    }
    // Enable auto-conversion of untagged files
    else if (S_ISREG(sb.st_mode)) {
      struct file_tag *t = &sb.st_tag;
      if (t->ft_txtflag == 0 && (t->ft_ccsid == 0 || t->ft_ccsid == 1047) &&
          (opts & O_RDONLY) != 0) {
        if (__file_needs_conversion_init(filename, fd)) {
          struct f_cnvrt cvtreq = {SETCVTON, 0, 1047};
          fcntl(fd, F_CONTROL_CVT, &cvtreq);
          /* Calling fcntl() should not clobber errno. */
          errno = old_errno;
        }
      }
    } else if (isatty(fd)) {
      // tty devices need to have auto convert enabled
      struct file_tag *t = &sb.st_tag;
      if (t->ft_txtflag == 0 && (t->ft_ccsid == 0 || t->ft_ccsid == 1047)) {
          struct f_cnvrt cvtreq = {SETCVTON, 0, 1047};
          fcntl(fd, F_CONTROL_CVT, &cvtreq);
          /* Calling fcntl() should not clobber errno. */
          errno = old_errno;
      }
    }
  }
  va_end(ap);
  return fd;
}

FILE *__fopen_ascii(const char *filename, const char *mode) {
  struct stat sb;
  int is_new_file = stat(filename, &sb) != 0;
  FILE* fp = __fopen_orig(filename, mode);
  int old_errno = errno;

  if (fp) {
    int fd = fileno(fp);
    if (is_new_file) {
      __chgfdccsid(fd, 819);
     errno = old_errno;
    }
    // Enable auto-conversion of untagged files
    else if (S_ISREG(sb.st_mode)) {
      struct file_tag *t = &sb.st_tag;
      if (t->ft_txtflag == 0 && (t->ft_ccsid == 0 || t->ft_ccsid == 1047) &&
          strcmp(mode, "r") == 0) {
        if (__file_needs_conversion_init(filename, fd)) {
          struct f_cnvrt cvtreq = {SETCVTON, 0, 1047};
          fcntl(fd, F_CONTROL_CVT, &cvtreq);
          /* Calling fcntl() should not clobber errno. */
          errno = old_errno;
        } else {
          // fopen tags untagged files, which enables auto-conversion
          __disableautocvt(fd);
        }
      }
    } else if (isatty(fd)) {
      // tty devices need to have auto convert enabled
      struct file_tag *t = &sb.st_tag;
      if (t->ft_txtflag == 0 && (t->ft_ccsid == 0 || t->ft_ccsid == 1047)) {
          struct f_cnvrt cvtreq = {SETCVTON, 0, 1047};
          fcntl(fd, F_CONTROL_CVT, &cvtreq);
          /* Calling fcntl() should not clobber errno. */
          errno = old_errno;
      }
    }
  }
  return fp;
}

int __pipe_ascii(int fd[2]) {
  int ret = __pipe_orig(fd);
  if (ret < 0)
    return ret;

  // Default ccsid for new pipes should be ASCII (819)
  if (__chgfdccsid(fd[0], 819) == 0)
    return __chgfdccsid(fd[1], 819);

  return -1;
}

int __mkfifo_ascii(const char *pathname, mode_t mode) {
  int ret = __mkfifo_orig(pathname, mode);
  if (ret < 0)
    return ret;

  __chgpathccsid((char*)pathname, 819);
  return 0; 
}

int __mkstemp_ascii(char * tmpl) {
  int ret = __mkstemp_orig(tmpl);
  if (ret < 0)
    return ret;

  // Default ccsid for new fds should be ASCII (819)
  if (__chgfdccsid(ret, 819) != 0)
    return -1;

  return ret;
}

int __close(int fd) {
  int ret = __close_orig(fd);
  if (ret < 0)
    return ret;

  __fd_close(fd);
  return ret;
}

int __socketpair_ascii(int domain, int type, int protocol, int sv[2]) {
  int ret = __socketpair_orig(domain, type, protocol, sv);
  if (__is_os_level_at_or_above(ZOSLVL_V2R5)) {
    if (ret < 0 || domain != AF_UNIX)
      return ret;

    struct f_cnvrt cvtreq = {SETCVTON, 0, 0};
    if (fcntl(sv[0], F_CONTROL_CVT, &cvtreq) == 0)
      return fcntl(sv[1], F_CONTROL_CVT, &cvtreq);
  } else { 
    return ret;
  }

  return -1;
}

#ifdef __cplusplus
}
#endif
