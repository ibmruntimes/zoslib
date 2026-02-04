#ifndef __IHAPDS_H__
#define __IHAPDS_H__

/* Standalone PDS directory entry structures - no external dependencies */

#pragma pack(1)

struct pds2 {
  unsigned char  pds2name[8];             /* MEMBER NAME OR ALIAS NAME                 */
  unsigned char  pds2ttrp[3];             /* TTR OF FIRST BLOCK OF NAMED MEMBER        */
  char           pds2cnct;                /* CONCATENATION NUMBER OF THE DATA SET      */
  unsigned char  pds2libf;                /* LIBRARY FLAG FIELD                        */
  unsigned char  pds2indc;                /* INDICATOR BYTE                            */
  union {
    unsigned char  pds2usrd;    /* START OF VARIABLE LENGTH USER DATA FIELD */
    unsigned char  pds2ttrt[3]; /* TTR OF FIRST BLOCK OF TEXT               */
    };
  unsigned char  pds2zero;                /* ZERO                                      */
  unsigned char  pds2ttrn[3];             /* TTR OF NOTE LIST OR SCATTER/TRANSLATION   */
  char           pds2nl;                  /* NUMBER OF ENTRIES IN NOTE LIST FOR        */
  union {
    unsigned char  pds2atr[2]; /* TWO-BYTE PROGRAM ATTRIBUTE FIELD */
    struct {
      unsigned char  pds2atr1;     /* FIRST BYTE OF PROGRAM ATTRIBUTE FIELD    */
      unsigned int   pds2flvl : 1, /* If one, the program cannot be processed  */
                     pds2org0 : 1, /* ORIGIN OF FIRST BLOCK OF TEXT IS ZERO    */
                     pds2ep0  : 1, /* ENTRY POINT IS ZERO                      */
                     pds2nrld : 1, /* PROGRAM CONTAINS NO RLD ITEMS            */
                     pds2nrep : 1, /* PROGRAM CANNOT BE REPROCESSED BY LINKAGE */
                     pds2tstn : 1, /* PROGRAM CONTAINS TESTRAN SYMBOL CARDS    */
                     pds2lef  : 1, /* PROGRAM CREATED BY LINKAGE EDITOR F      */
                     pds2refr : 1; /* REFRESHABLE PROGRAM                      */
      };
    };
  int            pds2stor : 24;           /* TOTAL CONTIGUOUS MAIN STORAGE REQUIREMENT */
  short int      pds2ftbl;                /* LENGTH OF FIRST BLOCK OF TEXT             */
  unsigned int   pds2epa : 24;            /* ENTRY POINT ADDRESS ASSOCIATED WITH       */
  union {
    unsigned char  pds2ftbo[3]; /* FLAG BYTES (MVS USE OF FIELD)        @LCC */
    struct {
      unsigned char  pds2ftb1;     /* BYTE 1 OF PDS2FTBO                        */
      unsigned int   pds2altp : 1, /* ALTERNATE PRIMARY FLAG. IF ON (FOR A @L8A */
                              : 1,
                     pdslrm64 : 1, /* 0: RMODE is 24 or 31.                @LDA */
                     pdslrmod : 1, /* 0: RMODE 24 or reserved              @LDA */
                     pdsaamod : 2, /* ALIAS ENTRY POINT ADDRESSING MODE    @L6A */
                     pdsmamod : 2; /* MAIN ENTRY POINT ADDRESSING MODE     @L6A */
      struct {
        unsigned int   pds2nmig : 1, /* THIS PROGRAM OBJECT CANNOT BE CONVERTED   */
                       pds2prim : 1, /* FETCHOPT PRIME WAS SPECIFIED         @L7A */
                       pds2pack : 1, /* FETCHOPT PACK WAS SPECIFIED          @L7A */
                                : 5;
        } pds2rlds;                  /* NUMBER OF RLD/CONTROL RECORDS WHICH  @L6A */
      };
    };
  short int      pds2slsz;                /* NUMBER OF BYTES IN SCATTER LIST           */
  short int      pds2ttsz;                /* NUMBER OF BYTES IN TRANSLATION TABLE      */
  unsigned char  pds2esdt[2];             /* IDENTIFICATION OF ESD ITEM (ESDID) OF     */
  unsigned char  pds2esdc[2];             /* IDENTIFICATION OF ESD ITEM (ESDID) OF     */
  unsigned int   pds2epm : 24;            /* ENTRY POINT FOR MEMBER NAME               */
  unsigned char  pds2mnm[8];              /* MEMBER NAME OF PROGRAM. WHEN THE          */
  union {
    short int      pdss03;      /* FORCE HALF-WORD ALIGNMENT FOR SSI */
    unsigned char  pdsssiwd[4]; /* SSI INFORMATION WORD              */
    struct {
      char           pdschlvl;    /* CHANGE LEVEL OF MEMBER               */
      unsigned char  pdsssifb;    /* SSI FLAG BYTE                        */
      unsigned char  pdsmbrsn[2]; /* MEMBER SERIAL NUMBER                 */
      struct {
        char           pdsapfct; /* LENGTH OF PROGRAM AUTHORIZATION CODE */
        unsigned char  pdsapfac; /* PROGRAM AUTHORIZATION CODE           */
        } pdsapf;                /* PROGRAM AUTHORIZATION FACILITY (APF) */
      };
    };
  union {
    char           pds2lpol; /* LARGE PROGRAM OBJECT SECTION LENGTH  @L7A */
    char           pds2llml; /* ALTERNATE NAME FOR PDS2LLML          @L7A */
    };
  int            pds2vstr;                /* VIRTUAL STORAGE REQUIREMENT FOR THIS      */
  int            pds2mepa;                /* MAIN ENTRY POINT OFFSET                   */
  int            pds2aepa;                /* ALIAS ENTRY POINT OFFSET. ONLY VALID      */
  unsigned int                       : 4,
                 pds2xattr_optn_mask : 4; /* Bits 4-7 of PDS2XATTRBYTE0 identify the   */
  unsigned int   pds2longparm        : 1, /* PARM > 100 chars allowed             @LBA */
                                     : 7;
  char           _filler1;                /* Reserved                             @LBA */
  };

