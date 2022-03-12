///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

// APIs that implement Coded Character Sets (ASCII, EBCDIC) processing of
// strings and files, and their conversion.

#ifndef ZOS_CHAR_UTIL_H_
#define ZOS_CHAR_UTIL_H_

#include <_Nascii.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert from EBCDIC to ASCII.
 * \param [out] dst Destination string (must be pre-allocated).
 * \param [in] src Source string.
 * \param [in] size Number of bytes to convert.
 * \return returns destination string.
 */
void *_convert_e2a(void *dst, const void *src, size_t size);

/**
 * Convert from ASCII to EBCDIC
 * \param [out] dst Destination string (must be pre-allocated).
 * \param [in] src Source string.
 * \param [in] size Number of bytes to convert
 * \return returns destination string.
 */
void *_convert_a2e(void *dst, const void *src, size_t size);

/**
 * Guess if string is UTF8 (ASCII) or EBCDIC based
 * on the first CCSID_GUESS_BUF_SIZE_ENVAR of the file
 * associated with the given fd. CCSID_GUESS_BUF_SIZE_ENVAR
 * is default at 4KB.
 * \param [in] fd - open file descriptor to guess.
 * \return guessed CCSID (819 for UTF8, 1047 for EBCDIC; otherwise
 *  65535 for BINARY and, if not NULL, errmsg will contain details).
 */
int __guess_fd_ue(int fd, char *errmsg, size_t er_size, int is_new_fd);

/**
 * Guess if string is UTF8 (ASCII) or EBCDIC.
 * \param [in] src - character string.
 * \param [in] size - number of bytes to analyze.
 * \return guessed CCSID (819 for UTF8, 1047 for EBCDIC; otherwise
 *  65535 for BINARY and, if not NULL, errmsg will contain details).
 */
int __guess_ue(const void *src, size_t size, char *errmsg, size_t er_size);

/**
 * Guess if string is ASCII or EBCDIC.
 * \param [in] src - character string.
 * \param [in] size - number of bytes to analyze.
 * \return guessed CCSID.
 */
int __guess_ae(const void *src, size_t size);

/**
 * Convert string from UTF8 to UTF16
 */
int conv_utf8_utf16(char *, size_t, const char *, size_t);

/**
 * Convert string from UTF16 to UTF8.
 */
int conv_utf16_utf8(char *, size_t, const char *, size_t);

#if DEBUG_ONLY
// TODO(gabylb): should we enable the calls to __dump_title() and ledump()?
/**
 * Convert from EBCDIC to ASCII in place.
 * \param [out] bufptr Buffer to convert.
 * \param [in] szLen Number of characters to convert.
 * \return number of characters converted, or -1 if unsuccessful.
 */
size_t __e2a_l(char *bufptr, size_t szLen);

/**
 * Convert from ASCII to EBCDIC in place.
 * \param [out] bufptr Buffer to convert.
 * \param [in] szLen Number of characters to convert.
 * \return number of characters converted, or -1 if unsuccessful.
 */
size_t __a2e_l(char *bufptr, size_t szLen);

/**
 * Convert null-terminated string from ASCII to EBCDIC in place.
 * \param [out] string String to convert.
 * \return number of characters converted, or -1 if unsuccessful.
 */
size_t __e2a_s(char *string);

/**
 * Convert null-terminate string from EBCDIC to ASCII in place.
 * \param [out] string string to convert.
 * \return number of characters converted, or -1 if unsuccessful.
 */
size_t __a2e_s(char *string);
#endif

/**
 * Sets file descriptor to auto convert.
 * \param [in] fd - file descriptor.
 * \param [in] ccsid - CCSID to auto convert to.
 * \param [in] txtflag - Indicates if ccsid is text.
 * \param [in] on_untagged_only - applies only to untagged
 */
void __set_autocvt_on_fd_stream(int fd, unsigned short ccsid,
                                unsigned char txtflag, int on_untagged_only);

/**
 * Determines if file descriptor needs conversion from EBCDIC to ASCII.
 * Call __file_needs_conversion_init first before calling this function.
 * \param [in] fd file descriptor
 * \return returns 1 if file needs conversion, 0 if not.
 */
int __file_needs_conversion(int fd);

/**
 * Determines if file needs conversion from EBCDIC to ASCII.
 * \param [in] name path to file
 * \param [in] fd file descriptor
 * \return returns 1 if file needs conversion, 0 if not.
 */
int __file_needs_conversion_init(const char *name, int fd);

/**
 * Unsets fd attributes
 * \param [in] fd file descriptor
 */
void __fd_close(int fd);

