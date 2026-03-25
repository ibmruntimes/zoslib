#define _XOPEN_SOURCE_EXTENDED 1
#define _EXT 1

#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <env.h>
#include <unistd.h>
#include <dynit.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdarg.h>
#include <ctype.h>

#include "zos-datasetio.h"

void* descriptor_table[MAX_FDS] = { 0 };

static dsio_recfm_t detect_recfm_from_fldata(const fldata_t* fdata);
static dsio_dsorg_t detect_dsorg_from_fldata(const fldata_t* fdata);

static const char DATASET_CHAR[] = "ABCDEFGHIJKLMNOPQRSTUVWYZ$#@";

//TODO: add to header
void* convertBuffer(void* buf, unsigned short from_ccsid, unsigned short to_ccsid);
DatasetEntry* createDatasetEntry(FILE* dd, unsigned short file_ccsid);

#define DATASET_CHAR_LEN (sizeof(DATASET_CHAR)-1)

static char* generate_name(char* tmplate)
{
  struct timeval tv;
  unsigned long long us;
  int start=0;
  int end=strlen(tmplate);
  int index;

  for (start=0; start<end; ++start) {
    if (tmplate[start] == 'X') {
      break;
    }
  }
  while (end > start) {
    if (tmplate[end] == 'X') {
      break;
    }
    --end;
  }
  if (end - start + 1 < 3) { /* at least 3 X's required in the tmplate */
    return NULL;            /* TBD: need to set proper errno's */
  }

  gettimeofday(&tv, NULL);
  us = ((unsigned long long) (tv.tv_sec)) * 1000000UL + ((unsigned long long) (tv.tv_usec));

  for (index=start; index<=end; ++index) {
    int entry = us % DATASET_CHAR_LEN;
    us /= DATASET_CHAR_LEN;
    tmplate[index] = DATASET_CHAR[entry];
  }
  return tmplate;
}

int mkstemp_dataset(char* tmplate)
{
  void* dd;
  int fd = -1;

  /* Without fopen(...,"wx" support, there is a race condition that can only be fixed by coding (maybe) in assembler */
  /* So... this code could overwrite another (probably temporary) file name */
  char* name = generate_name(tmplate);
  if (!name) {
    return -1;
  }

  /* Current just have 'dd' be the FILE pointer - may want something more substantial */
  dd = fopen(tmplate, "w");
  if (!dd) {
    return -1;
  }

  /* Use enhanced entry creation */
  DatasetEntry* dentry = create_entry(dd, 1047);
  if (!dentry) {
    fclose(dd);
    return -1;
  }
  
  parse_and_store_name(dentry, tmplate);
  update_global_stats_open();

  fd = GET_DUMMY_FD();
  ADD_DD(fd, dentry);
  

  
  return fd;
}

/*
 * create_dataset_fd - Open a dataset, create a DatasetEntry, register it, and return an fd.
 *
 * Handles the full lifecycle: fopen, fldata query, FB/FBS reopen optimization,
 * buffer allocation, and DD table registration.
 * Returns a valid fd on success, -1 on failure (all resources cleaned up).
 */
int create_dataset_fd(const char* name, unsigned short file_ccsid, int flags)
{
  /*
   * Performance recommendations per IBM z/OS documentation:
   * https://www.ibm.com/docs/en/zos/3.1.0?topic=considerations-accessing-mvs-data-sets
   *
   * We initially open with type=record to query attributes via fldata().
   * For FB/FBS datasets, we then reopen in plain binary mode for
   * multi-record I/O. For V/U, we keep type=record.
   */
  const char* fopen_mode;
  if (flags & O_RDONLY) {
    fopen_mode = "rb,type=record,recfm=+";
  } else if (flags & O_WRONLY) {
    fopen_mode = (flags & O_APPEND) ? "ab,type=record,recfm=+,noseek" : "wb,type=record,recfm=+,noseek";
  } else if (flags & O_RDWR) {
    fopen_mode = (flags & O_APPEND) ? "ab+,type=record,recfm=+" : "rb+,type=record,recfm=+";
  } else {
    fopen_mode = "rb,type=record,recfm=+";
  }

  DSIO_LOG_DEBUG("Open with mode %s\n", fopen_mode);
  FILE* dd = fopen(name, fopen_mode);
  if (!dd) {
    perror("dataset open failed");
    return -1;
  }

  DatasetEntry* dentry = create_entry(dd, file_ccsid);
  if (!dentry) {
    fclose(dd);
    return -1;
  }
  parse_and_store_name(dentry, name);
  dentry->open_flags = flags;
  dentry->conversion_state = SETCVTON; /* Default to conversion on */

  /*
   * Query dataset attributes via fldata().
   * IBM recommendation: "If you are using FB or FBS files, use binary I/O
   * instead of record I/O. This way, you can read or write more than one
   * record at a time."
   */
  fldata_t fld;
  if (fldata(dd, NULL, &fld) == 0) {
    dentry->recfm = fld.__recfmF ? 2 /* F */ :
                    fld.__recfmV ? 1 /* V */ :
                    fld.__recfmU ? 3 /* U */ : 0;
    dentry->reclen = fld.__maxreclen > 0 ? fld.__maxreclen : 80;
    dentry->blksize = fld.__blksize > 0 ? fld.__blksize : dentry->reclen;
    dentry->is_fixed_recfm = (fld.__recfmF && !fld.__recfmV && !fld.__recfmU) ? 1 : 0;
  } else {
    /* Default to FB80 if fldata fails */
    dentry->recfm = 2;
    dentry->reclen = 80;
    dentry->blksize = 80;
    dentry->is_fixed_recfm = 1;
  }

  /*
   * For FB/FBS datasets, reopen in plain binary mode (without type=record)
   * so we can read/write multiple records per fread/fwrite call.
   */
  if (dentry->is_fixed_recfm) {
    fclose(dd);
    dd = NULL;
    const char* reopen_mode;
    if (flags & O_RDONLY) {
      reopen_mode = "rb,recfm=+";
    } else if (flags & O_WRONLY) {
      reopen_mode = (flags & O_APPEND) ? "ab,recfm=+,noseek" : "wb,recfm=+,noseek";
    } else if (flags & O_RDWR) {
      reopen_mode = (flags & O_APPEND) ? "ab+,recfm=+" : "rb+,recfm=+";
    } else {
      reopen_mode = "rb,recfm=+";
    }
    DSIO_LOG_DEBUG("FB optimization: reopening with mode %s\n", reopen_mode);
    dd = fopen(name, reopen_mode);
    if (!dd) {
      free(dentry);
      return -1;
    }
    dentry->file_ptr = dd;
  }

  /* Optimization: Since we are already buffering in rec_buf to handle record boundaries, 
   * disabling C runtime buffering (_IONBF) avoids an unnecessary internal memcpy inside fwrite. 
   */
  setvbuf(dentry->file_ptr, NULL, _IONBF, 0);

  /*
   * Buffer sizing:
   * - For FB: use blksize to enable multi-record I/O
   * - For V/U: use maxreclen (one record at a time via type=record)
   */
  dentry->rec_buf_size = dentry->is_fixed_recfm ? dentry->blksize : dentry->reclen;

  /* Allocate record buffer (+1 for potential null terminator during conversion) */
  dentry->rec_buf = malloc(dentry->rec_buf_size + 1);
  if (!dentry->rec_buf) {
    fclose(dd);
    free(dentry);
    return -1;
  }

  int fd = GET_DUMMY_FD();
  if (fd < 0) {
    DSIO_LOG_DEBUG("create_dataset_fd: ERROR - GET_DUMMY_FD failed, errno=%d\n", errno);
    fclose(dd);
    free(dentry);
    return -1;
  }
  DSIO_LOG_DEBUG("create_dataset_fd: Assigned dummy fd %d\n", fd);
  ADD_DD(fd, dentry);
  return fd;
}

