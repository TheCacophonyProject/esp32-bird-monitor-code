#include <Arduino.h>
#include <SD.h>

#include "config.h"


char const *strs = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
String randStr(int length){
    String randStr = "";
    for (int i = 0; i < 20; i++) randStr += strs[random(62)];
    return randStr;
}

void deleteDir(String dir) {
    if (!SD.exists(dir)) {
        Serial.println(dir + " does not exist.");
        return;
    }

    File d = SD.open(dir);
    if (!d.isDirectory()) {
        Serial.println(dir + " is not a directory.");
        return;
    }

    while (true) {
        File f = d.openNextFile();
        if (!f) break;
        if (f.isDirectory()) {
            deleteDir(f.name());
        } else {
            SD.remove(f.name());
        }
    }

    SD.rmdir(dir);
    if (SD.exists(dir)) {
        Serial.println("Failed to delete directory " + dir);
    }
}

void buzzerOn() {
  ledcWrite(PWM1_Ch, 125);
}

void buzzerOff() {
  ledcWrite(PWM1_Ch, 0);
}

void beep(int beeps) {
  for (int i = 0; i < beeps; i++) {
    buzzerOn();
    delay(200);
    buzzerOff();
    delay(200);
  }
}