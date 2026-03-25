#ifndef __DATASET_IO__
#define __DATASET_IO__ 1

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef __ssize_t
  #define __ssize_t 1
  typedef signed long ssize_t;
#endif
#ifndef __size_t
  #define __size_t 1
  typedef unsigned long size_t;
#endif
#ifndef __mode_t
  #define __mode_t  1
  typedef int mode_t ;
#endif

int open_dataset(const char* name, int flags, mode_t mode);
int close_dataset(int fd);
int mkstemp_dataset(char* tmplate);
ssize_t write_dataset(int fd, const void* buf, size_t count);
ssize_t read_dataset(int fd, void* buf, size_t count);
off_t lseek_dataset(int fd, off_t offset, int whence);
int stat_dataset(const char* pathname, struct stat *statbuf);
int fstat_dataset(int fd, struct stat *statbuf);

/*
 * The following functions are only required for testing, 
 * not for the shared library.
 *
 * Datasets are always in the form: //'<dataset>' or //'<dataset>(<member>)'
 */
char* temp_file_name(char* result);
char* temp_dataset_name(char* result);
int allocate_dataset(const char* dataset);
int delete_dataset(const char* dataset);

/* ========================================================================
 * ERROR HANDLING
 * ======================================================================== */

/* Comprehensive error codes for better diagnostics */
typedef enum {
    DSIO_SUCCESS = 0,
    DSIO_ERR_INVALID_NAME,
    DSIO_ERR_NAME_TOO_LONG,
    DSIO_ERR_OPEN_FAILED,
    DSIO_ERR_READ_FAILED,
    DSIO_ERR_WRITE_FAILED,
    DSIO_ERR_CLOSE_FAILED,
    DSIO_ERR_ALLOC_FAILED,
    DSIO_ERR_INVALID_FD,
    DSIO_ERR_INVALID_RECFM,
    DSIO_ERR_INVALID_DSORG,
    DSIO_ERR_CCSID_CONVERSION,
    DSIO_ERR_BUFFER_OVERFLOW,
    DSIO_ERR_MEMBER_NOT_FOUND,
    DSIO_ERR_NOT_A_DATASET,
    DSIO_ERR_FLDATA_FAILED,
    DSIO_ERR_FSEEK_FAILED,
    DSIO_ERR_FTELL_FAILED,
    DSIO_ERR_RECORD_TOO_LONG,
    DSIO_ERR_UNSUPPORTED_OPERATION,
    DSIO_ERR_INTERNAL_ERROR
} dsio_error_t;

/* Get error message for error code */
const char* dsio_strerror(dsio_error_t error);

/* Get last error for a file descriptor */
dsio_error_t dsio_get_last_error(int fd);

/* Get detailed error message for a file descriptor */
const char* dsio_get_error_message(int fd);

/* Clear error state for a file descriptor */
void dsio_clear_error(int fd);

/* ========================================================================
 * DATASET METADATA
 * ======================================================================== */

/* Record formats */
typedef enum {
    DSIO_RECFM_UNKNOWN = 0,
    DSIO_RECFM_F = 1,      /* Fixed */
    DSIO_RECFM_V = 2,      /* Variable */
    DSIO_RECFM_U = 3,      /* Undefined */
    DSIO_RECFM_FB = 4,     /* Fixed Blocked */
    DSIO_RECFM_VB = 5,     /* Variable Blocked */
    DSIO_RECFM_FBA = 6,    /* Fixed Blocked ASA */
    DSIO_RECFM_VBA = 7,    /* Variable Blocked ASA */
    DSIO_RECFM_FA = 8,
    DSIO_RECFM_VA = 9
} dsio_recfm_t;

/* Dataset organizations */
typedef enum {
    DSIO_DSORG_UNKNOWN = 0,
    DSIO_DSORG_PS = 1,     /* Physical Sequential */
    DSIO_DSORG_PO = 2,     /* Partitioned (PDS) */
    DSIO_DSORG_POE = 3,    /* Partitioned Extended (PDSE) */
    DSIO_DSORG_DA = 4,     /* Direct Access */
    DSIO_DSORG_VS = 5      /* VSAM */
} dsio_dsorg_t;

