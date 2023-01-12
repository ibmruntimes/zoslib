///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#define _AE_BIMODAL 1
#include "zos-char-util.h"
#include "zos-base.h"
#include "zos-io.h"

#include <_Ccsid.h>
#include <fcntl.h>
#include <iconv.h>
#include <mutex>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unordered_map>

#ifdef __cplusplus
extern "C" {
#endif

static int ccsid_guess_buf_size = 4096;

static int utf8scan(const unsigned char *str, unsigned size, char *errmsg,
                    size_t sz) {
  static int byte0_next_state[256] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1,  1,  1,  1,  1,  1,
      1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
      1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
      2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  -1, -1, -1, -1,
      -1, -1, -1, -1};

  unsigned char onebyte;
  int state = 0;
  unsigned int value;
  unsigned char d[4];
  size_t offset = 0;
  int linenum = 1;
  onebyte = str[offset];
  while (onebyte && offset < size) {
    switch (state) {
    case 0:
      state = byte0_next_state[onebyte];
      if (-1 == state) {
        snprintf(errmsg, sz,
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, byte 0x%02X malformed, not one of 0xxxxxxx, "
                 "110xxxxx, 1110xxxx, 11110xxx\n",
                 offset, linenum, onebyte);
        return -1;
      }
      if (state == 0) {
        if (onebyte == 0x0a)
          ++linenum;
        break;
      } else {
        d[0] = onebyte;
      }
      break;
    case 1:
      if ((onebyte & 0xc0) == 0x80) {
        d[1] = onebyte;
        value = (0x1c & d[0] << 6) | (((0x03 & d[0]) << 6) | (0x3f & d[1]));
        if (value < 0x80 || value > 0x7ff) {
          snprintf(errmsg, sz,
                   "Invalid unicode sequence at file offset %lu around line "
                   "%d, 2-byte sequence 0x%02X%02X value U+%04X invalid, range "
                   "out of U+0080 and U+07FF\n",
                   offset, linenum, d[0], d[1], value);
          return -1;
        }
        state = 0;
      } else {
        snprintf(errmsg, sz,
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, 2-byte sequence 0x%02X%02X 2nd byte malformed, not "
                 "110xxxxx-10xxxxxx\n",
                 offset, linenum, d[0], onebyte);
        return -1;
      }
      break;

    case 2:
      if ((onebyte & 0xc0) == 0x80) {
        d[1] = onebyte;
        state = 22;
      } else {
        snprintf(errmsg, sz,
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, 3-byte sequence 0x%02X%02Xxx 2nd byte malformed, not "
                 "1110xxxx-10xxxxxx-xxxxxxxx\n",
                 offset, linenum, d[0], onebyte);
        return -1;
      }
      break;

    case 3:
      if ((onebyte & 0xc0) == 0x80) {
        d[1] = onebyte;
        state = 33;
      } else {
        snprintf(errmsg, sz,
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, 4-byte sequence 0x%02X%02Xxxxx 2nd byte malformed, not "
                 "11110xxx-10xxxxxx-xxxxxxxx-xxxxxxxx\n",
                 offset, linenum, d[0], onebyte);
        return -1;
      }
      break;

    case 33:
      if ((onebyte & 0xc0) == 0x80) {
        d[2] = onebyte;
        state = 333;
      } else {
        snprintf(errmsg, sz,
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, 4-byte sequence 0x%02X%02X%02Xxx 3rd byte malformed, not "
                 "11110xxx-10xxxxxx-10xxxxxxx-xxxxxxxx\n",
                 offset, linenum, d[0], d[1], onebyte);
        return -1;
      }
      break;

    case 22:
      if ((onebyte & 0xc0) == 0x80) {
        d[2] = onebyte;
        value =
            ((0x000f & d[0]) << 12) | ((0x003f & d[1]) << 6) | (0x3f & d[2]);
        if (value < 0x0800 || value > 0x0ffff) {
          snprintf(errmsg, sz,
                   "Invalid unicode sequence at file offset %lu around line "
                   "%d, 3-byte sequence 0x%02X%02X%02X value U+%04X "
                   "invalid, range "
                   "out of U+0800 and U+FFFF\n",
                   offset, linenum, d[0], d[1], d[2], value);
          return -1;
        }
        state = 0;
      } else {
        snprintf(errmsg, sz,
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, 3-byte sequence 0x%02X%02X%02X 3rd byte malformed, not "
                 "11110xxx-10xxxxxx-10xxxxxxx\n",
                 offset, linenum, d[0], d[1], onebyte);
        return -1;
      }
      break;
    case 333:
      if ((onebyte & 0xc0) == 0x80) {
        d[3] = onebyte;
        value = ((0x0007 & d[0]) << 18) | ((0x003f & d[1]) << 12) |
                ((0x003f & d[2]) << 6) | (0x3f & d[3]);
        if (value < 0x010000 || value > 0x010ffff) {
          snprintf(errmsg, sz,
                   "Invalid unicode sequence at file offset %lu around line "
                   "%d, 4-byte sequence 0x%02X%02X%02X%02X value U+%05X "
                   "invalid, range "
                   "out of U+10000 and U+10FFFF\n",
                   offset, linenum, d[0], d[1], d[2], d[3], value);
          return -1;
        }
        state = 0;
      } else {
        snprintf(errmsg, sz,
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, 4-byte sequence 0x%02X%02X%02X%02Xx 4th byte "
                 "malformed, not "
                 "11110xxx-10xxxxxx-10xxxxxxx-10xxxxxx\n",
                 offset, linenum, d[0], d[1], d[2], onebyte);
        return -1;
      }
      break;
    default:
      snprintf(errmsg, sz,
               "Invalid unicode sequence at file offset %lu around line "
               "%d, parser in unknown state %d, byte read 0x%02X\n",
               offset, linenum, state, onebyte);
      return -1;
    }
    ++offset;
    onebyte = str[offset];
  }
  if (state != 0) {
    snprintf(errmsg, sz,
             "Excepted End of File detected at file offset %lu around line "
             "%d, parser in state %d, byte read 0x%02X\n",
             offset, linenum, state, onebyte);
    return -1;
  }
  return 0;
}

