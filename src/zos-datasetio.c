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

#if ZOSLIB_ENABLE_DATASETIO

/* __close_orig is defined in zos-io.cc as the original close() syscall.
 * We need it in close_dataset to close the dummy /dev/null fd without
 * routing through our __close() override (which would cause reentrancy).
 */
extern int __close_orig(int) __asm("close");

void* descriptor_table[MAX_FDS] = { 0 };

static dsio_recfm_t detect_recfm_from_fldata(const fldata_t* fdata);
static int map_dsio_error_to_errno(dsio_error_t dsio_err);

static dsio_dsorg_t detect_dsorg_from_fldata(const fldata_t* fdata);

static const char DATASET_CHAR[] = "ABCDEFGHIJKLMNOPQRSTUVWYZ$#@";

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
    errno = EINVAL;
    return NULL;
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
  int accmode = flags & O_ACCMODE;
  const char* fopen_mode;
  if (accmode == O_RDONLY) {
    fopen_mode = "rb,type=record,recfm=+";
  } else if (accmode == O_WRONLY) {
    fopen_mode = (flags & O_APPEND) ? "ab,type=record,recfm=+,noseek" : "wb,type=record,recfm=+,noseek";
  } else if (accmode == O_RDWR) {
    fopen_mode = (flags & O_APPEND) ? "ab+,type=record,recfm=+" : "rb+,type=record,recfm=+";
  } else {
    fopen_mode = "rb,type=record,recfm=+";
  }

  DSIO_LOG_DEBUG("Open with mode %s\n", fopen_mode);
  FILE* dd = fopen(name, fopen_mode);
  if (!dd) {
    perror("dataset open failed");
    errno = EIO;
    return -1;
  }

  DatasetEntry* dentry = create_entry(dd, file_ccsid);
  if (!dentry) {
    fclose(dd);
    errno = ENOMEM;
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
    dentry->recfm = detect_recfm_from_fldata(&fld);
    dentry->reclen = fld.__maxreclen > 0 ? fld.__maxreclen : 80;
    dentry->blksize = fld.__blksize > 0 ? fld.__blksize : dentry->reclen;
    dentry->is_fixed_recfm = (fld.__recfmF && !fld.__recfmV && !fld.__recfmU) ? 1 : 0;
    DSIO_LOG_DEBUG("open_dataset: fldata attributes - recfm=%s, reclen=%zu, blksize=%zu, is_fixed=%d\n",
                  dsio_recfm_to_string(dentry->recfm), dentry->reclen, dentry->blksize, dentry->is_fixed_recfm);
  } else {
    /* Default to FB80 if fldata fails */
    dentry->recfm = DSIO_RECFM_FB;
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
    if (accmode == O_RDONLY) {
      reopen_mode = "rb,recfm=+";
    } else if (accmode == O_WRONLY) {
      reopen_mode = (flags & O_APPEND) ? "ab,recfm=+,noseek" : "wb,recfm=+,noseek";
    } else if (accmode == O_RDWR) {
      reopen_mode = (flags & O_APPEND) ? "ab+,recfm=+" : "rb+,recfm=+";
    } else {
      reopen_mode = "rb,recfm=+";
    }
    DSIO_LOG_DEBUG("FB optimization: reopening with mode %s\n", reopen_mode);
    dd = fopen(name, reopen_mode);
    if (!dd) {
      set_entry_error(dentry, DSIO_ERR_OPEN_FAILED, "Failed to reopen dataset in binary mode");
      free(dentry);
      errno = map_dsio_error_to_errno(DSIO_ERR_OPEN_FAILED);
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
    set_entry_error(dentry, DSIO_ERR_ALLOC_FAILED, "Failed to allocate record buffer");
    fclose(dd);
    free(dentry);
    errno = map_dsio_error_to_errno(DSIO_ERR_ALLOC_FAILED);
    return -1;
  }

  int fd = GET_DUMMY_FD();
  if (fd < 0) {
    DSIO_LOG_DEBUG("create_dataset_fd: ERROR - GET_DUMMY_FD failed, errno=%d\n", errno);
    set_entry_error(dentry, DSIO_ERR_INTERNAL_ERROR, "Failed to allocate file descriptor");
    fclose(dd);
    free(dentry);
    errno = map_dsio_error_to_errno(DSIO_ERR_INTERNAL_ERROR);
    return -1;
  }
  DSIO_LOG_DEBUG("create_dataset_fd: Assigned dummy fd %d\n", fd);
  ADD_DD(fd, dentry);
  return fd;
}