/* Values for field "pds2libf" */
#define pds2lnrm 0x00 /* NORMAL CASE                               */
#define pds2llnk 0x01 /* IF DCB OPERAND IN BLDL MACRO INTRUCTION   */
#define pds2ljob 0x02 /* IF DCB OPERAND IN BLDL MACRO INTRUCTION   */

/* Values for field "pds2indc" */
#define pds2alis 0x80 /* NAME IN THE FIELD PDS2NAME IS AN ALIAS    */
#define dealias  0x80 /* --- ALIAS FOR PDS2ALIS                    */
#define pds2nttr 0x60 /* NUMBER OF TTR'S IN THE USER DATA FIELD    */
#define pds2lusr 0x1F /* - LENGTH OF USER DATA FIELD               */

/* Values for field "pds2atr1" */
#define pds2rent 0x80 /* REENTERABLE                               */
#define dereen   0x80 /* --- ALIAS FOR PDS2RENT                    */
#define pds2reus 0x40 /* REUSABLE                                  */
#define pds2ovly 0x20 /* IN OVERLAY STRUCTURE                      */
#define deovly   0x20 /* --- ALIAS FOR PDS2OVLY                    */
#define pds2test 0x10 /* PROGRAM TO BE TESTED - TESTRAN            */
#define pds2load 0x08 /* ONLY LOADABLE                             */
#define delody   0x08 /* --- ALIAS FOR PDS2LOAD                    */
#define pds2sctr 0x04 /* SCATTER FORMAT                            */
#define descat   0x04 /* --- ALIAS FOR PDS2SCTR                    */
#define pds2exec 0x02 /* EXECUTABLE                                */
#define dexcut   0x02 /* --- ALIAS FOR PDS2EXEC                    */
#define pds21blk 0x01 /* IF ZERO, PROGRAM CONTAINS MULTIPLE        */