void *_convert_e2a(void *dst, const void *src, size_t size) {
  int ccsid;
  int am;
  strlen_ae((unsigned char *)src, &ccsid, size, &am);
  if (ccsid == 819) {
    memcpy(dst, src, size);
    return dst;
  }
  return __convert_one_to_one(__ibm1047_iso88591, dst, size, src);
}

void *_convert_a2e(void *dst, const void *src, size_t size) {
  int ccsid;
  int am;
  strlen_ae((unsigned char *)src, &ccsid, size, &am);
  if (ccsid == 1047) {
    memcpy(dst, src, size);
    return dst;
  }
  return __convert_one_to_one(__iso88591_ibm1047, dst, size, src);
}

int __guess_ue(const void *src, size_t size, char *errmsg, size_t er_size) {
  const int ERR_MG_SIZE = 1024;
  char utf8msg[ERR_MG_SIZE];
  char ebcdicmsg[ERR_MG_SIZE];

  if (utf8scan((unsigned char *)src, size, utf8msg, sizeof(utf8msg)) == 0)
    return 819;

  unsigned e_size = strlen_e((unsigned char *)src, size);
  if (e_size == size)
    return 1047;

  if (errmsg) {
    snprintf(ebcdicmsg, sizeof(ebcdicmsg),
             "Character that does not belong to codepage 1047 was found");

    snprintf(errmsg, er_size, "unicode: %s, ebcdic-1047: %s", utf8msg,
             ebcdicmsg);
  }
  return 65535;
}

extern "C" void __set_ccsid_guess_buf_size(int nbytes) {
  ccsid_guess_buf_size = nbytes;
}

int __guess_fd_ue(int fd, char *errmsg, size_t er_size, int is_new_fd) {
  if (!is_new_fd && lseek(fd, 0, SEEK_SET) < 0) {
    perror("guess_ue:lseek");
    return -1;
  }

  // Only guess first CCSID_GUESS_BUF_SIZE_ENVAR byte of data at most
  if (ccsid_guess_buf_size <= 4096) {
    char buffer[ccsid_guess_buf_size];
    ssize_t bytes = read(fd, buffer, ccsid_guess_buf_size);
    if (bytes < 0) {
      perror("guess_ue:read");
      return -1;
    }
    buffer[bytes] = '\0';
    return __guess_ue(buffer, (size_t)bytes, errmsg, er_size);
  } else {
    char *buffer = (char *)malloc(ccsid_guess_buf_size);
    ssize_t bytes = read(fd, buffer, ccsid_guess_buf_size);
    if (bytes < 0) {
      perror("guess_ue:read");
      free(buffer);
      return -1;
    }
    buffer[bytes] = '\0';
    int ccsid = __guess_ue(buffer, (size_t)bytes, errmsg, er_size);
    free(buffer);
    return ccsid;
  }
}