int open_dataset(const char* name, int flags, mode_t mode) 
{
  /* Start with support for the following 'access mode' flags:
   *  O_RDONLY, O_WRONLY, O_RDWR
   * Start with support for the following 'file creation flags'
   *  O_APPEND O_TRUNC
   * In particular, O_CREAT is _not_ supported since the dataset
   * is presumed to already exist. 
   * Use fopen, mkstemp, or other C services to allocate datasets
   *
   * Define the modes carefully so the dataset will not get created if it
   * does not already exist - we want this to be a failure
   * https://tech.mikefulton.ca/fopen_pdse_member
   */
  /*
   * Validate flags before delegating to create_dataset_fd,
   * which handles opening, attribute detection, and registration.
   */
  bool pds_member = strchr(name, '(');
  DSIO_LOG_DEBUG("open_dataset: name %s, flags %d, mode %d\n", name, flags, mode);

  if ((flags & O_APPEND) && (pds_member)) {
    DSIO_LOG_DEBUG("open_dataset: O_APPEND not supported for PDS member %s\n", name);
    errno = EINVAL;
    return -1;
  }

  if (!(flags & (O_RDONLY | O_WRONLY | O_RDWR))) {
    DSIO_LOG_DEBUG("open_dataset: Missing access mode in flags %d\n", flags);
    errno = EINVAL;
    return -1;
  }

  if ((flags & O_LARGEFILE) || (flags & O_NONBLOCK)) {
    DSIO_LOG_DEBUG("open_dataset: Unsupported flags in %d\n", flags);
    errno = EACCES;
    return -1;
  }

  if (flags & O_CREAT) {
    if (pds_member) {
      /* This is ok - the fopen will inherit the PDS(E) attributes for the open and 
       * so we can safely ignore that they asked for 'CREAT'
       */
      ;
    } else {
      /* No support for sequential datasets or DDNames being created through open
       * of a dataset at this point
       */
      DSIO_LOG_DEBUG("open_dataset: O_CREAT not supported for non-PDS member %s\n", name);
      errno = EINVAL;
      return -1;
    }
  }

  int fd = create_dataset_fd(name, 1047, flags);
  DSIO_LOG_DEBUG("open_dataset: create_dataset_fd returned fd %d\n", fd);
  return fd;
}

ssize_t write_dataset(int fd, const void* buf, size_t count)
{
  void* dd = GET_DD(fd);
  if (!dd) {
    errno = EBADF;
    return -1;
  }

  DatasetEntry* dentry = (DatasetEntry*) (dd);
  FILE* fp = dentry->file_ptr;

  DSIO_LOG_DEBUG("In Write, File ccsid: %d\n", dentry->file_ccsid);
  DSIO_LOG_DEBUG("In Write, Program ccsid: %d\n", dentry->program_ccsid);
  DSIO_LOG_DEBUG("In Write, Conversion state %d\n", dentry->conversion_state);

  const char* src = (const char*) buf;
  size_t total_written = 0;
  size_t pos = 0;

  while (pos < count) {
    /* How many bytes can we fit in the current record? */
    size_t space_in_rec = dentry->reclen - dentry->rec_buf_pos;
    size_t remaining = count - pos;
    size_t scan_len = (remaining < space_in_rec) ? remaining : space_in_rec;
    
    /* Look for newline in the current available record space */
    const char* nl = memchr(src + pos, '\n', scan_len);
    
    if (nl) {
      /* Copy everything up to the newline */
      size_t to_copy = nl - (src + pos);
      if (to_copy > 0) {
        memcpy(dentry->rec_buf + dentry->rec_buf_pos, src + pos, to_copy);
        dentry->rec_buf_pos += to_copy;
      }
      
      /* Record finalized by newline - flush it */
      if (dentry->is_fixed_recfm) {
        /* Pad FB record to reclen with spaces */
        while (dentry->rec_buf_pos < dentry->reclen) {
          dentry->rec_buf[dentry->rec_buf_pos++] = ' ';
        }
      }

      if (dentry->rec_buf_pos > 0) {
        if (dentry->conversion_state == SETCVTON) {
          dsio_convert_buffer(dentry->rec_buf, dentry->rec_buf_pos, 
                              dentry->program_ccsid, dentry->file_ccsid);
        }

        if (fwrite(dentry->rec_buf, 1, dentry->rec_buf_pos, fp) != dentry->rec_buf_pos) {
          return total_written > 0 ? (ssize_t) total_written : -1;
        }
        dentry->rec_buf_pos = 0;
        dentry->dirty = 0;
      } else if (!dentry->is_fixed_recfm) {
        fwrite("", 1, 0, fp);
      }
      
      pos += to_copy + 1; /* skip the data and the newline */
      total_written += to_copy + 1;
      dentry->dirty = 1;
    } else {
      /* No newline found in this record's space - copy what we can */
      size_t to_copy = scan_len;
      if (to_copy > 0) {
        memcpy(dentry->rec_buf + dentry->rec_buf_pos, src + pos, to_copy);
        dentry->rec_buf_pos += to_copy;
        dentry->dirty = 1;
      }
      
      pos += to_copy;
      total_written += to_copy;

      /* If record is full (no newline), flush it as a complete record */
      if (dentry->rec_buf_pos >= dentry->reclen) {
        if (dentry->conversion_state == SETCVTON) {
          dsio_convert_buffer(dentry->rec_buf, dentry->rec_buf_pos, 
                              dentry->program_ccsid, dentry->file_ccsid);
        }

        if (fwrite(dentry->rec_buf, 1, dentry->rec_buf_pos, fp) != dentry->rec_buf_pos) {
          return total_written > 0 ? (ssize_t) total_written : -1;
        }
        dentry->rec_buf_pos = 0;
        dentry->dirty = 0;
      }
    }
  }

  dentry->stream_offset += total_written;
  return (ssize_t) total_written;
}

ssize_t read_dataset(int fd, void* buf, size_t count)
{
  void* dd = GET_DD(fd);
  if (!dd) {
    errno = EBADF;
    return -1;
  }

  DatasetEntry* dentry = (DatasetEntry*) (dd);
  FILE* fp = dentry->file_ptr;

  /* If there are pending writes, we should flush them before reading 
   * However, for FB binary mode, the C runtime should handle it if it was opened with ab+ or rb+
   * But we are bypassing C runtime buffering with _IONBF and doing our own buffering in rec_buf.
   */
  if (dentry->dirty && dentry->rec_buf_pos > 0) {
    /* Flush pending write buffer */
    if (dentry->is_fixed_recfm) {
      while (dentry->rec_buf_pos < dentry->reclen) {
        dentry->rec_buf[dentry->rec_buf_pos++] = ' ';
      }
    }
    if (dentry->conversion_state == SETCVTON) {
      dsio_convert_buffer(dentry->rec_buf, dentry->rec_buf_pos, 
                          dentry->program_ccsid, dentry->file_ccsid);
    }
    fwrite(dentry->rec_buf, 1, dentry->rec_buf_pos, fp);
    dentry->rec_buf_pos = 0;
    dentry->dirty = 0;
  }
  
  /* If the buffer currently contains data that was read, we don't clear it yet.
   * If it was used for writing (and just flushed), dentry->rec_buf_pos is 0.
   * If we are switching from write to read, we might need to reset rec_buf_len.
   */
  if (dentry->dirty) {
    dentry->rec_buf_len = 0;
    dentry->rec_buf_pos = 0;
    dentry->dirty = 0;
  }

  DSIO_LOG_DEBUG("read_dataset: fd=%d count=%zu offset=%zu recfm=%s\n",
            fd, count, dentry->stream_offset, 
            dsio_recfm_to_string(dentry->recfm));

  char* dst = (char*) buf;
  size_t bytes_copied = 0;

  while (bytes_copied < count) {
    /* Fast-path: Emit pending newline */
    if (dentry->newline_pending) {
      dst[bytes_copied++] = '\n';
      dentry->newline_pending = 0;
      dentry->stream_offset++;
      continue;
    }

    /* Fast-path: Serve from existing buffer data using memcpy */
    if (dentry->rec_buf_pos < dentry->rec_buf_len) {
      size_t avail = dentry->rec_buf_len - dentry->rec_buf_pos;
      
      /* Record boundaries for fixed-format: insert newline after each LRECL */
      if (dentry->is_fixed_recfm && dentry->reclen > 0) {
        size_t pos_in_stream = dentry->stream_offset % (dentry->reclen + 1);
        size_t rec_avail = dentry->reclen - pos_in_stream;
        if (avail > rec_avail) avail = rec_avail;
      }

      size_t to_copy = (count - bytes_copied < avail) ? (count - bytes_copied) : avail;
      memcpy(dst + bytes_copied, dentry->rec_buf + dentry->rec_buf_pos, to_copy);
      
      dentry->rec_buf_pos += to_copy;
      bytes_copied += to_copy;
      dentry->stream_offset += to_copy;

      /* Check for record boundary after copying */
      if (dentry->is_fixed_recfm && dentry->reclen > 0) {
        if ((dentry->stream_offset % (dentry->reclen + 1)) == dentry->reclen) {
          dentry->newline_pending = 1;
        }
      } else if (dentry->rec_buf_pos >= dentry->rec_buf_len) {
        dentry->newline_pending = 1;
      }
      continue;
    }

    /* Slow-path: Read next block/record from dataset */
    if (dentry->eof_reached) break;

    size_t rc = fread(dentry->rec_buf, 1, dentry->rec_buf_size, fp);
    if (rc == 0) {
      dentry->eof_reached = 1;
      dentry->newline_pending = 1;
      break; /* Will emit newline on next loop or return if bytes_copied > 0 */
    }
    
    /* Convert CCSID of the newly read data in-place */
    if (dentry->conversion_state == SETCVTON && rc > 0) {
      dsio_convert_buffer(dentry->rec_buf, rc, dentry->file_ccsid, dentry->program_ccsid);
    }

    dentry->rec_buf_len = rc;
    dentry->rec_buf_pos = 0;
  }

  if (bytes_copied > 0) {
    update_read_stats(dentry, bytes_copied);
  }

  return (bytes_copied == 0 && count > 0) ? 0 : (ssize_t) bytes_copied;
}

