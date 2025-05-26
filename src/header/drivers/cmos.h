#ifndef CMOS_H
#define CMOS_H

#include "cpu/portio.h"

#define CURRENT_YEAR        2025                          // Change this each year!

extern int           century_register;
extern unsigned char second, minute, hour, day, month, century;
extern unsigned int  year;

enum {
      cmos_address = 0x70,
      cmos_data    = 0x71
};

int           get_update_in_progress_flag(void);
unsigned char get_RTC_register(int reg);
void          read_rtc(void);
void read_rtc();

#endif
