///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020, 2024. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

// BPX4PTR callable service for ptrace and related definitions.

#ifndef ZOS_PTRACE_H_
#define ZOS_PTRACE_H_

#include "zos-macros.h"
#include <sys/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// BPX4PTR (__ptrace) low-level callable service
// https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-ptrace-bpx1ptr-bpx4ptr-control-another-process-debugging
//
__Z_EXPORT void __bpx4ptr(const int32_t *request,
                          const pid_t *pid,
                          void **address,
                          void **data,
                          void **buffer,
                          int32_t *return_value,
                          int32_t *return_code,
                          int32_t *reason_code);

/* Type of the REQUEST argument to `ptrace`.  */
enum __ptrace_request
{
  PTRACE_TRACEME = 0,
#define PT_TRACE_ME PTRACE_TRACEME
  PTRACE_PEEKTEXT = 1,
#define PT_READ_I PTRACE_PEEKTEXT
  PTRACE_PEEKDATA = 2,
#define PT_READ_D PTRACE_PEEKDATA
  PTRACE_PEEKUSER = 3,
#define PT_READ_U PTRACE_PEEKUSER
  PTRACE_POKETEXT = 4,
#define PT_WRITE_I PTRACE_POKETEXT
  PTRACE_POKEDATA = 5,
#define PT_WRITE_D PTRACE_POKEDATA
  PTRACE_CONT = 7,
#define PT_CONTINUE PTRACE_CONT
  PTRACE_KILL = 8,
#define PT_KILL PTRACE_KILL
  PT_READ_GPR = 11,
  PT_READ_FPR = 12,
  PT_READ_VR = 13,
  PT_WRITE_GPR = 14,
  PT_WRITE_FPR = 15,
  PT_WRITE_VR = 16,
  PT_READ_BLOCK = 17,
  PT_WRITE_BLOCK = 19,
  PT_READ_GPRH = 20,
  PT_WRITE_GPRH = 21,
  PT_REGHSET = 22,
  PTRACE_ATTACH = 30,
#define PT_ATTACH PTRACE_ATTACH
  PTRACE_DETACH = 31,
#define PT_DETACH PTRACE_DETACH
  PT_REGSET = 32,
  PT_REATTACH = 33,
  PT_LDINFO = 34,
  PT_MULTI = 35,
  PT_BLOCKREQ = 40,
  PT_THREAD_INFO = 60,
  PT_THREAD_MODIFY = 61,
  PT_THREAD_READ_FOCUS = 62,
  PT_THREAD_WRITE_FOCUS = 63,
  PT_THREAD_HOLD = 64,
  PT_THREAD_SIGNAL = 65,
  PT_EXPLAIN = 66,
  PT_EVENTS = 67,
  PT_REATTACH2 = 71,
  PT_CAPTURE = 72,
  PT_UNCAPTURE = 73,
  PT_GET_THREAD_TCB = 74,
  PT_GET_ALET = 75,
  PT_SWAPIN = 76,
  PT_EXTENDED_EVENT = 98,
  PT_RECOVER = 99,
};

/* User area offsets.  */
#define PTUAREA_MINSIG	1
#define PTUAREA_MAXSIG	1024
#define PTUAREA_INTCODE	1025
#define PTUAREA_ABENDCC	1026
#define PTUAREA_ABENDRC	1027
#define PTUAREA_SIGCODE	1028
#define PTUAREA_ILC	1029
#define PTUAREA_PRFLAGS	1030

/* Maximum length that may be required by the requests that take a buffer. */
#define PTMAXIMUMLENGTH 64000

struct __ptrace_ldinfo
{
  uint32_t offset_to_next;
  uint32_t unused1;
  uint32_t text_origin;
  uint32_t text_size;
  uint8_t text_subpool;
  uint8_t text_flags;
#define PTLDTEXTWRITE	0x80
#define PTLDTEXTMVS	0x40
#define PTLDTEXTEXT	0x20
  uint16_t ext_offset;
  uint32_t unused2;
  uint32_t unused3;
  uint8_t unused4;
  uint8_t unused5;
  uint16_t reserved;
  char pathname[];
};