int mkstemp_dataset(char* tmplate)
{
  /* Without fopen(...,"wx" support, there is a race condition that can only be fixed by coding (maybe) in assembler */
  /* So... this code could overwrite another (probably temporary) file name */
  char* name = generate_name(tmplate);
  if (!name) {
    return -1;
  }

  /* Delegate to create_dataset_fd which handles the full lifecycle:
   * fopen, fldata query, FB reopen, buffer allocation, and DD registration.
   */
  return create_dataset_fd(tmplate, 1047, O_RDWR);
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
  int accmode = flags & O_ACCMODE;
  DSIO_LOG_DEBUG("open_dataset: name %s, flags %d, mode %d\n", name, flags, mode);

  if ((flags & O_APPEND) && (pds_member)) {
    DSIO_LOG_DEBUG("open_dataset: O_APPEND not supported for PDS member %s\n", name);
    errno = EINVAL;
    return -1;
  }

  /* O_ACCMODE check: O_RDONLY is 0, O_WRONLY is 1, O_RDWR is 2 */
  if (accmode != O_RDONLY && accmode != O_WRONLY && accmode != O_RDWR) {
    DSIO_LOG_DEBUG("open_dataset: Invalid access mode in flags %d\n", flags);
    errno = EINVAL;
    return -1;
  }

  /* Strip flags that we don't support but shouldn't fail on */
  flags &= ~(O_LARGEFILE | O_NONBLOCK);

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
  
  /* Clear any previous errors */
  dentry->last_error = DSIO_SUCCESS;
  
  FILE* fp = dentry->file_ptr;

  DSIO_LOG_DEBUG("write_dataset: fd=%d count=%zu ccsid=%d->%d conv=%d\n",
                 fd, count, dentry->program_ccsid, dentry->file_ccsid, dentry->conversion_state);

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
          set_entry_error(dentry, DSIO_ERR_WRITE_FAILED, "Failed to write record to dataset");
          errno = map_dsio_error_to_errno(DSIO_ERR_WRITE_FAILED);
          return total_written > 0 ? (ssize_t) total_written : -1;
        }
        dentry->rec_buf_pos = 0;
        dentry->dirty = 0;
      } else if (!dentry->is_fixed_recfm) {
        fwrite("", 1, 0, fp);
      }
      
      pos += to_copy + 1; /* skip the data and the newline */
      total_written += to_copy + 1;
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
          set_entry_error(dentry, DSIO_ERR_WRITE_FAILED, "Failed to write full record to dataset");
          errno = map_dsio_error_to_errno(DSIO_ERR_WRITE_FAILED);
          return total_written > 0 ? (ssize_t) total_written : -1;
        }
        dentry->rec_buf_pos = 0;
        dentry->dirty = 0;
      }
    }
  }

  dentry->stream_offset += total_written;
  /* Invalidate cached size since content has changed */
  if (total_written > 0) {
    dentry->size_calculated = 0;
  }
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
  
  /* Clear any previous errors */
  dentry->last_error = DSIO_SUCCESS;
  FILE* fp = dentry->file_ptr;

  /* If there are pending writes, flush them before reading.
   * We bypass C runtime buffering with _IONBF and do our own buffering in rec_buf.
   */
  if (dentry->dirty) {
    if (dentry->rec_buf_pos > 0) {
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
      if (fwrite(dentry->rec_buf, 1, dentry->rec_buf_pos, fp) != dentry->rec_buf_pos) {
        set_entry_error(dentry, DSIO_ERR_WRITE_FAILED, "Failed to flush write buffer before read");
        errno = map_dsio_error_to_errno(DSIO_ERR_WRITE_FAILED);
        return -1;
      }
    }
    /* Reset buffer state for switch to read mode */
    dentry->rec_buf_pos = 0;
    dentry->rec_buf_len = 0;
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
    if (dentry->eof_reached) {
      if (dentry->newline_pending) continue;
      break;
    }

    size_t rc = fread(dentry->rec_buf, 1, dentry->rec_buf_size, fp);
    DSIO_LOG_DEBUG("read_dataset: fread returned %zu, buf_size=%zu, feof=%d, ferror=%d\n", 
                  rc, dentry->rec_buf_size, feof(fp), ferror(fp));
    if (rc == 0) {
      if (ferror(fp)) {
        set_entry_error(dentry, DSIO_ERR_READ_FAILED, "Failed to read from dataset");
        errno = map_dsio_error_to_errno(DSIO_ERR_READ_FAILED);
        return bytes_copied > 0 ? (ssize_t)bytes_copied : -1;
      }
      dentry->eof_reached = 1;
      /* Only set newline_pending if we actually read data from this dataset.
       * Without this guard, an empty dataset would return '\n' instead of 0.
       */
      if (bytes_copied > 0) {
        dentry->newline_pending = 1;
      }
      break; /* Will emit newline on next call or return if bytes_copied > 0 */
    }
    
    /* Convert CCSID of the newly read data in-place */
    if (dentry->conversion_state == SETCVTON && rc > 0) {
      dsio_convert_buffer(dentry->rec_buf, rc, dentry->file_ccsid, dentry->program_ccsid);
    }

    dentry->rec_buf_len = rc;
    dentry->rec_buf_pos = 0;
  }

  return (bytes_copied == 0 && count > 0) ? 0 : (ssize_t) bytes_copied;
}

