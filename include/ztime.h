#ifndef __Z_TIME__
  #define __Z_TIME__ 1

  #include <time.h>

  int pdjd_to_tm(const char* pdjd, int start_century, struct tm* ltime);
  void tm_to_pdjd(unsigned char* century, char* pdjd, struct tm* ltime);
  time_t tod_to_time(unsigned long long tod);
  unsigned int pd_to_d(unsigned char pd);
  unsigned char d_to_pd(unsigned int val, int set_positive_sign);
#endif