int close_dataset(int fd)
{
  void* dd = GET_DD(fd);

  DatasetEntry* dentry = (DatasetEntry*) (dd);
  FILE* fp = dentry->file_ptr;

  /* Flush any partial record remaining in the write buffer */
  if (dentry->dirty && dentry->rec_buf_pos > 0 &&
      (dentry->open_flags & (O_WRONLY | O_RDWR))) {
    /* For FB datasets, pad final record to reclen with spaces */
    if (dentry->is_fixed_recfm) {
      while (dentry->rec_buf_pos < dentry->reclen) {
        dentry->rec_buf[dentry->rec_buf_pos++] = ' ';
      }
    }

    /* Convert CCSID before writing */
    if (dentry->conversion_state == SETCVTON) {
      void* conv_result = dsio_convert_buffer(dentry->rec_buf, dentry->rec_buf_pos, 
                                             dentry->program_ccsid, dentry->file_ccsid);
      if (conv_result == NULL) {
        fprintf(stderr, "WARNING: CCSID conversion failed during close\n");
        /* Continue with close despite conversion error */
      }
    }

    /* Write final record */
    size_t rc = fwrite(dentry->rec_buf, 1, dentry->rec_buf_pos, fp);
    if (rc != dentry->rec_buf_pos) {
      fprintf(stderr, "WARNING: Final record write incomplete (%zu of %zu bytes)\n", 
              rc, dentry->rec_buf_pos);
      /* Continue with close despite write error */
    }
  }

  int rc = fclose(fp);
  if (!rc) {
    close(fd);
    /* Free record buffer and deallocate DatasetEntry */
    if (dentry->rec_buf) {
      free(dentry->rec_buf);
    }
    free(dentry);
    CLEAR_DD(fd);
  }
  return rc;
}

char* temp_dataset_name(char* result)
{
  char* orig = getenv("__POSIX_TMPNAM");
  char temp[L_tmpnam+1];
  setenv("__POSIX_TMPNAM", "NO", 1);
  tmpnam(temp);
  setenv("__POSIX_TMPNAM", orig, 1);
  sprintf(result, "//'%s'", temp);
  return result;
}

char* temp_file_name(char* result)
{
  char* orig = getenv("__POSIX_TMPNAM");
  setenv("__POSIX_TMPNAM", "YES", 1);
  tmpnam(result);
  setenv("__POSIX_TMPNAM", orig, 1);
  return result;
}

/*
 * Input dataset name should be of the form: //'<dataset>'
 */
int allocate_dataset(const char* dataset)
{
  __dyn_t ip;
  char sysdd[] = "????????";
  int rca=0;
  int rcb=0;

  dyninit(&ip);

  char mvs_style_dataset[45];
  size_t dataset_len = strlen(dataset);

  if (dataset_len > 48) {
    return -1;
  }
  if (dataset[0] != '/' || dataset[1] != '/' || dataset[2] != '\'' || dataset[dataset_len-1] != '\'') {
    return -1;
  }
  memcpy(mvs_style_dataset, &dataset[3], dataset_len-4);
  mvs_style_dataset[dataset_len-4] = '\0';

  DSIO_LOG_DEBUG("Allocate dataset: %s\n", mvs_style_dataset);

  ip.__ddname = sysdd;
  ip.__dsname = mvs_style_dataset;
  ip.__status = __DISP_NEW;
  ip.__normdisp = __DISP_CATLG;
  ip.__dsorg = __DSORG_PO;
  ip.__recfm = _FB_;
  ip.__lrecl = 80;
  ip.__dsntype = __DSNT_LIBRARY;

  ip.__primary = 100;
  ip.__secondary = 10;
  ip.__alcunit = __TRK;

  rca = dynalloc(&ip);
  if (!rca) {
    rcb = dynfree(&ip);
  }
  if (rca | rcb) {
    fprintf(stderr, "Dataset allocation error. Dynalloc or Dynfree failed with error code %d, info code %d\n", ip.__errcode, ip.__infocode);
  }

  return rca | rcb;
}

int delete_dataset(const char* dataset)
{
  int rc = remove(dataset);
  if (rc) {
    perror("remove");
  }
  return rc;
}

void* convertBuffer(void* buf, unsigned short from_ccsid, unsigned short to_ccsid)
{
  if (from_ccsid == 1047 && to_ccsid == 819) {
    __e2a_s(buf);
  } else if (from_ccsid == 819 && to_ccsid == 1047) {
    __a2e_s(buf);
  } else {
    fprintf(stderr, "from_ccsid: %d to to_ccsid: %d not supported\n", from_ccsid, to_ccsid);
  }
  return buf;
}

int zos_fcntl(int fd, int cmd, struct f_cnvrt* req);

int fcntl_zos(int fd, int cmd, struct f_cnvrt* req)
{
  return zos_fcntl(fd, cmd, req);
}

int zos_fcntl(int fd, int cmd, struct f_cnvrt* req)
{
  //TODO: Only has support for F_CONTROL_CVT for now
  if (cmd == F_CONTROL_CVT) {
    if (!req) 
      return -1;

    if (fd < 0)
      return -1;

    void* dd = GET_DD(fd);
    DatasetEntry* dentry = (DatasetEntry*) (dd);

    if (req->cvtcmd == QUERYCVT) {
      req->pccsid = dentry->program_ccsid;
      req->fccsid = dentry->file_ccsid;
      req->cvtcmd = dentry->conversion_state;
    } else {
      dentry->program_ccsid = req->pccsid;
      dentry->file_ccsid = req->fccsid;
      dentry->conversion_state = req->cvtcmd;
    }
  }
  return 0;
}

DatasetEntry* createDatasetEntry(FILE* dd, unsigned short file_ccsid)
{
  DatasetEntry* dentry = malloc(sizeof(DatasetEntry));
  dentry->file_ptr = dd;
  dentry->file_ccsid = file_ccsid;
  dentry->program_ccsid = 819;
  dentry->conversion_state = SETCVTON;
  return dentry;
}


/* ========================================================================
 * lseek() - File Positioning
 * ======================================================================== */

off_t lseek_dataset(int fd, off_t offset, int whence) {
  DSIO_LOG_DEBUG("lseek_dataset: ENTER fd=%d, offset=%lld, whence=%d\n", fd, (long long)offset, whence);
    
    /* Validate fd */
    void* dd = GET_DD(fd);
    if (!dd) {
        errno = EBADF;
        DSIO_LOG_DEBUG("lseek_dataset: ERROR - EBADF (NULL dd) for fd %d\n", fd);
        return (off_t)-1;
    }
    
    DatasetEntry* dentry = (DatasetEntry*) dd;
    if (!dentry->file_ptr) {
        errno = EBADF;
        DSIO_LOG_DEBUG("lseek_dataset: ERROR - EBADF (NULL file_ptr) for fd %d\n", fd);
        return (off_t)-1;
    }
    FILE* fp = dentry->file_ptr;
    
    /* Calculate target position */
    off_t target;
    switch (whence) {
        case SEEK_SET:
            target = offset;
            break;
            
        case SEEK_CUR:
            target = (off_t)dentry->stream_offset + offset;
            break;
            
        case SEEK_END: {
            /* Use dsio_get_size for accurate size calculation */
            ssize_t file_size = dsio_get_size(fd);
            if (file_size < 0) {
                return (off_t)-1;
            }
            target = (off_t)file_size + offset;
            break;
        }
        
        default:
            errno = EINVAL;
            return (off_t)-1;
    }
    
    /* Validate target */
    if (target < 0) {
        errno = EINVAL;
        return (off_t)-1;
    }
    
    /* Check if already at target */
    if ((size_t)target == dentry->stream_offset) {
        return (off_t)target;
    }
    
    /* Handle backward seek */
    if ((size_t)target < dentry->stream_offset) {
        rewind(fp);
        dentry->stream_offset = 0;
        dentry->rec_buf_pos = 0;
        dentry->rec_buf_len = 0;
        dentry->newline_pending = 0;
    }
    
    /* Handle forward seek */
    if (dentry->is_fixed_recfm && dentry->reclen > 0) {
        /* FB: Optimize by calculating native position directly */
        size_t delta = (size_t)target - dentry->stream_offset;
        size_t records_to_skip = delta / (dentry->reclen + 1);
        size_t byte_in_record = delta % (dentry->reclen + 1);
        
        /* Handle seeking to newline position */
        if (byte_in_record == dentry->reclen) {
            dentry->newline_pending = 1;
            byte_in_record = 0;
            records_to_skip++;
        }
        
        /* Calculate native file position */
        long current_native = ftell(fp);
        if (current_native < 0) {
            errno = EIO;
            return (off_t)-1;
        }
        
        long target_native = current_native + (records_to_skip * dentry->reclen) + byte_in_record;
        
        if (fseek(fp, target_native, SEEK_SET) != 0) {
            errno = EIO;
            return (off_t)-1;
        }
        
        dentry->stream_offset = (size_t)target;
        dentry->rec_buf_pos = 0;
        dentry->rec_buf_len = 0;
        
    } else {
        /* VB/U: Must read and discard (no optimization possible) */
        while (dentry->stream_offset < (size_t)target) {
            char discard[4096];  /* Larger buffer for efficiency */
            size_t need = (size_t)target - dentry->stream_offset;
            size_t chunk = need < sizeof(discard) ? need : sizeof(discard);
            ssize_t n = read_dataset(fd, discard, chunk);
            if (n <= 0) {
                /* EOF reached before target */
                break;
            }
        }
    }
    
  DSIO_LOG_DEBUG("lseek_dataset: fd=%d target=%lld final=%zu\n",
                 fd, (long long)target, dentry->stream_offset);
    
    return (off_t)dentry->stream_offset;
}