struct __ptrace_ldinfo_extents
{
  uint16_t count;
  uint16_t __reserved;
  uint32_t text_origin[15];
  uint32_t text_size[15];
};

struct __ptrace_thread_info
{
  uint32_t offset_to_next;
  uint8_t thread_id[8];
  uint8_t __reserved[16];
  uint32_t state;
#define PTRACE_TSTATE_ACTIVE		0x80000000
#define PTRACE_THRD_ST_ASYNC		0x40000000
#define PTRACE_THRD_ST_CANCELPEND 	0x20000000
  uint32_t kernel;
#define PTRACE_THRD_KERN_DETACHED	0x80000000
#define PTRACE_THRD_KERN_MEDIUM		0x40000000
#define PTRACE_THRD_KERN_ASYNC		0x20000000
#define PTRACE_THRD_KERN_PTCREATE	0x10000000
#define PTRACE_THRD_KERN_HOLD		0x00800000
  uint32_t exit_status_low;
  uint8_t pending_sigmask[8];
  uint32_t exit_status_high;
  uint32_t __reserved2;
};

struct __ptrace_thread_info_ext
{
  uint32_t offset_to_next;
  uint8_t thread_id[8];
  uint32_t tcb;
  uint32_t otcb;
  uint8_t blocked_sigmask[8];
  uint32_t state;
  uint32_t kernel;
  uint32_t exit_status_low;
  uint8_t pending_sigmask[8];
  uint32_t pid;
  uint16_t asid;
  uint16_t flags;
#define PTRACE_THRD_EXT_IPT		0x8000
#define PTRACE_THRD_EXT_INCOMPLETE	0x4000
  uint32_t oapb;
  uint32_t exit_status_high;
};

struct __ptrace_process_and_thread_info
{
  char id[4];
  uint32_t offset_to_next;
  uint32_t offset_to_thread_info;
  uint32_t pid;
  uint8_t pending_sigmask[8];
  uint8_t blocked_sigmask[8];
  uint32_t total_thread_count;
  uint32_t current_thread_count;
  uint32_t thread_info_size;
  uint32_t __reserved;
  struct __ptrace_thread_info_ext thrd_infos[];
};

struct __ptrace_explain_info
{
  uint32_t r1;
  uint32_t r12;
  uint32_t r13;
  uint32_t reserved1;
  uint64_t r1_64;
  uint64_t r12_64;
  uint64_t r13_64;
};

struct __ptrace_program_recovery_parameters
{
  uint32_t address_of_registers;
  uint32_t address_of_psw;
  uint16_t program_interrupt_code;
  uint16_t signal_number_to_raise;
  uint32_t flags;
#define PTPR_INSTRUCTION_COUNTER_HAS_BEEN_MODIFIED 0x80000000
#define PTPR_REGISTERS_HAVE_BEEN_MODIFIED 0x40000000
#define PTPR_RAISE_SIGNAL 0x20000000
#define PTPR_BYPASS_SIGNAL 0x10000000
#define PTPR_INSTRUCTION_LENGTH_CODE_EXISTS 0x08000000
#define PTPR_HIGH_REGISTERS_EXIST 0x04000000
#define PTPR_HIGH_REGISTERS_HAVE_BEEN_MODIFIED 0x02000000
#define PTPI_USE_64BITS_FOR_PSW_AND_REGISTERS 0x01000000
  uint8_t abend_code;
  uint8_t abend_completion_code[3];
  uint32_t abend_reason_code;
  uint8_t instruction_length_code;
  uint8_t reserved[3];
  uint32_t address_of_high_registers;
  uint64_t address_of_registers64;
  uint64_t address_of_psw64;
  uint64_t address_of_high_registers64;
  uint8_t reserved2[8];
};