int close_dataset(int fd)
{
  void* dd = GET_DD(fd);
  if (!dd) {
    errno = EBADF;
    return -1;
  }

  DatasetEntry* dentry = (DatasetEntry*) (dd);
  
  /* Clear any previous errors */
  dentry->last_error = DSIO_SUCCESS;
  
  FILE* fp = dentry->file_ptr;

  /* Flush any partial record remaining in the write buffer */
  if (dentry->dirty && dentry->rec_buf_pos > 0 &&
      ((dentry->open_flags & O_ACCMODE) != O_RDONLY)) {
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
        set_entry_error(dentry, DSIO_ERR_CCSID_CONVERSION, "CCSID conversion failed during close");
        fprintf(stderr, "WARNING: CCSID conversion failed during close\n");
        /* Continue with close despite conversion error */
      }
    }

    /* Write final record */
    size_t rc = fwrite(dentry->rec_buf, 1, dentry->rec_buf_pos, fp);
    if (rc != dentry->rec_buf_pos) {
      set_entry_error(dentry, DSIO_ERR_WRITE_FAILED, "Final record write incomplete during close");
      fprintf(stderr, "WARNING: Final record write incomplete (%zu of %zu bytes)\n", 
              rc, dentry->rec_buf_pos);
      /* Continue with close despite write error */
    }
  }

  int rc = fclose(fp);
  if (rc != 0) {
    set_entry_error(dentry, DSIO_ERR_CLOSE_FAILED, "fclose() failed");
    errno = map_dsio_error_to_errno(DSIO_ERR_CLOSE_FAILED);
  }
  
  /* Free record buffer and deallocate DatasetEntry */
  if (dentry->rec_buf) {
    free(dentry->rec_buf);
  }
  free(dentry);
  /* Clear the DD entry BEFORE closing the dummy fd to avoid reentrancy:
   * close(fd) routes through __close() which would call close_dataset()
   * again if CLEAR_DD hasn't been called yet.
   */
  CLEAR_DD(fd);
  __close_orig(fd);
  
  return rc;
}

char* temp_dataset_name(char* result)
{
  char* orig = getenv("__POSIX_TMPNAM");
  char temp[L_tmpnam+1];
  setenv("__POSIX_TMPNAM", "NO", 1);
  tmpnam(temp);
  if (orig)
    setenv("__POSIX_TMPNAM", orig, 1);
  else
    unsetenv("__POSIX_TMPNAM");
  sprintf(result, "//'%s'", temp);
  return result;
}

