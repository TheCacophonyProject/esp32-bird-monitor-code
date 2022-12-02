/*
Bird monitor for an ESP32. Currently a WIP.
*/

/* TODO
- Make regular recordings with metadata files (timestamp, GPS)
- Connect to API
- Use 4G modem
- Use GPS
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "FS.h"
#include <esp_sleep.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include "cac_api.h"
#include "recorder.h"
#include "aac_encoder.h"
#include "config.h"
#include "rtc.h"
#include "util.h"

const char* ssid = "SSID";
const char* password = "password";
RTC rtc;

/*
  Decide if recordings should be uploaded now.
*/
bool shouldUploadNow() {
  File dir = SD.open(RECORDINGS_DIR);
  int recordingCount = 0;
  while (true) {
    File f = dir.openNextFile();
    if (!f) {
      break;
    }
    recordingCount++;
  }

  Serial.println("There are " + String(recordingCount) + " recordings on the device.");

  return NUM_RECORDINGS_WHEN_UPLOAD <= recordingCount;
}

#define PWM1_Ch 0

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

/*
  hibernate - Will go into low power mode. When waking up the program will start from the beginning.
*/
void hibernateUntilNextRecording() {
  beep(6);
  uint64_t seconds = random(MIN_HIBERNATION, MAX_HIBERNATION);
  Serial.print("Hibernating for ");
  Serial.print(seconds);
  Serial.println(" seconds.");
  pinMode(SD_ENABLE, OUTPUT);
  digitalWrite(SD_ENABLE, HIGH);
  esp_sleep_enable_timer_wakeup(seconds * 1000000);
  Serial.flush();
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n====================");
  Serial.println("Setup");
  delay(500);
  ledcAttachPin(4, PWM1_Ch);
  ledcSetup(PWM1_Ch, 2700, 8);
  rtc.init();
  Serial.println(esp_random());
  randomSeed(esp_random());

  //hibernateUntilNextRecording();

  //======= LOAD SD CARD ================
  Serial.println("Loading SD card...  ");
  pinMode(SD_ENABLE, OUTPUT);
  digitalWrite(SD_ENABLE, LOW);
  delay(100);
  if (!SD.begin(SD_CS)) {
    Serial.println("Initialization of SD failed!");
    while (true) {}
  }
  Serial.println("Done.");

  beep(2);
  //======= MAKE RECORDING ==============
  Serial.println("Make audio recording...  ");
  File wav = SD.open(WAV_RECORDING_FILE, FILE_WRITE);
  makeRecording(wav, RECORDING_DURATION);
  Serial.println("Finished making recording.");

  beep(3);
  //======= ENCODE RECORDING TO AAC =====
  Serial.println("Encode recording to AAC...  ");
  wav = SD.open(WAV_RECORDING_FILE, FILE_READ);
  if (!SD.exists(RECORDINGS_DIR)) {
    SD.mkdir(RECORDINGS_DIR);
  }
  String audioDir = RECORDINGS_DIR + String("/") + randStr(20);
  SD.mkdir(audioDir);
  Serial.println("Made audio dir " + audioDir);
  String aacFileName = audioDir + AAC_RECORDING_FILE;
  File acc = SD.open(aacFileName, FILE_WRITE);
  Serial.println("Saving audio to " + aacFileName);
  encodeToAAC(wav, acc);
  Serial.println("Done.");

  beep(4);
  //======= CHECK IF SLEEP OR UPLOAD ====
  if (!shouldUploadNow()) hibernateUntilNextRecording();


  //======= Connect to WiFi ==============
  Serial.println("Connecting to WiFi...  ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Done");


  //======= Upload recordings ============
  Serial.println("Upload recording...");
  File dir = SD.open(RECORDINGS_DIR);
  while (true) {
    File f = dir.openNextFile();
    if (!f) break;
    File accFile = SD.open(String(f.name())+AAC_RECORDING_FILE, FILE_READ);
    File dataFile = SD.open(String(f.name())+AAC_RECORDING_FILE, FILE_READ);
    String data = dataFile.readStringUntil('\n');
    postRecording(accFile, data);
    SD.remove(f.name());
  }

  Serial.println("Done.");


  hibernateUntilNextRecording();
}

void loop(){}