/* ========================================================================
 * fstat() / stat() - File Metadata
 * ======================================================================== */


int fstat_dataset(int fd, struct stat *buf) {
    DSIO_LOG_DEBUG("fstat_dataset: ENTER fd=%d\n", fd);
    
    /* Validate parameters */
    if (!buf) {
        errno = EINVAL;
        DSIO_LOG_DEBUG("fstat_dataset: ERROR - NULL buffer for fd %d\n", fd);
        return -1;
    }
    
    void* dd = GET_DD(fd);
    if (!dd) {
        errno = EBADF;
        DSIO_LOG_DEBUG("fstat_dataset: ERROR - EBADF (NULL dd) for fd %d\n", fd);
        return -1;
    }
    
    DatasetEntry* dentry = (DatasetEntry*) dd;
    if (!dentry->file_ptr) {
        errno = EBADF;
        DSIO_LOG_DEBUG("fstat_dataset: ERROR - EBADF (NULL file_ptr) for fd %d\n", fd);
        return -1;
    }

    /* Initialize stat buffer */
    memset(buf, 0, sizeof(struct stat));
    
    /* 
     * Provide non-zero st_dev and st_ino to prevent tools (like ggrep) from 
     * incorrectly assuming all datasets are the same file (since st_dev=0, st_ino=0 
     * is often returned for both).
     */
    buf->st_dev = 0xFFFF; /* Magic value for MVS Datasets */
    
    /* Simple Jenkins hash for st_ino based on the dataset name */
    uint32_t hash = 0;
    const char* key = dentry->full_path;
    while (*key) {
        hash += (unsigned char)(*key++);
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    buf->st_ino = (ino_t)(hash ? hash : 1);

    buf->st_mode = S_IFREG | 0666;
    buf->st_nlink = 1;
    buf->st_uid = getuid();
    buf->st_gid = getgid();
    buf->st_blksize = dentry->blksize;
    
    /* Set timestamps to current time */
    time_t now = time(NULL);
    buf->st_atime = now;
    buf->st_mtime = now;
    buf->st_ctime = now;

    /* Use dsio_get_size helper to calculate emulated stream size */
    ssize_t size = dsio_get_size(fd);
    if (size < 0) {
        /* Error already set by dsio_get_size */
        return -1;
    }
    
    buf->st_size = (off_t)size;
    
  DSIO_LOG_DEBUG("fstat_dataset: fd=%d size=%lld\n", fd, (long long)buf->st_size);
    
    return 0;
}

int stat_dataset(const char *pathname, struct stat *statbuf) {
    DSIO_LOG_DEBUG("stat_dataset: ENTER path=%s\n", pathname);
    if (!pathname || !statbuf) {
        errno = EINVAL;
        return -1;
    }
    
    /* Open the dataset temporarily to get stats */
    int fd = open_dataset(pathname, O_RDONLY, 0);
    if (fd < 0) {
        DSIO_LOG_DEBUG("stat_dataset: open_dataset failed for %s, errno=%d\n", pathname, errno);
        return -1;
    }
    
    int result = fstat_dataset(fd, statbuf);
    close_dataset(fd);
    
    DSIO_LOG_DEBUG("stat_dataset: RETURN %d for %s\n", result, pathname);
    return result;
}

/* ========================================================================
 * opendir() / readdir() / closedir() - Directory Operations for PDS/PDSE
 * ======================================================================== */

#undef opendir
#undef readdir
#undef closedir

/* Helper function to read PDS directory using BPAM or simple approach */
static int read_pds_directory(const char* dataset_name, char*** member_list, int* member_count) {
    /* This is a simplified implementation
     * A full implementation would use BPAM to read the directory
     * For now, we'll use a simple approach with fopen and directory reading
     */
    
    *member_list = NULL;
    *member_count = 0;
    
    /* Try to open the PDS as a directory */
    /* In z/OS, we can list members by opening the PDS and reading directory blocks */
    /* This is a placeholder - real implementation would use BPAM services */
    
    DSIO_LOG_WARN("PDS directory reading not fully implemented yet for: %s", dataset_name);
    
    /* For now, return empty directory */
    /* TODO: Implement actual BPAM directory reading */
    
    return 0;
}

static DIR* opendir_dataset(const char *name) {
    if (!name) {
        errno = EINVAL;
        return NULL;
    }
    
    /* Check if this is a PDS/PDSE (no member specified) */
    if (strchr(name, '(') != NULL) {
        /* Member specified - not a directory */
        errno = ENOTDIR;
        DSIO_LOG_ERROR("opendir: Cannot open PDS member as directory: %s", name);
        return NULL;
    }
    
    /* Allocate directory structure */
    DatasetDir* dir = calloc(1, sizeof(DatasetDir));
    if (!dir) {
        errno = ENOMEM;
        return NULL;
    }
    
    strncpy(dir->dataset_name, name, sizeof(dir->dataset_name) - 1);
    dir->is_dataset_dir = 1;
    dir->current_index = 0;
    
    /* Read PDS directory */
    if (read_pds_directory(name, &dir->member_list, &dir->member_count) != 0) {
        free(dir);
        errno = EIO;
        DSIO_LOG_ERROR("opendir: Failed to read PDS directory: %s", name);
        return NULL;
    }
    

    
    return (DIR*)dir;
}

DIR* opendir_zos(const char *name) {
    if (IS_DATASET(name)) {
        DSIO_LOG_DEBUG("calling opendir-dataset\n");
        return opendir_dataset(name);
    } else {
        DSIO_LOG_DEBUG("calling opendir-file\n");
        return opendir(name);
    }
}

static struct dirent* readdir_dataset(DIR *dirp) {
    if (!dirp) {
        errno = EINVAL;
        return NULL;
    }
    
    DatasetDir* dir = (DatasetDir*)dirp;
    
    /* Check if we've read all members */
    if (dir->current_index >= dir->member_count) {
        return NULL; /* End of directory */
    }
    
    /* Allocate dirent structure (static for simplicity) */
    static struct dirent entry;
    memset(&entry, 0, sizeof(entry));
    
    /* Copy member name */
    strncpy(entry.d_name, dir->member_list[dir->current_index], sizeof(entry.d_name) - 1);
    
    dir->current_index++;
    

    
    return &entry;
}

struct dirent* readdir_zos(DIR *dirp) {
    if (!dirp) {
        return NULL;
    }
    
    /* Check if this is a dataset directory */
    DatasetDir* dir = (DatasetDir*)dirp;
    if (dir->is_dataset_dir) {
        DSIO_LOG_DEBUG("calling readdir-dataset\n");
        return readdir_dataset(dirp);
    } else {
        DSIO_LOG_DEBUG("calling readdir-file\n");
        return readdir(dirp);
    }
}

static int closedir_dataset(DIR *dirp) {
    if (!dirp) {
        errno = EINVAL;
        return -1;
    }
    
    DatasetDir* dir = (DatasetDir*)dirp;
    
    /* Free member list */
    if (dir->member_list) {
        for (int i = 0; i < dir->member_count; i++) {
            free(dir->member_list[i]);
        }
        free(dir->member_list);
    }
    

    
    free(dir);
    return 0;
}

int closedir_zos(DIR *dirp) {
    if (!dirp) {
        return -1;
    }
    
    /* Check if this is a dataset directory */
    DatasetDir* dir = (DatasetDir*)dirp;
    if (dir->is_dataset_dir) {
        DSIO_LOG_DEBUG("calling closedir-dataset\n");
        return closedir_dataset(dirp);
    } else {
        DSIO_LOG_DEBUG("calling closedir-file\n");
        return closedir(dirp);
    }
}

// Made with Bob - Phase 1 System Calls
 

/* Global state */
GlobalStats g_stats = {0};
dsio_log_level_t g_log_level = DSIO_LOG_ERROR;
FILE* g_log_stream = NULL;
int g_debug_enabled = 0;

/* ========================================================================
 * ERROR HANDLING IMPLEMENTATION
 * Adapted from libdio's comprehensive error handling
 * ======================================================================== */

static const char* error_messages[] = {
    [DSIO_SUCCESS] = "Success",
    [DSIO_ERR_INVALID_NAME] = "Invalid dataset name",
    [DSIO_ERR_NAME_TOO_LONG] = "Dataset name too long",
    [DSIO_ERR_OPEN_FAILED] = "Failed to open dataset",
    [DSIO_ERR_READ_FAILED] = "Failed to read from dataset",
    [DSIO_ERR_WRITE_FAILED] = "Failed to write to dataset",
    [DSIO_ERR_CLOSE_FAILED] = "Failed to close dataset",
    [DSIO_ERR_ALLOC_FAILED] = "Memory allocation failed",
    [DSIO_ERR_INVALID_FD] = "Invalid file descriptor",
    [DSIO_ERR_INVALID_RECFM] = "Invalid record format",
    [DSIO_ERR_INVALID_DSORG] = "Invalid dataset organization",
    [DSIO_ERR_CCSID_CONVERSION] = "CCSID conversion failed",
    [DSIO_ERR_BUFFER_OVERFLOW] = "Buffer overflow",
    [DSIO_ERR_MEMBER_NOT_FOUND] = "Member not found",
    [DSIO_ERR_NOT_A_DATASET] = "Not a dataset",
    [DSIO_ERR_FLDATA_FAILED] = "fldata() failed",
    [DSIO_ERR_FSEEK_FAILED] = "fseek() failed",
    [DSIO_ERR_FTELL_FAILED] = "ftell() failed",
    [DSIO_ERR_RECORD_TOO_LONG] = "Record too long",
    [DSIO_ERR_UNSUPPORTED_OPERATION] = "Unsupported operation",
    [DSIO_ERR_INTERNAL_ERROR] = "Internal error"
};

const char* dsio_strerror(dsio_error_t error) {
    if (error >= 0 && error < sizeof(error_messages)/sizeof(error_messages[0])) {
        return error_messages[error];
    }
    return "Unknown error";
}

dsio_error_t dsio_get_last_error(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return DSIO_ERR_INVALID_FD;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    return entry->last_error;
}

const char* dsio_get_error_message(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return "Invalid file descriptor";
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    return entry->error_message;
}

void dsio_clear_error(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    entry->last_error = DSIO_SUCCESS;
    entry->error_message[0] = '\0';
}

void set_entry_error(DatasetEntry* entry, dsio_error_t error, const char* message) {
    if (!entry) return;
    
    entry->last_error = error;
    if (message) {
        strncpy(entry->error_message, message, DSIO_MAX_ERROR_MSG - 1);
        entry->error_message[DSIO_MAX_ERROR_MSG - 1] = '\0';
    } else {
        strncpy(entry->error_message, dsio_strerror(error), DSIO_MAX_ERROR_MSG - 1);
        entry->error_message[DSIO_MAX_ERROR_MSG - 1] = '\0';
    }
    
    DSIO_LOG_ERROR("Error %d: %s", error, entry->error_message);
    update_global_stats_error();
}

/* ========================================================================
 * RECORD FORMAT UTILITIES
 * Adapted from libdio's record format handling
 * ======================================================================== */

int dsio_has_length_prefix(dsio_recfm_t recfm) {
    return (recfm == DSIO_RECFM_V || 
            recfm == DSIO_RECFM_VB || 
            recfm == DSIO_RECFM_VBA);
}

int dsio_is_blocked(dsio_recfm_t recfm) {
    return (recfm == DSIO_RECFM_FB || 
            recfm == DSIO_RECFM_VB || 
            recfm == DSIO_RECFM_FBA || 
            recfm == DSIO_RECFM_VBA);
}

int dsio_has_asa(dsio_recfm_t recfm) {
    return (recfm == DSIO_RECFM_FBA || 
            recfm == DSIO_RECFM_VBA);
}

const char* dsio_recfm_to_string(dsio_recfm_t recfm) {
    switch (recfm) {
        case DSIO_RECFM_F:   return "F";
        case DSIO_RECFM_V:   return "V";
        case DSIO_RECFM_U:   return "U";
        case DSIO_RECFM_FB:  return "FB";
        case DSIO_RECFM_VB:  return "VB";
        case DSIO_RECFM_FBA: return "FBA";
        case DSIO_RECFM_VBA: return "VBA";
        default:             return "UNKNOWN";
    }
}

const char* dsio_dsorg_to_string(dsio_dsorg_t dsorg) {
    switch (dsorg) {
        case DSIO_DSORG_PS:  return "PS";
        case DSIO_DSORG_PO:  return "PO";
        case DSIO_DSORG_POE: return "POE";
        case DSIO_DSORG_DA:  return "DA";
        case DSIO_DSORG_VS:  return "VS";
        default:             return "UNKNOWN";
    }
}

dsio_recfm_t dsio_string_to_recfm(const char* str) {
    if (!str) return DSIO_RECFM_UNKNOWN;
    
    if (strcmp(str, "F") == 0)   return DSIO_RECFM_F;
    if (strcmp(str, "V") == 0)   return DSIO_RECFM_V;
    if (strcmp(str, "U") == 0)   return DSIO_RECFM_U;
    if (strcmp(str, "FB") == 0)  return DSIO_RECFM_FB;
    if (strcmp(str, "VB") == 0)  return DSIO_RECFM_VB;
    if (strcmp(str, "FBA") == 0) return DSIO_RECFM_FBA;
    if (strcmp(str, "VBA") == 0) return DSIO_RECFM_VBA;
    
    return DSIO_RECFM_UNKNOWN;
}

dsio_dsorg_t dsio_string_to_dsorg(const char* str) {
    if (!str) return DSIO_DSORG_UNKNOWN;
    
    if (strcmp(str, "PS") == 0)  return DSIO_DSORG_PS;
    if (strcmp(str, "PO") == 0)  return DSIO_DSORG_PO;
    if (strcmp(str, "POE") == 0) return DSIO_DSORG_POE;
    if (strcmp(str, "DA") == 0)  return DSIO_DSORG_DA;
    if (strcmp(str, "VS") == 0)  return DSIO_DSORG_VS;
    
    return DSIO_DSORG_UNKNOWN;
}

/* ========================================================================
 * DATASET NAME PARSING
 * Adapted from libdio's dataset name handling
 * ======================================================================== */

int dsio_is_dataset_name(const char* name) {
    if (!name || strlen(name) < 2) {
        return 0;
    }
    return (name[0] == '/' && name[1] == '/');
}

int dsio_validate_dataset_name(const char* name) {
    if (!dsio_is_dataset_name(name)) {
        return 0;
    }
    
    size_t len = strlen(name);
    if (len > DSIO_MAX_DATASET_NAME) {
        return 0;
    }
    
    /* Check for valid characters and structure */
    int in_quotes = 0;
    int in_member = 0;
    
    for (size_t i = 2; i < len; i++) {
        char c = name[i];
        
        if (c == '\'') {
            in_quotes = !in_quotes;
        } else if (c == '(' && !in_quotes) {
            in_member = 1;
        } else if (c == ')' && !in_quotes) {
            in_member = 0;
        } else if (!in_quotes && !in_member) {
            /* Outside quotes and member: allow alphanumeric, $, #, @, . */
            if (!isalnum(c) && c != '$' && c != '#' && c != '@' && c != '.') {
                return 0;
            }
        }
    }
    
    return 1;
}

int dsio_parse_dataset_name(const char* name, dsio_name_parts_t* parts) {
    if (!name || !parts) {
        return -1;
    }
    
    memset(parts, 0, sizeof(dsio_name_parts_t));
    
    if (!dsio_validate_dataset_name(name)) {
        return -1;
    }
    
    /* Copy full name */
    strncpy(parts->full_name, name, sizeof(parts->full_name) - 1);
    
    /* Skip // prefix */
    const char* ds_start = name + 2;
    
    /* Check for quotes */
    if (*ds_start == '\'') {
        parts->is_quoted = 1;
        ds_start++;
    }
    
    /* Find member name if present */
    const char* member_start = strchr(ds_start, '(');
    const char* ds_end = member_start ? member_start : (ds_start + strlen(ds_start));
    
    if (member_start) {
        parts->has_member = 1;
        const char* member_end = strchr(member_start, ')');
        if (member_end) {
            size_t member_len = member_end - member_start - 1;
            if (member_len > 0 && member_len <= DSIO_MAX_MEMBER_NAME) {
                strncpy(parts->member, member_start + 1, member_len);
                parts->member[member_len] = '\0';
            }
        }
    }
    
    /* Parse qualifiers */
    char ds_name[DSIO_MAX_DATASET_NAME];
    size_t ds_len = ds_end - ds_start;
    if (ds_len >= sizeof(ds_name)) {
        return -1;
    }
    strncpy(ds_name, ds_start, ds_len);
    ds_name[ds_len] = '\0';
    
    /* Remove trailing quote if present */
    if (parts->is_quoted && ds_len > 0 && ds_name[ds_len-1] == '\'') {
        ds_name[ds_len-1] = '\0';
        ds_len--;
    }
    
    /* Split into qualifiers */
    char* first_dot = strchr(ds_name, '.');
    char* last_dot = strrchr(ds_name, '.');
    
    if (first_dot) {
        /* Extract HLQ */
        size_t hlq_len = first_dot - ds_name;
        if (hlq_len <= DSIO_MAX_QUALIFIER) {
            strncpy(parts->hlq, ds_name, hlq_len);
            parts->hlq[hlq_len] = '\0';
        }
        
        /* Extract LLQ */
        if (last_dot && last_dot != first_dot) {
            strncpy(parts->llq, last_dot + 1, DSIO_MAX_QUALIFIER);
            parts->llq[DSIO_MAX_QUALIFIER] = '\0';
            
            /* Extract MLQs */
            size_t mlq_start = first_dot - ds_name + 1;
            size_t mlq_len = last_dot - first_dot - 1;
            if (mlq_len > 0 && mlq_len < sizeof(parts->mlqs)) {
                strncpy(parts->mlqs, ds_name + mlq_start, mlq_len);
                parts->mlqs[mlq_len] = '\0';
            }
        } else {
            /* Only two qualifiers */
            strncpy(parts->llq, first_dot + 1, DSIO_MAX_QUALIFIER);
            parts->llq[DSIO_MAX_QUALIFIER] = '\0';
        }
    } else {
        /* Single qualifier - treat as HLQ */
        strncpy(parts->hlq, ds_name, DSIO_MAX_QUALIFIER);
        parts->hlq[DSIO_MAX_QUALIFIER] = '\0';
    }
    
    return 0;
}

/* ========================================================================
 * CCSID CONVERSION
 * Adapted from libdio's CCSID handling
 * ======================================================================== */

void* dsio_convert_buffer(void* buf, size_t len, 
                          uint16_t from_ccsid, 
                          uint16_t to_ccsid) {
    if (!buf || len == 0) {
        return buf;
    }
    
    if (from_ccsid == to_ccsid) {
        return buf;
    }
    
    if (from_ccsid == DSIO_CCSID_EBCDIC && to_ccsid == DSIO_CCSID_ASCII) {
        return convert_ebcdic_to_ascii(buf, len);
    } else if (from_ccsid == DSIO_CCSID_ASCII && to_ccsid == DSIO_CCSID_EBCDIC) {
        return convert_ascii_to_ebcdic(buf, len);
    }
    
    DSIO_LOG_WARN("Unsupported CCSID conversion: %d -> %d", from_ccsid, to_ccsid);
    return buf;
}

void* convert_ebcdic_to_ascii(void* buf, size_t len) {
    __e2a_l(buf, len);
    return buf;
}

void* convert_ascii_to_ebcdic(void* buf, size_t len) {
    __a2e_l(buf, len);
    return buf;
}

/* ========================================================================
 * LOGGING IMPLEMENTATION
 * Adapted from libdio's logging framework
 * ======================================================================== */

void dsio_set_log_level(dsio_log_level_t level) {
    g_log_level = level;
}

void dsio_set_log_stream(FILE* stream) {
    g_log_stream = stream;
}

void dsio_enable_debug(int enable) {
    g_debug_enabled = enable;
    if (enable && g_log_level < DSIO_LOG_DEBUG) {
        g_log_level = DSIO_LOG_DEBUG;
    }
}

void dsio_log(dsio_log_level_t level, const char* format, ...) {
    if (level > g_log_level) {
        return;
    }
    
    FILE* stream = g_log_stream ? g_log_stream : stderr;
    
    const char* level_str;
    switch (level) {
        case DSIO_LOG_ERROR: level_str = "ERROR"; break;
        case DSIO_LOG_WARN:  level_str = "WARN "; break;
        case DSIO_LOG_INFO:  level_str = "INFO "; break;
        case DSIO_LOG_DEBUG: level_str = "DEBUG"; break;
        case DSIO_LOG_TRACE: level_str = "TRACE"; break;
        default:             level_str = "?????"; break;
    }
    
    fprintf(stream, "[%s] ", level_str);
    
    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
    
    fprintf(stream, "\n");
    fflush(stream);
}

extern void __console(const void *p_in, int len_i);

void dsio_debug_print(const char* str) {
    if (g_debug_enabled <= 0) return;
    
    if (g_log_stream == NULL) {
        g_log_stream = fopen("zoslib.debug.log", "a");
        if (g_log_stream == NULL) {
            __console(str, strlen(str));
            return;
        }
    }
    if (g_log_stream) {
        fprintf(g_log_stream, "%s", str);
        fflush(g_log_stream);
    }
}

void dsio_debug_printf(const char* format, ...) {
    if (g_debug_enabled <= 0) return;

    char buf[1024];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (g_log_stream == NULL) {
        g_log_stream = fopen("zoslib.debug.log", "a");
        if (g_log_stream == NULL) {
            __console(buf, len);
            return;
        }
    }
    if (g_log_stream) {
        fprintf(g_log_stream, "%s", buf);
        fflush(g_log_stream);
    }
}

void log_error(const char* format, ...) {
    if (DSIO_LOG_ERROR > g_log_level) return;
    
    va_list args;
    va_start(args, format);
    FILE* stream = g_log_stream ? g_log_stream : stderr;
    fprintf(stream, "[ERROR] ");
    vfprintf(stream, format, args);
    fprintf(stream, "\n");
    fflush(stream);
    va_end(args);
}

void log_warn(const char* format, ...) {
    if (DSIO_LOG_WARN > g_log_level) return;
    
    va_list args;
    va_start(args, format);
    FILE* stream = g_log_stream ? g_log_stream : stderr;
    fprintf(stream, "[WARN ] ");
    vfprintf(stream, format, args);
    fprintf(stream, "\n");
    fflush(stream);
    va_end(args);
}

void log_info(const char* format, ...) {
    if (DSIO_LOG_INFO > g_log_level) return;
    
    va_list args;
    va_start(args, format);
    FILE* stream = g_log_stream ? g_log_stream : stderr;
    fprintf(stream, "[INFO ] ");
    vfprintf(stream, format, args);
    fprintf(stream, "\n");
    fflush(stream);
    va_end(args);
}

void log_debug(const char* format, ...) {
    if (DSIO_LOG_DEBUG > g_log_level) return;
    
    va_list args;
    va_start(args, format);
    FILE* stream = g_log_stream ? g_log_stream : stderr;
    fprintf(stream, "[DEBUG] ");
    vfprintf(stream, format, args);
    fprintf(stream, "\n");
    fflush(stream);
    va_end(args);
}

void log_trace(const char* format, ...) {
    if (DSIO_LOG_TRACE > g_log_level) return;
    
    va_list args;
    va_start(args, format);
    FILE* stream = g_log_stream ? g_log_stream : stderr;
    fprintf(stream, "[TRACE] ");
    vfprintf(stream, format, args);
    fprintf(stream, "\n");
    fflush(stream);
    va_end(args);
}

/* ========================================================================
 * STATISTICS IMPLEMENTATION
 * ======================================================================== */

void update_read_stats(DatasetEntry* entry, size_t bytes) {
    if (!entry || !entry->stats_enabled) return;
    
    entry->bytes_read += bytes;
    entry->read_operations++;
    
    if (g_stats.stats_enabled) {
        g_stats.total_bytes_read += bytes;
        g_stats.total_read_operations++;
    }
}

void update_write_stats(DatasetEntry* entry, size_t bytes) {
    if (!entry || !entry->stats_enabled) return;
    
    entry->bytes_written += bytes;
    entry->write_operations++;
    
    if (g_stats.stats_enabled) {
        g_stats.total_bytes_written += bytes;
        g_stats.total_write_operations++;
    }
}

void update_global_stats_open(void) {
    if (g_stats.stats_enabled) {
        g_stats.total_open_operations++;
    }
}

void update_global_stats_close(void) {
    if (g_stats.stats_enabled) {
        g_stats.total_close_operations++;
    }
}

void update_global_stats_error(void) {
    if (g_stats.stats_enabled) {
        g_stats.total_errors++;
    }
}

int dsio_get_stats(int fd, dsio_stats_t* stats) {
    if (!stats) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    
    stats->bytes_read = entry->bytes_read;
    stats->bytes_written = entry->bytes_written;
    stats->read_operations = entry->read_operations;
    stats->write_operations = entry->write_operations;
    stats->open_operations = 1; /* This fd was opened once */
    stats->close_operations = 0; /* Not closed yet */
    stats->errors = (entry->last_error != DSIO_SUCCESS) ? 1 : 0;
    
    return 0;
}

int dsio_reset_stats(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    
    entry->bytes_read = 0;
    entry->bytes_written = 0;
    entry->read_operations = 0;
    entry->write_operations = 0;
    
    return 0;
}

void dsio_get_global_stats(dsio_stats_t* stats) {
    if (!stats) return;
    
    stats->bytes_read = g_stats.total_bytes_read;
    stats->bytes_written = g_stats.total_bytes_written;
    stats->read_operations = g_stats.total_read_operations;
    stats->write_operations = g_stats.total_write_operations;
    stats->open_operations = g_stats.total_open_operations;
    stats->close_operations = g_stats.total_close_operations;
    stats->errors = g_stats.total_errors;
}

void dsio_reset_global_stats(void) {
    memset(&g_stats, 0, sizeof(g_stats));
    g_stats.stats_enabled = 1; /* Re-enable after reset */
}

/* ========================================================================
 * METADATA IMPLEMENTATION (Partial - showing key functions)
 * This would use fldata() to get actual dataset attributes
 * ======================================================================== */

int load_metadata_from_file(DatasetEntry* entry) {
    if (!entry || !entry->file_ptr) {
        return -1;
    }
    
    /* Use fldata() to get file information */
    fldata_t fdata;
    if (fldata(entry->file_ptr, NULL, &fdata) != 0) {
        set_entry_error(entry, DSIO_ERR_FLDATA_FAILED, "fldata() failed");
        return -1;
    }
    
    /* Extract metadata from fldata */
    entry->recfm = detect_recfm_from_fldata(&fdata);
    entry->dsorg = detect_dsorg_from_fldata(&fdata);
    entry->lrecl = fdata.__maxreclen;
    entry->blksize = fdata.__blksize;
    
    entry->metadata_loaded = 1;
    

    
    return 0;
}

/* Additional functions would be implemented here... */
/* This is a partial implementation showing the key patterns */

// Made with Bob


/* ========================================================================
 * HELPER FUNCTIONS IMPLEMENTATION
 * ======================================================================== */

static dsio_recfm_t detect_recfm_from_fldata(const fldata_t* fdata) {
    if (!fdata) return DSIO_RECFM_UNKNOWN;
    
    /* Check record format from fldata structure */
    if (fdata->__recfmF && fdata->__recfmBlk && fdata->__recfmASA) {
        return DSIO_RECFM_FBA;
    } else if (fdata->__recfmF && fdata->__recfmBlk) {
        return DSIO_RECFM_FB;
    } else if (fdata->__recfmF && fdata->__recfmASA) {
        return DSIO_RECFM_FA;
    } else if (fdata->__recfmF) {
        return DSIO_RECFM_F;
    } else if (fdata->__recfmV && fdata->__recfmBlk && fdata->__recfmASA) {
        return DSIO_RECFM_VBA;
    } else if (fdata->__recfmV && fdata->__recfmBlk) {
        return DSIO_RECFM_VB;
    } else if (fdata->__recfmV && fdata->__recfmASA) {
        return DSIO_RECFM_VA;
    } else if (fdata->__recfmV) {
        return DSIO_RECFM_V;
    } else if (fdata->__recfmU) {
        return DSIO_RECFM_U;
    }
    
    return DSIO_RECFM_UNKNOWN;
}

static dsio_dsorg_t detect_dsorg_from_fldata(const fldata_t* fdata) {
    if (!fdata) return DSIO_DSORG_UNKNOWN;
    
    /* Check dataset organization from fldata structure */
    if (fdata->__dsorgPO && fdata->__dsorgPDSE) {
        return DSIO_DSORG_POE;
    } else if (fdata->__dsorgPO) {
        return DSIO_DSORG_PO;
    } else if (fdata->__dsorgPS) {
        return DSIO_DSORG_PS;
    } else if (fdata->__dsorgVSAM) {
        return DSIO_DSORG_VS;
    }
    
    return DSIO_DSORG_UNKNOWN;
}

DatasetEntry* create_entry(FILE* fp, unsigned short file_ccsid) {
    DatasetEntry* entry = calloc(1, sizeof(DatasetEntry));
    if (!entry) {
        DSIO_LOG_ERROR("Failed to allocate DatasetEntry");
        return NULL;
    }
    
    entry->file_ptr = fp;
    entry->file_ccsid = file_ccsid;
    entry->program_ccsid = 819; /* ASCII */
    entry->conversion_state = 1; /* SETCVTON */
    entry->last_error = DSIO_SUCCESS;
    entry->stats_enabled = 1;
    entry->metadata_loaded = 0;
    
    /* Try to load metadata */
    if (fp) {
        load_metadata_from_file(entry);
    }
    
    return entry;
}

void free_entry(DatasetEntry* entry) {
    if (entry) {
        free(entry);
    }
}

int parse_and_store_name(DatasetEntry* entry, const char* dataset_name) {
    if (!entry || !dataset_name) {
        return -1;
    }
    
    dsio_name_parts_t parts;
    if (dsio_parse_dataset_name(dataset_name, &parts) != 0) {
        set_entry_error(entry, DSIO_ERR_INVALID_NAME, "Failed to parse dataset name");
        return -1;
    }
    
    /* Store components */
    strncpy(entry->full_path, dataset_name, DSIO_MAX_DATASET_NAME);
    entry->full_path[DSIO_MAX_DATASET_NAME] = '\0';

    strncpy(entry->hlq, parts.hlq, DSIO_MAX_QUALIFIER);
    entry->hlq[DSIO_MAX_QUALIFIER] = '\0';
    
    strncpy(entry->llq, parts.llq, DSIO_MAX_QUALIFIER);
    entry->llq[DSIO_MAX_QUALIFIER] = '\0';
    
    if (parts.has_member) {
        strncpy(entry->member_name, parts.member, DSIO_MAX_MEMBER_NAME);
        entry->member_name[DSIO_MAX_MEMBER_NAME] = '\0';
        entry->is_pds_member = 1;
    } else {
        entry->member_name[0] = '\0';
        entry->is_pds_member = 0;
    }
    
    return 0;
}

/* ========================================================================
 * METADATA ACCESS FUNCTIONS
 * ======================================================================== */

int dsio_get_metadata(int fd, dsio_metadata_t* metadata) {
    if (!metadata) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    
    /* Ensure metadata is loaded */
    if (!entry->metadata_loaded) {
        if (load_metadata_from_file(entry) != 0) {
            return -1;
        }
    }
    
    /* Copy metadata */
    metadata->recfm = entry->recfm;
    metadata->dsorg = entry->dsorg;
    metadata->lrecl = entry->lrecl;
    metadata->blksize = entry->blksize;
    metadata->file_ccsid = entry->file_ccsid;
    metadata->program_ccsid = entry->program_ccsid;
    metadata->is_pds_member = entry->is_pds_member;
    metadata->readonly = entry->readonly;
    
    strncpy(metadata->member_name, entry->member_name, sizeof(metadata->member_name) - 1);
    strncpy(metadata->hlq, entry->hlq, sizeof(metadata->hlq) - 1);
    strncpy(metadata->llq, entry->llq, sizeof(metadata->llq) - 1);
    
    return 0;
}

int dsio_get_recfm(int fd, dsio_recfm_t* recfm) {
    if (!recfm) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    if (!entry->metadata_loaded) {
        if (load_metadata_from_file(entry) != 0) {
            return -1;
        }
    }
    
    *recfm = entry->recfm;
    return 0;
}

int dsio_get_lrecl(int fd, uint16_t* lrecl) {
    if (!lrecl) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    if (!entry->metadata_loaded) {
        if (load_metadata_from_file(entry) != 0) {
            return -1;
        }
    }
    
    *lrecl = entry->lrecl;
    return 0;
}

int dsio_get_dsorg(int fd, dsio_dsorg_t* dsorg) {
    if (!dsorg) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    if (!entry->metadata_loaded) {
        if (load_metadata_from_file(entry) != 0) {
            return -1;
        }
    }
    
    *dsorg = entry->dsorg;
    return 0;
}

int dsio_get_ccsid(int fd, uint16_t* file_ccsid, uint16_t* program_ccsid) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    
    if (file_ccsid) {
        *file_ccsid = entry->file_ccsid;
    }
    if (program_ccsid) {
        *program_ccsid = entry->program_ccsid;
    }
    
    return 0;
}