#define _str_e2a(_str)                                                         \
  ({                                                                           \
    const char *src = (const char *)(_str);                                    \
    int len = strlen(src) + 1;                                                 \
    char *tgt = (char *)alloca(len);                                           \
    (char *)_convert_e2a(tgt, src, len);                                       \
  })

#define _str_a2e(_str)                                                         \
  ({                                                                           \
    const char *src = (const char *)(_str);                                    \
    int len = strlen(src) + 1;                                                 \
    char *tgt = (char *)alloca(len);                                           \
    (char *)_convert_a2e(tgt, src, len);                                       \
  })

#define AEWRAP(_rc, _x)                                                        \
  (__isASCII() ? ((_rc) = (_x), 0)                                             \
               : (__ae_thread_swapmode(__AE_ASCII_MODE), ((_rc) = (_x)),       \
                  __ae_thread_swapmode(__AE_EBCDIC_MODE), 1))

#define AEWRAP_VOID(_x)                                                        \
  (__isASCII() ? ((_x), 0)                                                     \
               : (__ae_thread_swapmode(__AE_ASCII_MODE), (_x),                 \
                  __ae_thread_swapmode(__AE_EBCDIC_MODE), 1))

inline void *__convert_one_to_one(const void *table, void *dst, size_t size,
                                  const void *src) {
  void *rst = dst;
  __asm volatile(" troo 2,%2,1 \n jo *-4 \n"
                 : "+NR:r3"(size), "+NR:r2"(dst), "+r"(src)
                 : "NR:r1"(table)
                 : "r0");
  return rst;
}

#ifdef __cplusplus

class __auto_ascii {
  int ascii_mode;

public:
  __auto_ascii();
  ~__auto_ascii();
};

class __conv_off {
  int convert_state;

public:
  __conv_off();
  ~__conv_off();
};

#endif // ifdef __cplusplus

inline unsigned strlen_ae(const unsigned char *str, int *code_page, int max_len,
                          int *ambiguous) {
  static int last_ccsid = 819;
  static const unsigned char _tab_a[256] __attribute__((aligned(8))) = {
      1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  };
  static const unsigned char _tab_e[256] __attribute__((aligned(8))) = {
      1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
  };
  unsigned long bytes;
  unsigned long code_out;
  const unsigned char *start;

  bytes = max_len;
  code_out = 0;
  start = str;
  __asm volatile(" trte %1,%3,0\n"
                 " jo *-4\n"
                 : "+NR:r3"(bytes), "+NR:r2"(str), "+r"(bytes), "+r"(code_out)
                 : "NR:r1"(_tab_a)
                 :);
  unsigned a_len = str - start;

  bytes = max_len;
  code_out = 0;
  str = start;
  __asm volatile(" trte %1,%3,0\n"
                 " jo *-4\n"
                 : "+NR:r3"(bytes), "+NR:r2"(str), "+r"(bytes), "+r"(code_out)
                 : "NR:r1"(_tab_e)
                 :);
  unsigned e_len = str - start;
  if (a_len > e_len) {
    *code_page = 819;
    last_ccsid = 819;
    *ambiguous = 0;
    return a_len;
  } else if (e_len > a_len) {
    *code_page = 1047;
    last_ccsid = 1047;
    *ambiguous = 0;
    return e_len;
  }
  *code_page = last_ccsid;
  *ambiguous = 1;
  return a_len;
}

inline unsigned strlen_e(const unsigned char *str, unsigned size) {
  static const unsigned char _tab_e[256] __attribute__((aligned(8))) = {
      1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
  };

  unsigned long bytes = size;
  unsigned long code_out = 0;
  const unsigned char *start = str;

  __asm volatile(" trte %1,%3,0\n"
                 " jo *-4\n"
                 : "+NR:r3"(bytes), "+NR:r2"(str), "+r"(bytes), "+r"(code_out)
                 : "NR:r1"(_tab_e)
                 :);

  return str - start;
}

