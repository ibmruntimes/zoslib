#define _XOPEN_SOURCE_EXTENDED 1
#define _EXT 1

#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <env.h>
#include <unistd.h>
#include <dynit.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdarg.h>
#include <ctype.h>

#include "zos-datasetio.h"
#include "ispf.h"
#include "ztime.h"
#include "ihapds.h"
#include "ispf_reader.h"

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
  DatasetEntryEnhanced* dentry = create_enhanced_entry(dd, 1047);
  if (!dentry) {
    fclose(dd);
    return -1;
  }
  
  parse_and_store_name(dentry, tmplate);
  update_global_stats_open();

  fd = GET_DUMMY_FD();
  ADD_DD(fd, dentry);
  
  log_info("Created temporary dataset: %s (fd=%d)", tmplate, fd);
  
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

  void* dd;
  const char* fopen_mode;
  unsigned short file_ccsid;

  int fd = -1;
  bool pds_member = strchr(name, '(');

  if ((flags & O_APPEND) && (pds_member)) {
    errno = EINVAL;
    return -1;
  } else if (flags & O_RDONLY) {
    fopen_mode = "r,recfm=+";
  } else if (flags & O_WRONLY) {
    if (flags & O_APPEND) {
      fopen_mode = "a,recfm=+";
    } else {
      fopen_mode = "w,recfm=+";
    }
  } else if (flags & O_RDWR) {
    if (flags & O_APPEND) {
      fopen_mode = "a+,recfm=+";
    } else {
      fopen_mode = "r+,recfm=+";
    }
  } else {
    errno = EINVAL;
    return -1;
  }

  if ((flags & O_LARGEFILE) || (flags & O_NOCTTY) || (flags & O_NONBLOCK)) {
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
      errno = EINVAL;
      return -1;
    }
  }

  DEBUG_PRINT1("Open with mode %s\n", fopen_mode);
  dd = fopen(name, fopen_mode);
  if (!dd) {
    perror("dataset open failed");
    return -1;
  }

  /* Use enhanced entry creation with metadata loading */
  DatasetEntryEnhanced* dentry = create_enhanced_entry(dd, 1047);
  if (!dentry) {
    fclose(dd);
    return -1;
  }
  
  /* Parse and store dataset name components */
  parse_and_store_name(dentry, name);
  
  /* Update global statistics */
  update_global_stats_open();
  
  fd = GET_DUMMY_FD();
  ADD_DD(fd, dentry);
  
  DEBUG_PRINT1("Opened dataset: %s (fd=%d, RECFM=%s, LRECL=%d)\n",
           name, fd,
           dsio_recfm_to_string(dentry->recfm),
           dentry->lrecl);
  
  return fd;
}

ssize_t write_dataset(int fd, const void* buf, size_t count)
{
  void* dd = GET_DD(fd);

  DatasetEntryEnhanced* dentry = (DatasetEntryEnhanced*) (dd);
  FILE* fp = dentry->file_ptr;

  DEBUG_PRINT1("In Write, File ptr ccsid: %p\n", dentry->file_ptr);
  DEBUG_PRINT1("In Write, File ccsid: %d\n", dentry->file_ccsid);
  DEBUG_PRINT1("In Write, Program ccsid: %d\n", dentry->program_ccsid);
  DEBUG_PRINT1("In Write, Conversion state %d\n", dentry->conversion_state);
  DEBUG_PRINT1("write_dataset fd %d count %d\n", fd, count);

  char* write_buffer = (char *)malloc(count);
  if (write_buffer == NULL) {
    set_entry_error(dentry, DSIO_ERR_ALLOC_FAILED, "Memory allocation failed for write buffer");
    fprintf(stderr, "Memory allocation failed\n");
    return -1;
  }
  memcpy(write_buffer, buf, count);

  write_buffer = convertBuffer(write_buffer, dentry->program_ccsid, dentry->file_ccsid);
  size_t rc = fwrite(write_buffer, 1, count, fp);
  free(write_buffer);
  
  if (rc == count) {
    /* Update statistics on successful write */
    update_write_stats(dentry, rc);
    DEBUG_PRINT1("Wrote %zu bytes to fd=%d\n", rc, fd);
    return (ssize_t) rc;
  } else {
    set_entry_error(dentry, DSIO_ERR_WRITE_FAILED, "fwrite() returned fewer bytes than requested");
    return -1;
  }
}