int dsio_get_member_name(int fd, char* member, size_t len) {
    if (!member || len == 0) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    strncpy(member, entry->member_name, len - 1);
    member[len - 1] = '\0';
    
    return 0;
}

int dsio_get_hlq(int fd, char* hlq, size_t len) {
    if (!hlq || len == 0) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    strncpy(hlq, entry->hlq, len - 1);
    hlq[len - 1] = '\0';
    
    return 0;
}

int dsio_get_llq(int fd, char* llq, size_t len) {
    if (!llq || len == 0) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    strncpy(llq, entry->llq, len - 1);
    llq[len - 1] = '\0';
    
    return 0;
}

/* ========================================================================
 * DATASET PROPERTY CHECKS
 * ======================================================================== */

int dsio_is_pds(int fd) {
    dsio_dsorg_t dsorg;
    if (dsio_get_dsorg(fd, &dsorg) != 0) {
        return 0;
    }
    return (dsorg == DSIO_DSORG_PO);
}

int dsio_is_pdse(int fd) {
    dsio_dsorg_t dsorg;
    if (dsio_get_dsorg(fd, &dsorg) != 0) {
        return 0;
    }
    return (dsorg == DSIO_DSORG_POE);
}

int dsio_is_sequential(int fd) {
    dsio_dsorg_t dsorg;
    if (dsio_get_dsorg(fd, &dsorg) != 0) {
        return 0;
    }
    return (dsorg == DSIO_DSORG_PS);
}