const unsigned char __ibm1047_iso88591[256] __attribute__((aligned(8))) = {
    0x00, 0x01, 0x02, 0x03, 0x9c, 0x09, 0x86, 0x7f, 0x97, 0x8d, 0x8e, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x9d, 0x0a, 0x08, 0x87,
    0x18, 0x19, 0x92, 0x8f, 0x1c, 0x1d, 0x1e, 0x1f, 0x80, 0x81, 0x82, 0x83,
    0x84, 0x85, 0x17, 0x1b, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x05, 0x06, 0x07,
    0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04, 0x98, 0x99, 0x9a, 0x9b,
    0x14, 0x15, 0x9e, 0x1a, 0x20, 0xa0, 0xe2, 0xe4, 0xe0, 0xe1, 0xe3, 0xe5,
    0xe7, 0xf1, 0xa2, 0x2e, 0x3c, 0x28, 0x2b, 0x7c, 0x26, 0xe9, 0xea, 0xeb,
    0xe8, 0xed, 0xee, 0xef, 0xec, 0xdf, 0x21, 0x24, 0x2a, 0x29, 0x3b, 0x5e,
    0x2d, 0x2f, 0xc2, 0xc4, 0xc0, 0xc1, 0xc3, 0xc5, 0xc7, 0xd1, 0xa6, 0x2c,
    0x25, 0x5f, 0x3e, 0x3f, 0xf8, 0xc9, 0xca, 0xcb, 0xc8, 0xcd, 0xce, 0xcf,
    0xcc, 0x60, 0x3a, 0x23, 0x40, 0x27, 0x3d, 0x22, 0xd8, 0x61, 0x62, 0x63,
    0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0xab, 0xbb, 0xf0, 0xfd, 0xfe, 0xb1,
    0xb0, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0xaa, 0xba,
    0xe6, 0xb8, 0xc6, 0xa4, 0xb5, 0x7e, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7a, 0xa1, 0xbf, 0xd0, 0x5b, 0xde, 0xae, 0xac, 0xa3, 0xa5, 0xb7,
    0xa9, 0xa7, 0xb6, 0xbc, 0xbd, 0xbe, 0xdd, 0xa8, 0xaf, 0x5d, 0xb4, 0xd7,
    0x7b, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0xad, 0xf4,
    0xf6, 0xf2, 0xf3, 0xf5, 0x7d, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
    0x51, 0x52, 0xb9, 0xfb, 0xfc, 0xf9, 0xfa, 0xff, 0x5c, 0xf7, 0x53, 0x54,
    0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xb2, 0xd4, 0xd6, 0xd2, 0xd3, 0xd5,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0xb3, 0xdb,
    0xdc, 0xd9, 0xda, 0x9f};

const unsigned char __iso88591_ibm1047[256] __attribute__((aligned(8))) = {
    0x00, 0x01, 0x02, 0x03, 0x37, 0x2d, 0x2e, 0x2f, 0x16, 0x05, 0x15, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x3c, 0x3d, 0x32, 0x26,
    0x18, 0x19, 0x3f, 0x27, 0x1c, 0x1d, 0x1e, 0x1f, 0x40, 0x5a, 0x7f, 0x7b,
    0x5b, 0x6c, 0x50, 0x7d, 0x4d, 0x5d, 0x5c, 0x4e, 0x6b, 0x60, 0x4b, 0x61,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0x7a, 0x5e,
    0x4c, 0x7e, 0x6e, 0x6f, 0x7c, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xad, 0xe0, 0xbd, 0x5f, 0x6d,
    0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x91, 0x92,
    0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6,
    0xa7, 0xa8, 0xa9, 0xc0, 0x4f, 0xd0, 0xa1, 0x07, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x06, 0x17, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x09, 0x0a, 0x1b,
    0x30, 0x31, 0x1a, 0x33, 0x34, 0x35, 0x36, 0x08, 0x38, 0x39, 0x3a, 0x3b,
    0x04, 0x14, 0x3e, 0xff, 0x41, 0xaa, 0x4a, 0xb1, 0x9f, 0xb2, 0x6a, 0xb5,
    0xbb, 0xb4, 0x9a, 0x8a, 0xb0, 0xca, 0xaf, 0xbc, 0x90, 0x8f, 0xea, 0xfa,
    0xbe, 0xa0, 0xb6, 0xb3, 0x9d, 0xda, 0x9b, 0x8b, 0xb7, 0xb8, 0xb9, 0xab,
    0x64, 0x65, 0x62, 0x66, 0x63, 0x67, 0x9e, 0x68, 0x74, 0x71, 0x72, 0x73,
    0x78, 0x75, 0x76, 0x77, 0xac, 0x69, 0xed, 0xee, 0xeb, 0xef, 0xec, 0xbf,
    0x80, 0xfd, 0xfe, 0xfb, 0xfc, 0xba, 0xae, 0x59, 0x44, 0x45, 0x42, 0x46,
    0x43, 0x47, 0x9c, 0x48, 0x54, 0x51, 0x52, 0x53, 0x58, 0x55, 0x56, 0x57,
    0x8c, 0x49, 0xcd, 0xce, 0xcb, 0xcf, 0xcc, 0xe1, 0x70, 0xdd, 0xde, 0xdb,
    0xdc, 0x8d, 0x8e, 0xdf};

#ifdef __cplusplus
}
#endif
#endif // ZOS_CHAR_UTIL_H_