ssize_t read_dataset(int fd, void* buf, size_t count)
{
  void* dd = GET_DD(fd);

  DatasetEntryEnhanced* dentry = (DatasetEntryEnhanced*) (dd);
  FILE* fp = dentry->file_ptr;

  DEBUG_PRINT1("In Read, File ptr ccsid: %p\n", dentry->file_ptr);
  DEBUG_PRINT1("In Read, File ccsid: %d\n", dentry->file_ccsid);
  DEBUG_PRINT1("In Read, Program ccsid: %d\n", dentry->program_ccsid);
  DEBUG_PRINT1("In Read, Conversion state %d\n", dentry->conversion_state);
  DEBUG_PRINT1("read_dataset fd %d count %d\n", fd, count);
  size_t rc = fread(buf, 1, count, fp);

  if (rc > 0) {
    buf = convertBuffer(buf, dentry->file_ccsid, dentry->program_ccsid);
    /* Update statistics on successful read */
    update_read_stats(dentry, rc);
    DEBUG_PRINT1("Read %zu bytes from fd=%d\n", rc, fd);
  } else if (rc == 0 && ferror(fp)) {
    set_entry_error(dentry, DSIO_ERR_READ_FAILED, "fread() failed");
    return -1;
  }

  return (ssize_t) rc;
}