int dsio_has_member(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return 0;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    return entry->is_pds_member;
}

int dsio_is_readonly(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return 0;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    return entry->readonly;
}

/* ========================================================================
 * UTILITY FUNCTIONS
 * ======================================================================== */

int dsio_get_max_reclen(int fd) {
    uint16_t lrecl;
    if (dsio_get_lrecl(fd, &lrecl) != 0) {
        return -1;
    }
    return (int)lrecl;
}

int dsio_is_empty(int fd) {
    ssize_t size = dsio_get_size(fd);
    if (size < 0) return -1;
    return (size == 0) ? 1 : 0;
}

/*
 * Helper function to calculate emulated stream size for VB datasets
 * by reading all records and summing their lengths (with newlines).
 * This is a one-time cost that gets cached.
 */
static ssize_t calculate_vb_emulated_size(FILE* fp, DatasetEntry* entry) {
    size_t total_size = 0;
    size_t rec_count = 0;
    
    DSIO_LOG_DEBUG("calculate_vb_emulated_size: ENTER rec_buf_size=%zu\n", entry->rec_buf_size);
    
    if (!entry->is_fixed_recfm) {
        /* VB/U: Already in type=record mode, save and restore position */
        fpos_t saved_pos;
        if (fgetpos(fp, &saved_pos) != 0) {
            DSIO_LOG_DEBUG("calculate_vb_emulated_size: RETURN -1 (fgetpos failed) %d\n", 1);
            return -1;
        }
        
        if (fseek(fp, 0, SEEK_SET) != 0) {
            fsetpos(fp, &saved_pos);
            DSIO_LOG_DEBUG("calculate_vb_emulated_size: RETURN -1 (fseek failed) %d\n", 1);
            return -1;
        }
        
        /* Read each record */
        while (1) {
            size_t rc = fread(entry->rec_buf, 1, entry->rec_buf_size, fp);
            if (rc == 0) break;  /* EOF */
            
            DSIO_LOG_DEBUG("calculate_vb_emulated_size: Read record #%zu, raw_length=%zu\n", rec_count + 1, rc);
            
            /* Validate VB record length doesn't exceed buffer */
            if (rc > entry->rec_buf_size) {
                fprintf(stderr, "ERROR: VB record length %zu exceeds buffer size %zu\n",
                        rc, entry->rec_buf_size);
                fsetpos(fp, &saved_pos);
                errno = EFBIG;
                DSIO_LOG_DEBUG("calculate_vb_emulated_size: RETURN -1 (record too large) %d\n", 1);
                return -1;
            }
            
            size_t original_rc = rc;
            /* Strip trailing spaces */
            while (rc > 0 && entry->rec_buf[rc - 1] == ' ') {
                rc--;
            }
            
            DSIO_LOG_DEBUG("calculate_vb_emulated_size: Record #%zu stripped from %zu to %zu bytes\n",
                         rec_count + 1, original_rc, rc);
            
            total_size += rc + 1;  /* +1 for newline */
            rec_count++;
            
            DSIO_LOG_DEBUG("calculate_vb_emulated_size: Running total=%zu bytes, rec_count=%zu\n",
                         total_size, rec_count);
        }
        
        /* Restore position */
        if (fsetpos(fp, &saved_pos) != 0) {
            DSIO_LOG_DEBUG("WARNING: Failed to restore file position after size calculation %d\n", 1);
        }
    }
    
    DSIO_LOG_DEBUG("calculate_vb_emulated_size: RETURN %zu bytes (%zu records) for %s dataset\n", 
                 total_size, rec_count, entry->is_fixed_recfm ? "FB" : "VB");
    
    return (ssize_t)total_size;
}