/* PT_BLOCKREQ structures */
struct __ptrace_blockreq_elem
{
  int32_t type;
  int32_t status;
  uint32_t req_blk_off;
  uint8_t __reserved[4];
};

struct __ptrace_blockreq
{
  int32_t num_reqs;
  uint8_t __reserved[12];
  struct __ptrace_blockreq_elem requests[];
};

struct __ptrace_gpr_blockreq
{
  uint32_t write_regs;
#define PTBR_GPR_WGPR0	0x80000000
#define PTBR_GPR_WGPR1	0x40000000
#define PTBR_GPR_WGPR2	0x20000000
#define PTBR_GPR_WGPR3	0x10000000
#define PTBR_GPR_WGPR4	0x08000000
#define PTBR_GPR_WGPR5	0x04000000
#define PTBR_GPR_WGPR6	0x02000000
#define PTBR_GPR_WGPR7	0x01000000
#define PTBR_GPR_WGPR8	0x00800000
#define PTBR_GPR_WGPR9	0x00400000
#define PTBR_GPR_WGPR10	0x00200000
#define PTBR_GPR_WGPR11	0x00100000
#define PTBR_GPR_WGPR12	0x00080000
#define PTBR_GPR_WGPR13	0x00040000
#define PTBR_GPR_WGPR14	0x00020000
#define PTBR_GPR_WGPR15	0x00010000
#define PTBR_GPR_WPSW	0x00008000
  uint8_t __reserved[12];
  uint32_t gprs[16];
  uint32_t crs[16];
  uint32_t psw_mask;
  uint32_t psw_addr;
};

struct __ptrace_gprh_blockreq
{
  uint32_t write_regs;
  uint8_t __reserved[12];
  uint32_t gprs[16];
};

struct __ptrace_fpr_blockreq
{
  uint32_t write_regs;
#define PTBR_FPR_WFPR0	0x80000000
#define PTBR_FPR_WFPR1	0x40000000
#define PTBR_FPR_WFPR2	0x20000000
#define PTBR_FPR_WFPR3	0x10000000
#define PTBR_FPR_WFPR4	0x08000000
#define PTBR_FPR_WFPR5	0x04000000
#define PTBR_FPR_WFPR6	0x02000000
#define PTBR_FPR_WFPR7	0x01000000
#define PTBR_FPR_WFPR8	0x00800000
#define PTBR_FPR_WFPR9	0x00400000
#define PTBR_FPR_WFPR10	0x00200000
#define PTBR_FPR_WFPR11	0x00100000
#define PTBR_FPR_WFPR12	0x00080000
#define PTBR_FPR_WFPR13	0x00040000
#define PTBR_FPR_WFPR14	0x00020000
#define PTBR_FPR_WFPR15	0x00010000
#define PTBR_FPR_WFPC	0x00008000
  uint8_t __reserved[12];
  double fprs[16];
  uint32_t fpc;
};

struct __ptrace_block32_blockreq
{
  uint32_t address;
  uint32_t length;
  uint8_t __reserved[8];
  char data[];
};

struct __ptrace_block64_blockreq
{
  uint64_t address;
  uint32_t length;
  uint8_t __reserved[4];
  char data[];
};

struct __ptrace_data32_blockreq
{
  uint32_t address;
  uint32_t data;
};

struct __ptrace_data64_blockreq
{
  uint64_t address;
  uint32_t data;
};

struct __ptrace_read_u_field
{
  uint32_t offset;
  uint32_t data;
};

struct __ptrace_read_u_blockreq
{
  uint32_t count;
  uint32_t __reserved;
  struct __ptrace_read_u_field data[];
};


extern long int ptrace (enum __ptrace_request __request, ...);


#ifdef __cplusplus
}
#endif

#endif // ZOS_PTRACE_H_