/* Dataset metadata structure */
typedef struct {
    dsio_recfm_t recfm;           /* Record format */
    dsio_dsorg_t dsorg;           /* Dataset organization */
    uint16_t lrecl;               /* Logical record length */
    uint32_t blksize;             /* Block size */
    uint16_t file_ccsid;          /* File CCSID */
    uint16_t program_ccsid;       /* Program CCSID */
    char member_name[9];          /* Member name (if PDS/PDSE) */
    char hlq[9];                  /* High-level qualifier */
    char llq[9];                  /* Low-level qualifier */
    int is_pds_member;            /* 1 if this is a PDS member */
    int readonly;                 /* 1 if read-only */
} dsio_metadata_t;

/* Get dataset metadata */
int dsio_get_metadata(int fd, dsio_metadata_t* metadata);

/* Get specific metadata fields */
int dsio_get_recfm(int fd, dsio_recfm_t* recfm);
int dsio_get_lrecl(int fd, uint16_t* lrecl);
int dsio_get_dsorg(int fd, dsio_dsorg_t* dsorg);
int dsio_get_ccsid(int fd, uint16_t* file_ccsid, uint16_t* program_ccsid);
int dsio_get_member_name(int fd, char* member, size_t len);
int dsio_get_hlq(int fd, char* hlq, size_t len);
int dsio_get_llq(int fd, char* llq, size_t len);

/* Check dataset properties */
int dsio_is_pds(int fd);
int dsio_is_pdse(int fd);
int dsio_is_sequential(int fd);
int dsio_has_member(int fd);
int dsio_is_readonly(int fd);

/* ========================================================================
 * RECORD FORMAT UTILITIES
 * ======================================================================== */

/* Check if record format has length prefix */
int dsio_has_length_prefix(dsio_recfm_t recfm);

/* Check if record format is blocked */
int dsio_is_blocked(dsio_recfm_t recfm);

/* Check if record format has ASA control characters */
int dsio_has_asa(dsio_recfm_t recfm);

/* Convert record format to string */
const char* dsio_recfm_to_string(dsio_recfm_t recfm);

/* Convert dataset organization to string */
const char* dsio_dsorg_to_string(dsio_dsorg_t dsorg);

/* Parse record format from string */
dsio_recfm_t dsio_string_to_recfm(const char* str);

/* Parse dataset organization from string */
dsio_dsorg_t dsio_string_to_dsorg(const char* str);

/* ========================================================================
 * CCSID CONVERSION
 * ======================================================================== */

/* CCSID configuration */
typedef struct {
    uint16_t source_ccsid;
    uint16_t target_ccsid;
    int auto_detect;              /* Auto-detect source CCSID */
    int conversion_enabled;       /* Enable/disable conversion */
} dsio_ccsid_config_t;

/* Set CCSID configuration for a file descriptor */
int dsio_set_ccsid_config(int fd, const dsio_ccsid_config_t* config);

/* Get CCSID configuration for a file descriptor */
int dsio_get_ccsid_config(int fd, dsio_ccsid_config_t* config);

/* Convert buffer between CCSIDs (standalone utility) */
void* dsio_convert_buffer(void* buf, size_t len, 
                          uint16_t from_ccsid, 
                          uint16_t to_ccsid);

/* ========================================================================
 * DATASET NAME PARSING
 * ======================================================================== */

/* Dataset name components */
typedef struct {
    char full_name[55];           /* Full dataset name */
    char hlq[9];                  /* High-level qualifier */
    char mlqs[45];                /* Mid-level qualifiers */
    char llq[9];                  /* Low-level qualifier */
    char member[9];               /* Member name (if any) */
    int has_member;               /* 1 if member specified */
    int is_quoted;                /* 1 if name was quoted */
} dsio_name_parts_t;

