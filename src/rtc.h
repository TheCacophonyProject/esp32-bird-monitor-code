#ifndef RTC_h_
#define RTC_h_

#include <RTClib.h>
#include <Wire.h>

class RTC {
    public:
        void setup();
        void init();
        //RTC_PCF8523 rtc;
        RTC_PCF8563 rtc;
    private:
      boolean dateTimeMatchEEPROMDateTime();
      void printDateTime(DateTime);
};

#endif