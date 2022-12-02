#include "rtc.h"
#include "config.h"
#include <EEPROM.h>
#include "utcTime.h"

#include <RTClib.h>     // https://github.com/adafruit/RTClib


TwoWire tw = TwoWire(0);

void RTC::setup() {}

void RTC::init() {
  delay(100);
  Serial.println("Running RTC init...  ");  
  tw.setPins(22, 21);
  if (!rtc.begin(&tw)) {
    Serial.println("Couldn't find RTC");
    return; // TODO STATUS_CODE_RTC_NOT_FOUND;
  } else if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running.");
    return; // TODO STATUS_CODE_RTC_TIME_NOT_SET;
  }

  DateTime uploadDateTime = DateTime(COMPILE_UTC_TIME);
  Serial.println("Software updated on (UTC)" + uploadDateTime.timestamp());
  
  EEPROM.begin(512);
  if (EEPROM.readString(0) != uploadDateTime.timestamp()) {
    Serial.println("New compile time. Writing to RTC and saving to EEPROM");
    rtc.adjust(uploadDateTime);
    EEPROM.writeString(0, uploadDateTime.timestamp());
    EEPROM.commit();
  } else if (rtc.now().unixtime() < uploadDateTime.unixtime()) {
    Serial.println("### Invalid RTC time. Time is before compile time, making it invalid. "+rtc.now().timestamp());
    return; // TODO STATUS_CODE_INVALID_RTC_TIME;
  } else if (rtc.now().year() > 2050) {
    Serial.println("### Invalid RTC time. Time is past 2050, should be done by now right? "+rtc.now().timestamp());
    return; // TODO STATUS_CODE_INVALID_RTC_TIME
  }

  Serial.println("RTC time (UTC): "+rtc.now().timestamp());
}

void printMIn24(int m) {
  int h = m/60;
  m = m - 60*h;
  if (h<10) {
    Serial.print("0");
  }
  Serial.print(h);
  Serial.print(":");
  if (m<10) {
    Serial.print("0");
  }
  Serial.println(m);
}