/* Parse dataset name into components */
int dsio_parse_dataset_name(const char* name, dsio_name_parts_t* parts);

/* Validate dataset name */
int dsio_validate_dataset_name(const char* name);

/* Check if name is a dataset (starts with //) */
int dsio_is_dataset_name(const char* name);

/* ========================================================================
 * LOGGING AND DEBUGGING
 * ======================================================================== */

/* Log levels */
typedef enum {
    DSIO_LOG_NONE = 0,
    DSIO_LOG_ERROR,
    DSIO_LOG_WARN,
    DSIO_LOG_INFO,
    DSIO_LOG_DEBUG,
    DSIO_LOG_TRACE
} dsio_log_level_t;

/* Set global log level */
void dsio_set_log_level(dsio_log_level_t level);

/* Set log output stream */
void dsio_set_log_stream(FILE* stream);

/* Enable/disable debug mode */
void dsio_enable_debug(int enable);

/* Log a message (internal use) */
void dsio_log(dsio_log_level_t level, const char* format, ...);

/* ========================================================================
 * STATISTICS AND MONITORING
 * ======================================================================== */

/* I/O statistics */
typedef struct {
    size_t bytes_read;            /* Total bytes read */
    size_t bytes_written;         /* Total bytes written */
    size_t read_operations;       /* Number of read calls */
    size_t write_operations;      /* Number of write calls */
    size_t open_operations;       /* Number of open calls */
    size_t close_operations;      /* Number of close calls */
    size_t errors;                /* Number of errors */
} dsio_stats_t;

/* Get statistics for a file descriptor */
int dsio_get_stats(int fd, dsio_stats_t* stats);

/* Reset statistics for a file descriptor */
int dsio_reset_stats(int fd);

/* Get global statistics */
void dsio_get_global_stats(dsio_stats_t* stats);

/* Reset global statistics */
void dsio_reset_global_stats(void);

/* ========================================================================
 * UTILITY FUNCTIONS
 * ======================================================================== */

/* Get maximum record length for a dataset */
int dsio_get_max_reclen(int fd);

/* Check if dataset is empty */
int dsio_is_empty(int fd);

/* Get dataset size in bytes */
ssize_t dsio_get_size(int fd);

/* Flush any pending writes */
int dsio_flush(int fd);

/* ========================================================================
 * CONSTANTS
 * ======================================================================== */

/* Maximum lengths */
#define DSIO_MAX_DATASET_NAME 54
#define DSIO_MAX_MEMBER_NAME 8
#define DSIO_MAX_QUALIFIER 8
#define DSIO_MAX_ERROR_MSG 256

/* Special CCSID values */
#define DSIO_CCSID_BINARY (-1)
#define DSIO_CCSID_EBCDIC 1047
#define DSIO_CCSID_ASCII 819
#define DSIO_CCSID_UTF8 1208

/* ========================================================================
 * INTERNAL STRUCTURES AND MACROS
 * ======================================================================== */

/* In z/OS, the 32nd bit of a 64-bit address is never on                                              */
/* We can use this as a safe way to distinguish between a 64-bit pointer to a 'dataset descriptor'    */
/* and a 31-bit (unsigned) bit file descriptor (an invalid descriptor will not be stored in the table */

#define MAX_FDS 1024
#define INV_ADDR_BIT  (0x0000000080000000ULL)

#define ADD_FD(fd)    (descriptor_table[(fd)] = ((void*)(((unsigned long long) fd) | INV_ADDR_BIT)))
#define ADD_DD(fd,dd) (descriptor_table[(fd)] = (dd))
#define GET_DD(fd)    (descriptor_table[(fd)])
#define CLEAR_DD(fd)  (descriptor_table[(fd)] = 0)

/* Validate descriptor slot is within bounds and occupied */
#define IS_VALID_SLOT(slot)  ((slot) >= 0 && (slot) < MAX_FDS && descriptor_table[(slot)] != NULL)