ssize_t dsio_get_size(int fd) {
    DSIO_LOG_DEBUG("dsio_get_size: ENTER fd=%d\n", fd);
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        errno = EBADF;
        DSIO_LOG_DEBUG("dsio_get_size: RETURN -1 (invalid fd) %d\n", 1);
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    if (!entry->file_ptr) {
        errno = EBADF;
        DSIO_LOG_DEBUG("dsio_get_size: RETURN -1 (no file_ptr) %d\n", 1);
        return -1;
    }
    
    FILE* fp = entry->file_ptr;
    
    /* For VB datasets, check if we have cached size */
    if (!entry->is_fixed_recfm && entry->vb_size_calculated) {
        DSIO_LOG_DEBUG("dsio_get_size: RETURN %zu (cached VB size)\n", entry->vb_cached_size);
        return (ssize_t)entry->vb_cached_size;
    }
    
    /* Calculate emulated stream size from native file size */
    fpos_t pos;
    if (fgetpos(fp, &pos) != 0) {
        return -1;
    }
    
    if (fseek(fp, 0, SEEK_END) != 0) {
        fsetpos(fp, &pos);
        return -1;
    }
    
    long native_size = ftell(fp);
    if (native_size < 0) {
        fsetpos(fp, &pos);
        DSIO_LOG_DEBUG("dsio_get_size: RETURN -1 (ftell failed) %d\n", 1);
        return -1;
    }
    
    DSIO_LOG_DEBUG("dsio_get_size: native_size=%ld, is_fixed_recfm=%d, reclen=%zu\n",
                 native_size, entry->is_fixed_recfm, entry->reclen);
    
    if (fsetpos(fp, &pos) != 0) {
        DSIO_LOG_DEBUG("WARNING: fsetpos() failed in dsio_get_size %d\n", 1);
    }
    
    /* Calculate emulated stream size based on record format */
    ssize_t emulated_size;
    
    if (entry->is_fixed_recfm && entry->reclen > 0) {
        /* FB: native_size is total bytes, add newlines */
        size_t num_records = native_size / entry->reclen;
        emulated_size = (ssize_t)(native_size + num_records);
        DSIO_LOG_DEBUG("dsio_get_size: FB calculation - native=%ld, reclen=%zu, num_records=%zu, emulated=%zd\n",
                     native_size, entry->reclen, num_records, emulated_size);
    } else {
        /* VB/U: Calculate by reading all records (one-time cost) */
        DSIO_LOG_DEBUG("dsio_get_size: Calculating VB emulated size...%d\n", 1);
        emulated_size = calculate_vb_emulated_size(fp, entry);
        if (emulated_size >= 0) {
            /* Cache the result for future calls */
            entry->vb_cached_size = (size_t)emulated_size;
            entry->vb_size_calculated = 1;
            DSIO_LOG_DEBUG("dsio_get_size: VB size calculated and cached: %zd\n", emulated_size);
        } else {
            DSIO_LOG_DEBUG("dsio_get_size: VB size calculation failed %d\n", 1);
        }
    }
    
    DSIO_LOG_DEBUG("dsio_get_size: RETURN %zd (fd=%d, native=%ld, emulated=%zd)\n", 
                 emulated_size, fd, native_size, emulated_size);
    
    return emulated_size;
}

int dsio_flush(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    if (!entry->file_ptr) {
        return -1;
    }
    
    return fflush(entry->file_ptr);
}

/* ========================================================================
 * CCSID CONVERSION STUBS
 * These would use iconv() or __atoe()/__etoa() for actual conversion
 * ======================================================================== */

int dsio_set_ccsid_config(int fd, const dsio_ccsid_config_t* config) {
    if (!config) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    
    /* Store CCSID configuration */
    entry->file_ccsid = config->source_ccsid;
    entry->program_ccsid = config->target_ccsid;
    entry->conversion_state = config->conversion_enabled ? 1 : 0;
    

    
    return 0;
}

int dsio_get_ccsid_config(int fd, dsio_ccsid_config_t* config) {
    if (!config) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    
    config->source_ccsid = entry->file_ccsid;
    config->target_ccsid = entry->program_ccsid;
    config->conversion_enabled = entry->conversion_state;
    config->auto_detect = 0;
    
    return 0;
}



// Made with Bob
