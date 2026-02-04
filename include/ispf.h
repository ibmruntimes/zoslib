#ifndef __ISPF__
  #define __ISPF__

  #include <time.h>

  #pragma pack(1)
  struct ispf_disk_stats {
    unsigned char ver_num;
    unsigned char mod_num;
    int sclm:1;
    int reserve_a:1;
    int extended:1;
    int reserve_b:5;
    unsigned char pd_mod_seconds;

    unsigned char create_century;
    char pd_create_julian[3];

    unsigned char mod_century;
    char pd_mod_julian[3];

    unsigned char pd_mod_hours;
    unsigned char pd_mod_minutes;
    unsigned short curr_num_lines;

    unsigned short init_num_lines;
    unsigned short mod_num_lines;

    char userid[8];

    /* following is available only in extended format */
    unsigned int full_curr_num_lines;
    unsigned int full_init_num_lines;
    unsigned int full_mod_num_lines;
  };
  #pragma pack(pop)

  struct ispf_stats {
    struct tm create_time;
    struct tm mod_time;
    unsigned int curr_num_lines;
    unsigned int init_num_lines;
    unsigned int mod_num_lines;
    unsigned char userid[8+1];
    unsigned char ver_num;
    unsigned char mod_num;
    unsigned char sclm;
  };

#endif