/* Check if slot contains a file descriptor (has INV_ADDR_BIT set) */
#define IS_FD(slot)   (IS_VALID_SLOT(slot) && \
                       ((((unsigned long long) (descriptor_table[(slot)])) & INV_ADDR_BIT) != 0))

/* Check if slot contains a dataset descriptor (no INV_ADDR_BIT) */
#define IS_DD(slot)   (IS_VALID_SLOT(slot) && \
                       ((((unsigned long long) (descriptor_table[(slot)])) & INV_ADDR_BIT) == 0))

typedef struct DatasetEntry {
    /* Original fields */
    FILE* file_ptr;
    unsigned short file_ccsid;
    unsigned short process_ccsid;
    unsigned short program_ccsid;
    unsigned char conversion_state;
    
    /* Error tracking */
    dsio_error_t last_error;
    char error_message[DSIO_MAX_ERROR_MSG];
    
    /* Dataset metadata */
    dsio_recfm_t recfm;
    dsio_dsorg_t dsorg;
    uint16_t lrecl;
    uint32_t blksize;
    
    /* Dataset name components */
    char full_path[DSIO_MAX_DATASET_NAME + 1];
    char member_name[DSIO_MAX_MEMBER_NAME + 1];
    char hlq[DSIO_MAX_QUALIFIER + 1];
    char llq[DSIO_MAX_QUALIFIER + 1];
    int is_pds_member;
    int readonly;
    
    /* I/O statistics */
    size_t bytes_read;
    size_t bytes_written;
    size_t read_operations;
    size_t write_operations;
    
    /* State flags */
    int metadata_loaded;
    int stats_enabled;
    
    /* Record buffer for stream emulation */
    char*   rec_buf;          /* internal record I/O buffer */
    size_t  rec_buf_size;     /* allocated size (= blksize or reclen) */
    size_t  rec_buf_len;      /* valid bytes currently in buffer */
    size_t  rec_buf_pos;      /* current read position within buffer */
    size_t  stream_offset;    /* virtual byte offset for lseek */
    int     newline_pending;  /* 1 if we need to emit \n before next record */
    size_t  reclen;           /* logical record length from fldata */
    int     is_fixed_recfm;   /* 1 if FB/FBS - use binary I/O, not type=record */
    int     open_flags;       /* original O_RDONLY/O_WRONLY/O_RDWR flags */
    int     dirty;            /* 1 if buffer has pending writes */
    int     eof_reached;      /* 1 if we reached physical EOF */
    
    /* VB size caching */
    int     vb_size_calculated; /* 1 if VB size has been calculated */
    size_t  vb_cached_size;     /* Cached emulated size for VB datasets */
} DatasetEntry;

/* Directory structure for PDS/PDSE member listing */
typedef struct DatasetDir {
  char dataset_name[256];
  char** member_list;
  int member_count;
  int current_index;
  int is_dataset_dir;
} DatasetDir;

#define GET_DUMMY_FD()   (open("/dev/null", O_WRONLY, 0))
#define IS_DATASET(name) ((name) && ((name)[0] == '/') && ((name)[1] == '/'))

/* Enable/disable logging - set to 1 to enable log_* calls */
#ifndef ZOSLIB_DATASET_LOGGING
  #define ZOSLIB_DATASET_LOGGING 0
#endif

#if ZOSLIB_DATASET_LOGGING
  #define DSIO_LOG_ERROR(fmt, ...) do { if (g_debug_enabled) log_error(fmt, ##__VA_ARGS__); } while(0)
  #define DSIO_LOG_WARN(fmt, ...) do { if (g_debug_enabled) log_warn(fmt, ##__VA_ARGS__); } while(0)
  #define DSIO_LOG_INFO(fmt, ...) do { if (g_debug_enabled) log_info(fmt, ##__VA_ARGS__); } while(0)
  #define DSIO_LOG_DEBUG(fmt, ...) do { if (g_debug_enabled) log_debug(fmt, ##__VA_ARGS__); } while(0)
  #define DSIO_LOG_TRACE(fmt, ...) do { if (g_debug_enabled) log_trace(fmt, ##__VA_ARGS__); } while(0)