int __guess_ae(const void *src, size_t size) {
  int ccsid;
  int am;
  strlen_ae((unsigned char *)src, &ccsid, size, &am);
  return ccsid;
}

#if DEBUG_ONLY
size_t __e2a_l(char *bufptr, size_t szLen) {
  int ccsid;
  int am;
  if (0 == bufptr) {
    errno = EINVAL;
    return -1;
  }
  strlen_ae((const unsigned char *)bufptr, &ccsid, szLen, &am);

  if (ccsid == 819) {
    if (!am) {
      /*
      __dump_title(2, bufptr, szLen, 16,
                   "Attempt convert from ASCII to ASCII \n");
      ledump((char *)"Attempt convert from ASCII to ASCII");
      */
      return szLen;
    }
    // return szLen; restore to convert
  }

  __convert_one_to_one(__ibm1047_iso88591, bufptr, szLen, bufptr);
  return szLen;
}

size_t __a2e_l(char *bufptr, size_t szLen) {
  int ccsid;
  int am;
  if (0 == bufptr) {
    errno = EINVAL;
    return -1;
  }
  strlen_ae((const unsigned char *)bufptr, &ccsid, szLen, &am);

  if (ccsid == 1047) {
    if (!am) {
      /*
     __dump_title(2, bufptr, szLen, 16,
                  "Attempt convert from EBCDIC to EBCDIC\n");
     ledump((char *)"Attempt convert from EBCDIC to EBCDIC");
     */
      return szLen;
    }
    // return szLen; restore to convert
  }
  __convert_one_to_one(__iso88591_ibm1047, bufptr, szLen, bufptr);
  return szLen;
}

size_t __e2a_s(char *string) {
  if (0 == string) {
    errno = EINVAL;
    return -1;
  }
  return __e2a_l(string, strlen(string));
}

size_t __a2e_s(char *string) {
  if (0 == string) {
    errno = EINVAL;
    return -1;
  }
  return __a2e_l(string, strlen(string));
}
#endif // #if DEBUG_ONLY

__auto_ascii::__auto_ascii(void) {
  ascii_mode = __isASCII();
  if (ascii_mode == 0)
    __ae_thread_swapmode(__AE_ASCII_MODE);
}

__auto_ascii::~__auto_ascii(void) {
  if (ascii_mode == 0)
    __ae_thread_swapmode(__AE_EBCDIC_MODE);
}

__conv_off::__conv_off(void) {
  convert_state = __ae_autoconvert_state(_CVTSTATE_QUERY);
  __ae_autoconvert_state(_CVTSTATE_OFF);
}

__conv_off::~__conv_off(void) { __ae_autoconvert_state(convert_state); }

class __csConverter {
  int fr_id;
  int to_id;
  char fr_name[_CSNAME_LEN_MAX + 1];
  char to_name[_CSNAME_LEN_MAX + 1];
  iconv_t cv;
  int valid;

public:
  __csConverter(int fr_ccsid, int to_ccsid) : fr_id(fr_ccsid), to_id(to_ccsid) {
    valid = 0;
    if (0 != __toCSName(fr_id, fr_name)) {
      return;
    }
    if (0 != __toCSName(to_id, to_name)) {
      return;
    }
    if (fr_id != -1 && to_id != -1) {
      cv = iconv_open(fr_name, to_name);
      if (cv != (iconv_t)-1) {
        valid = 1;
      }
    }
  }
  int is_valid(void) { return valid; }
  ~__csConverter(void) {
    if (valid)
      iconv_close(cv);
  }
  size_t iconv(char **inbuf, size_t *inbytesleft, char **outbuf,
               size_t *outbytesleft) {
    return ::iconv(cv, inbuf, inbytesleft, outbuf, outbytesleft);
  }
  int conv(char *out, size_t outsize, const char *in, size_t insize) {
    size_t o_len = outsize;
    size_t i_len = insize;
    char *p = (char *)in;
    char *q = out;
    if (i_len == 0)
      return 0;
    int converted = ::iconv(cv, &p, &i_len, &q, &o_len);
    if (converted == -1)
      return -1;
    if (i_len == 0) {
      return outsize - o_len;
    }
    return -1;
  }
};

static __csConverter utf16_to_8(1208, 1200);
static __csConverter utf8_to_16(1200, 1208);