/* Values for field "pds2ftb1" */
#define pdsaosle 0x80 /* Program has been processed by OS/VS1 or   */
#define pds2big  0x40 /* THE LARGE PROGRAM OBJECT EXTENSION        */
#define pds2paga 0x20 /* PAGE ALIGNMENT REQUIRED FOR PROGRAM       */
#define pds2ssi  0x10 /* SSI INFORMATION PRESENT                   */
#define pdsapflg 0x08 /* INFORMATION IN PDSAPF IS VALID            */
#define pds2pgmo 0x04 /* PROGRAM OBJECT. THE PDS2FTB3              */
#define pds2lfmt 0x04 /* ALTERNATE NAME FOR PDS2PGMO          @L7A */
#define pds2sign 0x02 /* PROGRAM OBJECT IS SIGNED. VERIFIED ON     */
#define pds2xatr 0x01 /* PDS2XATTR SECTION                    @LBA */

/* Values for field "pdsssifb" */
#define pdsforce 0x40 /* A FORCE CONTROL CARD WAS USED WHEN        */
#define pdsusrch 0x20 /* A CHANGE WAS MADE TO MEMBER BY THE        */
#define pdsemfix 0x10 /* SET WHEN AN EMERGENCY IBM-AUTHORIZED      */
#define pdsdepch 0x08 /* A CHANGE MADE TO THE MEMBER IS DEPENDENT  */
#define pdssysgn 0x06 /* FLAGS THAT INDICATE WHETHER A             */
#define pdsnosgn 0x00 /* NOT CRITICAL FOR SYSTEM GENERATION        */
#define pdscmsgn 0x02 /* MAY REQUIRE COMPLETE REGENERATION         */
#define pdsptsgn 0x04 /* MAY REQUIRE PARTIAL REGENERATION          */
#define pdsibmmb 0x01 /* MEMBER IS SUPPLIED BY IBM                 */

#define bit0     128
#define bit1     64
#define bit2     32
#define bit3     16
#define bit4     8
#define bit5     4
#define bit6     2
#define bit7     1
#define dezbyte  0x0C /* --- ALIAS                                 */
#define pdsbcend 0x23 /* END OF BASIC SECTION                      */
#define pdsbcln  0x23 /* - LENGTH OF BASIC SECTION                 */
#define pdss01   0x23 /* START OF SCATTER LOAD SECTION             */
#define pdss01nd 0x2B /* END OF SCATTER LOAD SECTION               */
#define pdss01ln 0x08 /* - LENGTH OF SCATTER LOAD SECTION          */
#define pdss02   0x2B /* START OF ALIAS SECTION                    */
#define deentbk  0x2B /* --- ALIAS                                 */
#define pdss02nd 0x36 /* END OF ALIAS SECTION                      */
#define pdss02ln 0x0B /* - LENGTH OF ALIAS SECTION                 */
#define pdss03nd 0x3A /* END OF SSI SECTION                        */
#define pdss03ln 0x04 /* LENGTH OF SSI SECTION                     */
#define pdss04   0x3A /* START OF APF SECTION                      */
#define pdss04nd 0x3C /* END OF APF SECTION                        */
#define pdss04ln 0x02 /* LENGTH OF APF SECTION                     */
#define pdslpo   0x3C /* START OF LARGE PROGRAM OBJECT SECTION@L7A */
#define pdsllm   0x3C /* ALTERNATE NAME FOR PDSLPO            @L7A */
#define pdslpond 0x49 /* END OF LARGE PROGRAM OBJECT SECTION       */
#define pdsllmnd 0x49 /* ALTERNATE NAME FOR PDSLPOND               */
#define pdslpoln 0x0D /* LENGTH OF LLM SECTION              @L7A   */
#define pdsllmln 0x0D /* ALTERNATE NAME FOR PDSLPOLN          @L7A */
#define pds2xattr 0x49 /* Start of extended attributes         @LBA */
#define pds2xattr_opt 0x4C /* Start of optional fields. Number of       */

#pragma pack(pop)

#endif // __IHAPDS_H__