char* temp_file_name(char* result)
{
  char* orig = getenv("__POSIX_TMPNAM");
  setenv("__POSIX_TMPNAM", "YES", 1);
  tmpnam(result);
  if (orig)
    setenv("__POSIX_TMPNAM", orig, 1);
  else
    unsetenv("__POSIX_TMPNAM");
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

int zos_fcntl(int fd, int cmd, struct f_cnvrt* req)
{
  /* Only F_CONTROL_CVT is supported for dataset fds */
  if (cmd == F_CONTROL_CVT) {
    if (!req || fd < 0)
      return -1;

    if (!IS_DD(fd)) {
      errno = EBADF;
      return -1;
    }

    DatasetEntry* dentry = (DatasetEntry*) GET_DD(fd);

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
    
    /* Clear any previous errors and EOF state */
    dentry->last_error = DSIO_SUCCESS;
    dentry->eof_reached = 0;
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
                set_entry_error(dentry, DSIO_ERR_FSEEK_FAILED, "Failed to get file size for SEEK_END");
                errno = map_dsio_error_to_errno(DSIO_ERR_FSEEK_FAILED);
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
        size_t total_records = (size_t)target / (dentry->reclen + 1);
        size_t byte_in_record = (size_t)target % (dentry->reclen + 1);
        
        /* Handle seeking to newline position */
        if (byte_in_record == dentry->reclen) {
            dentry->newline_pending = 1;
            byte_in_record = 0;
            total_records++;
        } else {
            dentry->newline_pending = 0;
        }
        
        long target_native = (long)(total_records * dentry->reclen + byte_in_record);
        
        DSIO_LOG_DEBUG("lseek_dataset: FB target=%zu, records=%zu, byte=%zu, native=%ld, newline=%d\n",
                      (size_t)target, total_records, byte_in_record, target_native, dentry->newline_pending);

        if (fseek(fp, target_native, SEEK_SET) != 0) {
            set_entry_error(dentry, DSIO_ERR_FSEEK_FAILED, "fseek() failed during lseek");
            errno = map_dsio_error_to_errno(DSIO_ERR_FSEEK_FAILED);
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
    
    /* Clear any previous errors */
    dentry->last_error = DSIO_SUCCESS;
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

    /* Calculate emulated stream size. For VB/U datasets, this requires
     * reading all records (one-time cost, cached afterward). If size
     * cannot be determined (e.g. U-format with no seek support), return
     * st_size=0 so callers like `tail` can fall back to sequential reading
     * rather than failing entirely.
     */
    ssize_t size = dsio_get_size(fd);
    if (size < 0) {
        DSIO_LOG_DEBUG("fstat_dataset: dsio_get_size failed (fd=%d), returning st_size=0\n", fd);
        size = 0;
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


 

/* Global state */

dsio_log_level_t g_log_level = DSIO_LOG_DEBUG;
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

/* Map DSIO error codes to POSIX errno values */
static int map_dsio_error_to_errno(dsio_error_t dsio_err) {
    switch (dsio_err) {
        case DSIO_SUCCESS:
            return 0;
        case DSIO_ERR_INVALID_NAME:
        case DSIO_ERR_INVALID_RECFM:
        case DSIO_ERR_INVALID_DSORG:
        case DSIO_ERR_INVALID_FD:
            return EINVAL;
        case DSIO_ERR_NAME_TOO_LONG:
            return ENAMETOOLONG;
        case DSIO_ERR_OPEN_FAILED:
        case DSIO_ERR_READ_FAILED:
        case DSIO_ERR_WRITE_FAILED:
        case DSIO_ERR_CLOSE_FAILED:
        case DSIO_ERR_FLDATA_FAILED:
        case DSIO_ERR_FSEEK_FAILED:
        case DSIO_ERR_FTELL_FAILED:
            return EIO;
        case DSIO_ERR_ALLOC_FAILED:
            return ENOMEM;
        case DSIO_ERR_MEMBER_NOT_FOUND:
        case DSIO_ERR_NOT_A_DATASET:
            return ENOENT;
        case DSIO_ERR_CCSID_CONVERSION:
            return EILSEQ;
        case DSIO_ERR_BUFFER_OVERFLOW:
        case DSIO_ERR_RECORD_TOO_LONG:
            return EOVERFLOW;
        case DSIO_ERR_UNSUPPORTED_OPERATION:
            return ENOTSUP;
        case DSIO_ERR_INTERNAL_ERROR:
        default:
            return EIO;
    }
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
        case DSIO_RECFM_FA:  return "FA";
        case DSIO_RECFM_VA:  return "VA";
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
    if (enable) {
        if (g_log_level < DSIO_LOG_DEBUG) {
            g_log_level = DSIO_LOG_DEBUG;
        }
        /* Auto-create temp log file if not already set */
        if (!g_log_stream) {
            char logpath[256];
            snprintf(logpath, sizeof(logpath), "/tmp/zoslib_dsio_%d.log", getpid());
            g_log_stream = fopen(logpath, "a");
            if (g_log_stream) {
                fprintf(stderr, "DSIO: Logging to %s\n", logpath);
            }
        }
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
    
    fflush(stream);
}

extern void __console(const void *p_in, int len_i);

void log_error(const char* format, ...) {
    if (DSIO_LOG_ERROR > g_log_level) return;
    
    va_list args;
    va_start(args, format);
    FILE* stream = g_log_stream ? g_log_stream : stderr;
    fprintf(stream, "[ERROR] ");
    vfprintf(stream, format, args);
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
    fflush(stream);
    va_end(args);
}

/* ========================================================================
 * STATISTICS IMPLEMENTATION
 * ======================================================================== */


/* ========================================================================
 * METADATA IMPLEMENTATION (Partial - showing key functions)
 * This would use fldata() to get actual dataset attributes
 * ======================================================================== */

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
    
    /* Try to load metadata */
    if (fp) {
        fldata_t fdata;
        if (fldata(fp, NULL, &fdata) == 0) {
            entry->recfm = detect_recfm_from_fldata(&fdata);
            entry->dsorg = detect_dsorg_from_fldata(&fdata);
            entry->reclen = fdata.__maxreclen;
            entry->blksize = fdata.__blksize;
        }
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
    
    /* Copy metadata */
    metadata->recfm = entry->recfm;
    metadata->dsorg = entry->dsorg;
    metadata->reclen = entry->reclen;
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
    *recfm = entry->recfm;
    return 0;
}

int dsio_get_lrecl(int fd, size_t* lrecl) {
    if (!lrecl) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    *lrecl = entry->reclen;
    return 0;
}

int dsio_get_dsorg(int fd, dsio_dsorg_t* dsorg) {
    if (!dsorg) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
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
    size_t lrecl;
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
    
    if (!entry->is_fixed_recfm) {
        /* VB/U: Already in type=record mode, save and restore position */
        fpos_t saved_pos;
        if (fgetpos(fp, &saved_pos) != 0) {
            set_entry_error(entry, DSIO_ERR_FTELL_FAILED, "fgetpos() failed in calculate_vb_emulated_size");
            return -1;
        }
        
        /* rewind() == fseek(0,SEEK_SET) + clearerr(), more robust for record-mode files */
        rewind(fp);
        if (ferror(fp)) {
            set_entry_error(entry, DSIO_ERR_FSEEK_FAILED, "rewind() failed in calculate_vb_emulated_size");
            fsetpos(fp, &saved_pos);
            return -1;
        }
        
        /* Read each record, stripping trailing spaces, counting bytes+newline */
        while (1) {
            size_t rc = fread(entry->rec_buf, 1, entry->rec_buf_size, fp);
            if (rc == 0) break;  /* EOF */
            
            /* Strip trailing spaces */
            while (rc > 0 && entry->rec_buf[rc - 1] == ' ') {
                rc--;
            }
            total_size += rc + 1;  /* +1 for newline */
            rec_count++;
        }
        
        /* Restore position */
        if (fsetpos(fp, &saved_pos) != 0) {
            DSIO_LOG_DEBUG("calculate_vb_emulated_size: WARNING - failed to restore file position\n");
        }
    }
    
    DSIO_LOG_DEBUG("calculate_vb_emulated_size: %zu bytes (%zu records)\n", total_size, rec_count);
    
    return (ssize_t)total_size;
}

ssize_t dsio_get_size(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        errno = EBADF;
        return -1;
    }
    
    DatasetEntry* entry = ENTRY_TO(dd);
    if (!entry->file_ptr) {
        errno = EBADF;
        return -1;
    }
    
    /* Return cached size if available */
    if (entry->size_calculated) {
        return (ssize_t)entry->cached_size;
    }
    
    FILE* fp = entry->file_ptr;
    ssize_t emulated_size;

    if (entry->is_fixed_recfm && entry->reclen > 0) {
        /*
         * FB (binary mode): fseek(SEEK_END) is supported.
         * Native size in bytes divided by reclen gives record count;
         * add one newline per record for the emulated stream size.
         */
        fpos_t pos;
        if (fgetpos(fp, &pos) != 0) {
            return -1;
        }
        if (fseek(fp, 0, SEEK_END) != 0) {
            fsetpos(fp, &pos);
            return -1;
        }
        long native_size = ftell(fp);
        if (fsetpos(fp, &pos) != 0) {
            DSIO_LOG_DEBUG("dsio_get_size: WARNING - fsetpos() failed\n");
        }
        if (native_size < 0) {
            return -1;
        }
        size_t num_records = (size_t)native_size / entry->reclen;
        emulated_size = (ssize_t)(native_size + num_records);
        DSIO_LOG_DEBUG("dsio_get_size: fd=%d FB native=%ld reclen=%zu emulated=%zd\n",
                     fd, native_size, entry->reclen, emulated_size);
    } else {
        /*
         * VB/U (type=record mode): fseek(SEEK_END) is NOT supported by the
         * z/OS C runtime for variable-length record files.
         * calculate_vb_emulated_size() handles its own fgetpos/fread/fsetpos.
         */
        emulated_size = calculate_vb_emulated_size(fp, entry);
        DSIO_LOG_DEBUG("dsio_get_size: fd=%d VB/U emulated=%zd\n", fd, emulated_size);
    }
    
    if (emulated_size >= 0) {
        entry->cached_size = (size_t)emulated_size;
        entry->size_calculated = 1;
    }
    
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

#endif /* ZOSLIB_ENABLE_DATASETIO */