int close_dataset(int fd)
{
  void* dd = GET_DD(fd);

  DatasetEntryEnhanced* dentry = (DatasetEntryEnhanced*) (dd);
  FILE* fp = dentry->file_ptr;

  DEBUG_PRINT1("Closing dataset fd=%d (read=%zu bytes, wrote=%zu bytes, ops=%zu/%zu)\n",
           fd, dentry->bytes_read, dentry->bytes_written,
           dentry->read_operations, dentry->write_operations);

  int rc = fclose(fp);
  if (!rc) {
    /* Update global statistics */
    update_global_stats_close();
    
    close(fd);
    
    /* Deallocate enhanced DatasetEntry */
    free_enhanced_entry(dentry);
    CLEAR_DD(fd);
  } else {
    set_entry_error(dentry, DSIO_ERR_CLOSE_FAILED, "fclose() failed");
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

  DEBUG_PRINT1("Allocate dataset: %s\n", mvs_style_dataset);

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
    /* Validate file descriptor */
    if (fd < 0 || fd >= MAX_FDS) {
        errno = EBADF;
        log_error("lseek_dataset: Invalid fd=%d", fd);
        return -1;
    }
    
    void* dd = GET_DD(fd);
    if (!dd) {
        errno = EBADF;
        log_error("lseek_dataset: No dataset descriptor for fd=%d", fd);
        return -1;
    }
    
    /* Check if this is actually a dataset fd (not a regular file fd) */
    if (IS_FD(fd)) {
        errno = EINVAL;
        log_error("lseek_dataset: fd=%d is not a dataset descriptor", fd);
        return -1;
    }
    
    DatasetEntryEnhanced* dentry = (DatasetEntryEnhanced*)dd;
    if (!dentry->file_ptr) {
        errno = EBADF;
        set_entry_error(dentry, DSIO_ERR_INVALID_FD, "Invalid file pointer");
        log_error("lseek_dataset: NULL file pointer for fd=%d", fd);
        return -1;
    }
    
    FILE* fp = dentry->file_ptr;
    
    /* Log the seek operation with correct format specifiers */
    DEBUG_PRINT1("lseek_dataset fd=%d offset=%lld whence=%d\n", fd, (long long)offset, whence);
    
    /* Validate whence parameter */
    int fseek_whence;
    switch (whence) {
        case SEEK_SET:
            fseek_whence = SEEK_SET;
            break;
        case SEEK_CUR:
            fseek_whence = SEEK_CUR;
            break;
        case SEEK_END:
            fseek_whence = SEEK_END;
            break;
        default:
            errno = EINVAL;
            set_entry_error(dentry, DSIO_ERR_FSEEK_FAILED, "Invalid whence parameter");
            log_error("lseek_dataset: Invalid whence=%d for fd=%d", whence, fd);
            return -1;
    }
    
    /* Load metadata if not already loaded to check record format constraints */
    if (!dentry->metadata_loaded) {
        if (load_metadata_from_file(dentry) != 0) {
            DEBUG_PRINT1("lseek_dataset: Failed to load metadata for fd=%d, continuing anyway", fd);
        }
    }
    
    /* Check for dataset-specific constraints based on record format
     * Note: On z/OS, fseek/ftell behavior varies by record format:
     * - Fixed (F, FB): Seeking works by byte offset, relatively predictable
     * - Variable (V, VB): Seeking by byte offset is problematic due to RDWs
     * - Undefined (U): Seeking is unreliable and not recommended
     * - ASA formats: Additional complexity with carriage control characters
     */
    if (dentry->metadata_loaded) {
        dsio_recfm_t recfm = dentry->recfm;
        
        /* Variable-length records: seeking by byte offset doesn't align with record boundaries */
        if (recfm == DSIO_RECFM_V || recfm == DSIO_RECFM_VB || 
            recfm == DSIO_RECFM_VA || recfm == DSIO_RECFM_VBA) {
            DEBUG_PRINT1("lseek_dataset: Seeking in variable-length dataset (RECFM=%s) - byte offsets may not align with record boundaries",
                     dsio_recfm_to_string(recfm));
        }
        
        /* Undefined format: seeking is not reliable */
        if (recfm == DSIO_RECFM_U) {
            DEBUG_PRINT0("lseek_dataset: Seeking in undefined format dataset (RECFM=U) is not recommended and may produce unexpected results");
        }
        
        /* For fixed-length records, automatically align offset to record boundaries */
        if ((recfm == DSIO_RECFM_F || recfm == DSIO_RECFM_FB || 
             recfm == DSIO_RECFM_FA || recfm == DSIO_RECFM_FBA) && 
            whence == SEEK_SET && dentry->lrecl > 0) {
            /* Check if offset aligns with record boundaries */
            if (offset % dentry->lrecl != 0) {
                off_t aligned_offset = (offset / dentry->lrecl) * dentry->lrecl;
                DEBUG_PRINT1("lseek_dataset: Offset %lld not aligned with LRECL %d, rounding down to %lld\n",
                         (long long)offset, dentry->lrecl, (long long)aligned_offset);
                offset = aligned_offset;
            }
        }
    }
    
    /* Perform the seek operation using fseeko() to handle off_t properly */
    if (fseeko(fp, offset, fseek_whence) != 0) {
        int saved_errno = errno;
        set_entry_error(dentry, DSIO_ERR_FSEEK_FAILED, "fseek() failed");
        DEBUG_PRINT1("lseek_dataset: fseek() failed for fd=%d, offset=%lld, whence=%d, errno=%d (%s)",
                 fd, (long long)offset, whence, saved_errno, strerror(saved_errno));
        errno = saved_errno;
        return -1;
    }
    
    /* Get the new position using ftello() to return off_t */
    off_t pos = ftello(fp);
    if (pos < 0) {
        int saved_errno = errno;
        set_entry_error(dentry, DSIO_ERR_FTELL_FAILED, "ftello() failed");
        DEBUG_PRINT1("lseek_dataset: ftello() failed for fd=%d, errno=%d (%s)", 
                 fd, saved_errno, strerror(saved_errno));
        errno = saved_errno;
        return -1;
    }
    
    DEBUG_PRINT1("lseek_dataset: fd=%d, offset=%lld, whence=%d, new_pos=%lld\n",
             fd, (long long)offset, whence, (long long)pos);
    
    return pos;
}

/* ========================================================================
 * fstat() / stat() - File Metadata
 * ======================================================================== */

/* Helper function to read ISPF statistics from PDS member */
#if 0
static int read_ispf_stats(FILE* fp, struct ispf_stats* stats) {
    if (!fp || !stats) {
        return -1;
    }
    
    /* Get file data to check if this is a PDS member */
    fldata_t fdata;
    if (fldata(fp, NULL, &fdata) != 0) {
        return -1;
    }
    
    /* Check if this is a PDS/PDSE member */
    if (fdata.__dsorgPO == 0) {
        return -1;  /* Not a PDS/PDSE */
    }
    
    /* Get member name from fldata */
    char member_name[9] = {0};
    if (fdata.__dsname == NULL) {
        return -1;  /* No member name */
    }
    memcpy(member_name, fdata.__dsname, 8);
    member_name[8] = '\0';
    
    /* Use the ISPF reader module to get statistics */
    return read_member_ispf_stats(fp, member_name, stats);
}
#endif

int fstat_dataset(int fd, struct stat *statbuf) {
    DEBUG_PRINT1("Enter fstat_dataset %d\n", 0);
    if (!statbuf) {
        errno = EINVAL;
        return -1;
    }
    
    void* dd = GET_DD(fd);
    if (!dd) {
        errno = EBADF;
        return -1;
    }
    
    DatasetEntryEnhanced* dentry = (DatasetEntryEnhanced*)dd;
    FILE* fp = dentry->file_ptr;
    
    if (!fp) {
        set_entry_error(dentry, DSIO_ERR_INVALID_FD, "Invalid file pointer");
        errno = EBADF;
        return -1;
    }
    DEBUG_PRINT1("fstat_dataset %d\n", 1);
    
    /* Initialize stat buffer */
    memset(statbuf, 0, sizeof(struct stat));
    
    /* Ensure metadata is loaded */
    if (!dentry->metadata_loaded) {
        if (load_metadata_from_file(dentry) != 0) {
            log_warn("fstat: Failed to load metadata, continuing with limited info");
        }
    }
    
    /* Use fldata() to get dataset information */
    fldata_t fdata;
    if (fldata(fp, NULL, &fdata) != 0) {
        set_entry_error(dentry, DSIO_ERR_FLDATA_FAILED, "fldata() failed");
        errno = EIO;
        return -1;
    }
    
    /* Get file size using utility function */
    ssize_t size = dsio_get_size(fd);
    if (size >= 0) {
        statbuf->st_size = size;
    } else {
        log_warn("fstat: Failed to get file size");
        statbuf->st_size = 0;
    }
    
    /* Determine file mode based on dataset type */
    mode_t mode = S_IFREG;  /* Regular file by default */
    
    /* Check if PDS/PDSE without member (acts like directory) */
    if ((dsio_is_pds(fd) || dsio_is_pdse(fd)) && !dsio_has_member(fd)) {
        mode = S_IFDIR;
    }
    
    /* Set permissions based on readonly status */
    if (dsio_is_readonly(fd)) {
        mode |= S_IRUSR | S_IRGRP | S_IROTH;  /* Read-only */
    } else {
        mode |= S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;  /* Read-write for user */
    }
    
    /* Add execute permission for directories */
    if (mode & S_IFDIR) {
        mode |= S_IXUSR | S_IXGRP | S_IXOTH;
    }
    
    statbuf->st_mode = mode;
    statbuf->st_nlink = 1;
    
    /* Set block size from metadata or fldata */
    if (dentry->metadata_loaded && dentry->blksize > 0) {
        statbuf->st_blksize = dentry->blksize;
    } else if (fdata.__blksize > 0) {
        statbuf->st_blksize = fdata.__blksize;
    } else {
        statbuf->st_blksize = 4096;  /* Default */
    }
    
    /* Calculate blocks used (in 512-byte units for POSIX compatibility) */
    if (statbuf->st_size > 0) {
        statbuf->st_blocks = (statbuf->st_size + 511) / 512;
    }
    
    /* Set device and inode numbers (simulated for datasets) */
    statbuf->st_dev = 0x5A05;  /* 'ZOS' in hex */
    
    /* Generate pseudo-inode from dataset name components */
    unsigned long inode = 0;
    if (dentry->hlq[0]) {
        for (int i = 0; dentry->hlq[i] && i < DSIO_MAX_QUALIFIER; i++) {
            inode = (inode * 31) + (unsigned char)dentry->hlq[i];
        }
    }
    if (dentry->member_name[0]) {
        for (int i = 0; dentry->member_name[i] && i < DSIO_MAX_MEMBER_NAME; i++) {
            inode = (inode * 31) + (unsigned char)dentry->member_name[i];
        }
    }
    statbuf->st_ino = inode ? inode : 1;
    
    /* Set user and group IDs */
    statbuf->st_uid = getuid();
    statbuf->st_gid = getgid();
    
    /* Try to get ISPF statistics for timestamps (PDS members only) */
    int has_ispf = 0;
    if (dsio_has_member(fd) && (dsio_is_pds(fd) || dsio_is_pdse(fd))) {
        struct ispf_stats ispf_stats;
	/*
        if (read_ispf_stats(fp, &ispf_stats) == 0) {
            has_ispf = 1;
            
            // Convert struct tm to time_t
            statbuf->st_ctime = mktime(&ispf_stats.create_time);
            statbuf->st_atime = statbuf->st_ctime;
            
            statbuf->st_mtime = mktime(&ispf_stats.mod_time);
            if (statbuf->st_mtime == -1) {
                statbuf->st_mtime = statbuf->st_ctime;
            }
            
            log_debug("fstat: Using ISPF stats - created=%ld, modified=%ld, ver=%d.%d",
                      (long)statbuf->st_ctime, (long)statbuf->st_mtime,
                      ispf_stats.ver_num, ispf_stats.mod_num);
        }
    	*/
    }
    
    /* If no ISPF stats, try file system times or use current time */
    if (!has_ispf) {
        int fileno_val = fileno(fp);
        struct stat fs_stat;
        
        if (fileno_val >= 0 && fstat(fileno_val, &fs_stat) == 0) {
            statbuf->st_atime = fs_stat.st_atime;
            statbuf->st_mtime = fs_stat.st_mtime;
            statbuf->st_ctime = fs_stat.st_ctime;
            log_trace("fstat: Using file system timestamps");
        } else {
            /* Fallback to current time */
            time_t now = time(NULL);
            statbuf->st_atime = now;
            statbuf->st_mtime = now;
            statbuf->st_ctime = now;
            log_trace("fstat: Using current time for timestamps");
        }
    }
    
    DEBUG_PRINT1("fstat: fd=%d, size=%ld, blksize=%ld, blocks=%ld, mode=%o, inode=%lu",
              fd, (long)statbuf->st_size, (long)statbuf->st_blksize, 
              (long)statbuf->st_blocks, statbuf->st_mode, (unsigned long)statbuf->st_ino);
    
    return 0;
}

int stat_dataset(const char *pathname, struct stat *statbuf) {
    if (!pathname || !statbuf) {
        errno = EINVAL;
        return -1;
    }
    
    /* Open the dataset temporarily to get stats */
    int fd = open_dataset(pathname, O_RDONLY, 0);
    if (fd < 0) {
        return -1;
    }
    
    int result = fstat_dataset(fd, statbuf);
    close_dataset(fd);
    
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
    /* This is a placeholder - real implementation would use BPAM or ISPF services */
    
    log_warn("PDS directory reading not fully implemented yet for: %s", dataset_name);
    
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
        log_error("opendir: Cannot open PDS member as directory: %s", name);
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
        log_error("opendir: Failed to read PDS directory: %s", name);
        return NULL;
    }
    
    log_info("opendir: Opened PDS directory: %s (%d members)", name, dir->member_count);
    
    return (DIR*)dir;
}