#else
  #define DSIO_LOG_ERROR(fmt, ...) ((void)0)
  #define DSIO_LOG_WARN(fmt, ...) ((void)0)
  #define DSIO_LOG_INFO(fmt, ...) ((void)0)
  #define DSIO_LOG_DEBUG(fmt, ...) ((void)0)
  #define DSIO_LOG_TRACE(fmt, ...) ((void)0)
#endif

/* ========================================================================
 * ENHANCED INTERNAL STRUCTURES
 * ======================================================================== */

extern void* descriptor_table[MAX_FDS];



/* Global statistics */
typedef struct {
    size_t total_bytes_read;
    size_t total_bytes_written;
    size_t total_read_operations;
    size_t total_write_operations;
    size_t total_open_operations;
    size_t total_close_operations;
    size_t total_errors;
    int stats_enabled;
} GlobalStats;

/* Global state */
extern GlobalStats g_stats;
extern dsio_log_level_t g_log_level;
extern FILE* g_log_stream;
extern int g_debug_enabled;

/* ========================================================================
 * INTERNAL HELPER FUNCTIONS
 * ======================================================================== */

/* Create dataset entry */
DatasetEntry* create_entry(FILE* fp, unsigned short file_ccsid);

/* Free dataset entry */
void free_entry(DatasetEntry* entry);

/* Load metadata from FILE* using fldata() */
int load_metadata_from_file(DatasetEntry* entry);

/* Parse dataset name and extract components */
int parse_and_store_name(DatasetEntry* entry, const char* dataset_name);

/* Set error on entry */
void set_entry_error(DatasetEntry* entry, dsio_error_t error, const char* message);

/* Update statistics */
void update_read_stats(DatasetEntry* entry, size_t bytes);
void update_write_stats(DatasetEntry* entry, size_t bytes);
void update_global_stats_open(void);
void update_global_stats_close(void);
void update_global_stats_error(void);

/* Logging helpers */
void log_error(const char* format, ...);
void log_warn(const char* format, ...);
void log_info(const char* format, ...);
void log_debug(const char* format, ...);
void log_trace(const char* format, ...);

void dsio_debug_print(const char* str);
void dsio_debug_printf(const char* format, ...);

/* CCSID conversion helpers */
void* convert_ebcdic_to_ascii(void* buf, size_t len);
void* convert_ascii_to_ebcdic(void* buf, size_t len);
void* convert_buffer_ccsid(void* buf, size_t len, uint16_t from, uint16_t to);

/* Dataset name validation */
int validate_dataset_name_internal(const char* name);
int extract_member_name(const char* name, char* member, size_t len);
int extract_qualifiers(const char* name, char* hlq, char* llq, size_t len);

/* Utility macros */
#define ENTRY_TO(entry) ((DatasetEntry*)(entry))
#define IS_ENTRY(entry) ((entry) && ((DatasetEntry*)(entry))->metadata_loaded >= 0)

/* Error message templates */
#define ERR_MSG_INVALID_NAME "Invalid dataset name: %s"
#define ERR_MSG_OPEN_FAILED "Failed to open dataset: %s"
#define ERR_MSG_READ_FAILED "Failed to read from dataset"
#define ERR_MSG_WRITE_FAILED "Failed to write to dataset"
#define ERR_MSG_CLOSE_FAILED "Failed to close dataset"
#define ERR_MSG_ALLOC_FAILED "Memory allocation failed"
#define ERR_MSG_INVALID_FD "Invalid file descriptor: %d"
#define ERR_MSG_FLDATA_FAILED "fldata() failed for dataset"
#define ERR_MSG_CCSID_CONV "CCSID conversion failed: %d -> %d"

#if defined(__cplusplus)
}
#endif

#endif /* __DATASET_IO__ */