int conv_utf8_utf16(char *out, size_t outsize, const char *in, size_t insize) {
  return utf8_to_16.conv(out, outsize, in, insize);
}

int conv_utf16_utf8(char *out, size_t outsize, const char *in, size_t insize) {
  return utf16_to_8.conv(out, outsize, in, insize);
}

struct IntHash {
  size_t operator()(const int &n) const { return n * 0x54edcfac64d7d667L; }
};

typedef unsigned long fd_attribute;

typedef std::unordered_map<int, fd_attribute, IntHash>::const_iterator cursor_t;

class fdAttributeCache {
  std::unordered_map<int, fd_attribute, IntHash> cache;
  std::mutex access_lock;

public:
  fd_attribute get_attribute(int fd) {
    std::lock_guard<std::mutex> guard(access_lock);
    cursor_t c = cache.find(fd);
    if (c != cache.end()) {
      return c->second;
    }
    return 0;
  }
  void set_attribute(int fd, fd_attribute attr) {
    std::lock_guard<std::mutex> guard(access_lock);
    cache[fd] = attr;
  }
  void unset_attribute(int fd) {
    std::lock_guard<std::mutex> guard(access_lock);
    cache.erase(fd);
  }
  void clear(void) {
    std::lock_guard<std::mutex> guard(access_lock);
    cache.clear();
  }
};

fdAttributeCache fdcache;

void __fd_close(int fd) { fdcache.unset_attribute(fd); }

int __file_needs_conversion(int fd) {
  if (__get_no_tag_read_behaviour() == __NO_TAG_READ_STRICT)
    return 0;
  unsigned long attr = fdcache.get_attribute(fd);
  if (attr == 0x0000000000020000UL) {
    return 1;
  }
  return 0;
}

int __file_needs_conversion_init(const char *name, int fd) {
  char buf[4096];
  off_t off;
  unsigned cnt;
  if (__get_no_tag_ignore_ccsid1047()) {
    struct stat st;
    if (fstat(fd, &st) == 0 && st.st_tag.ft_txtflag == 0 &&
        st.st_tag.ft_ccsid == 1047) {
      return 0;
    }
  }
  notagread_t no_tag_read_behaviour = __get_no_tag_read_behaviour();
  if (no_tag_read_behaviour == __NO_TAG_READ_STRICT)
    return 0;
  if (no_tag_read_behaviour == __NO_TAG_READ_V6) {
    fdcache.set_attribute(fd, 0x0000000000020000UL);
    return 1;
  }
  if (lseek(fd, 1, SEEK_SET) == 1 && lseek(fd, 0, SEEK_SET) == 0) {
    // seekable file (real file)
    cnt = read(fd, buf, 4096);
    off = lseek(fd, 0, SEEK_SET);
    if (off != 0) {
      // introduce an error, because of the offset is no longer valid
      close(fd);
      return 0;
    }
    if (cnt > 8) {
      int ccsid;
      int am;
      unsigned len = strlen_ae((unsigned char *)buf, &ccsid, cnt, &am);
      if (ccsid == 1047 && len == cnt) {
        if (no_tag_read_behaviour == __NO_TAG_READ_DEFAULT_WITHWARNING) {
          if (name) {
            len = strlen(name) + 1;
            char filename[len];
            _convert_e2a(filename, name, len);
            dprintf(2, "Warning: File \"%s\" is untagged and seems to contain "
                       "EBCDIC characters\n", filename);
          } else {
            dprintf(2, "Warning: File (null) is untagged and seems to contain "
                       "EBCDIC characters\n");
          }
        }
        fdcache.set_attribute(fd, 0x0000000000020000UL);
        return 1;
      }
    }       // cnt > 8
  }         // seekable files
  return 0; // not seekable
}

void __set_autocvt_on_fd_stream(int fd, unsigned short ccsid,
                                unsigned char txtflag, int on_untagged_only) {
  struct file_tag tag;

  tag.ft_ccsid = ccsid;
  tag.ft_txtflag = txtflag;
  tag.ft_deferred = 0;
  tag.ft_rsvflags = 0;

  struct f_cnvrt req = {SETCVTON, 0, (short)ccsid};

  if (!on_untagged_only || (!isatty(fd) && 0 == __getfdccsid(fd))) {
    fcntl(fd, F_CONTROL_CVT, &req);
    fcntl(fd, F_SETTAG, &tag);
  }
}

#ifdef __cplusplus
}
#endif