DIR* opendir_zos(const char *name) {
    if (IS_DATASET(name)) {
        DEBUG_PRINT0("calling opendir-dataset\n");
        return opendir_dataset(name);
    } else {
        DEBUG_PRINT0("calling opendir-file\n");
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
    
    log_trace("readdir: Read member: %s", entry.d_name);
    
    return &entry;
}

struct dirent* readdir_zos(DIR *dirp) {
    if (!dirp) {
        return NULL;
    }
    
    /* Check if this is a dataset directory */
    DatasetDir* dir = (DatasetDir*)dirp;
    if (dir->is_dataset_dir) {
        DEBUG_PRINT0("calling readdir-dataset\n");
        return readdir_dataset(dirp);
    } else {
        DEBUG_PRINT0("calling readdir-file\n");
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
    
    log_info("closedir: Closed PDS directory: %s", dir->dataset_name);
    
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
        DEBUG_PRINT0("calling closedir-dataset\n");
        return closedir_dataset(dirp);
    } else {
        DEBUG_PRINT0("calling closedir-file\n");
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
    return entry->last_error;
}

const char* dsio_get_error_message(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return "Invalid file descriptor";
    }
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
    return entry->error_message;
}

void dsio_clear_error(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return;
    }
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
    entry->last_error = DSIO_SUCCESS;
    entry->error_message[0] = '\0';
}

void set_entry_error(DatasetEntryEnhanced* entry, dsio_error_t error, const char* message) {
    if (!entry) return;
    
    entry->last_error = error;
    if (message) {
        strncpy(entry->error_message, message, DSIO_MAX_ERROR_MSG - 1);
        entry->error_message[DSIO_MAX_ERROR_MSG - 1] = '\0';
    } else {
        strncpy(entry->error_message, dsio_strerror(error), DSIO_MAX_ERROR_MSG - 1);
        entry->error_message[DSIO_MAX_ERROR_MSG - 1] = '\0';
    }
    
    log_error("Error %d: %s", error, entry->error_message);
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
    
    log_warn("Unsupported CCSID conversion: %d -> %d", from_ccsid, to_ccsid);
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

void dsio_debug_print(const char* str) {
    if (g_log_stream == NULL) {
        g_log_stream = fopen("zoslib.debug.log", "a");
    }
    if (g_log_stream) {
        fprintf(g_log_stream, "%s", str);
        fflush(g_log_stream);
    }
}

void dsio_debug_printf(const char* format, ...) {
    if (g_log_stream == NULL) {
        g_log_stream = fopen("zoslib.debug.log", "a");
    }
    if (g_log_stream) {
        va_list args;
        va_start(args, format);
        vfprintf(g_log_stream, format, args);
        va_end(args);
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

void update_read_stats(DatasetEntryEnhanced* entry, size_t bytes) {
    if (!entry || !entry->stats_enabled) return;
    
    entry->bytes_read += bytes;
    entry->read_operations++;
    
    if (g_stats.stats_enabled) {
        g_stats.total_bytes_read += bytes;
        g_stats.total_read_operations++;
    }
}

void update_write_stats(DatasetEntryEnhanced* entry, size_t bytes) {
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
    
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
    
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

int load_metadata_from_file(DatasetEntryEnhanced* entry) {
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
    
    log_debug("Loaded metadata: RECFM=%s, LRECL=%d, BLKSIZE=%d",
              dsio_recfm_to_string(entry->recfm),
              entry->lrecl,
              entry->blksize);
    
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

DatasetEntryEnhanced* create_enhanced_entry(FILE* fp, unsigned short file_ccsid) {
    DatasetEntryEnhanced* entry = calloc(1, sizeof(DatasetEntryEnhanced));
    if (!entry) {
        log_error("Failed to allocate DatasetEntryEnhanced");
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

void free_enhanced_entry(DatasetEntryEnhanced* entry) {
    if (entry) {
        free(entry);
    }
}

int parse_and_store_name(DatasetEntryEnhanced* entry, const char* dataset_name) {
    if (!entry || !dataset_name) {
        return -1;
    }
    
    dsio_name_parts_t parts;
    if (dsio_parse_dataset_name(dataset_name, &parts) != 0) {
        set_entry_error(entry, DSIO_ERR_INVALID_NAME, "Failed to parse dataset name");
        return -1;
    }
    
    /* Store components */
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
    
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
    
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
    return entry->is_pds_member;
}

int dsio_is_readonly(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return 0;
    }
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
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
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
    if (!entry->file_ptr) {
        return -1;
    }
    
    /* Check file size */
    long current_pos = ftell(entry->file_ptr);
    if (current_pos < 0) {
        return -1;
    }
    
    if (fseek(entry->file_ptr, 0, SEEK_END) != 0) {
        return -1;
    }
    
    long size = ftell(entry->file_ptr);
    fseek(entry->file_ptr, current_pos, SEEK_SET);
    
    return (size == 0) ? 1 : 0;
}

ssize_t dsio_get_size(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
    if (!entry->file_ptr) {
        return -1;
    }
#if 0 
    /* Get file size */
    long current_pos = ftell(entry->file_ptr);
    if (current_pos < 0) {
        return -1;
    }
    
    if (fseek(entry->file_ptr, 0, SEEK_END) != 0) {
        return -1;
    }
    
    long size = ftell(entry->file_ptr);
    fseek(entry->file_ptr, current_pos, SEEK_SET);
    
    return (ssize_t)size;
#endif
    long long total = 0;
    char buf[8192];

    long cur = ftell(entry->file_ptr);
    fseek(entry->file_ptr, 0, SEEK_SET);

    while (1) {
        size_t n = fread(buf, 1, sizeof(buf), entry->file_ptr);
        total += n;
        if (n == 0)
            break;
    }
    DEBUG_PRINT1("get_size fd %d size %d\n", fd, total);

    fseek(entry->file_ptr, cur, SEEK_SET);
    return (ssize_t)total;
}

int dsio_flush(int fd) {
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
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
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
    
    /* Store CCSID configuration */
    entry->file_ccsid = config->source_ccsid;
    entry->program_ccsid = config->target_ccsid;
    entry->conversion_state = config->conversion_enabled ? 1 : 0;
    
    log_debug("Set CCSID config: source=%d, target=%d, enabled=%d",
              config->source_ccsid, config->target_ccsid, config->conversion_enabled);
    
    return 0;
}

int dsio_get_ccsid_config(int fd, dsio_ccsid_config_t* config) {
    if (!config) return -1;
    
    void* dd = GET_DD(fd);
    if (!dd || IS_FD(fd)) {
        return -1;
    }
    
    DatasetEntryEnhanced* entry = ENTRY_TO_ENHANCED(dd);
    
    config->source_ccsid = entry->file_ccsid;
    config->target_ccsid = entry->program_ccsid;
    config->conversion_enabled = entry->conversion_state;
    config->auto_detect = 0;
    
    return 0;
}



// Made with Bob
