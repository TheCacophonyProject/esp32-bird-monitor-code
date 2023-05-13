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
#include <Arduino_JSON.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include "cac_api.h"
#include "recorder.h"
#include "aac_encoder.h"
#include "config.h"
#include "rtc.h"
#include "util.h"

const char* ssid = "bushnet";
const char* password = "feathers";
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



/*
  hibernate - Will go into low power mode. When waking up the program will start from the beginning.
*/
void hibernateUntilNextRecording() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
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
  JSONVar dataObject;
  dataObject["type"] = "audio";
  dataObject["duration"] = RECORDING_DURATION/1000;
  dataObject["recordingDateTime"] = rtc.rtc.now().timestamp();
  //dataObject["location"] = "("+String(LONGITUDE) +","+String(LATITUDE) +")";
  //dataObject["version"] = VERSION;
  //dataObject["filehash"] = "";            //TODO add sha1 hash of file 
  //dataObject["batteryCharging"] = false;  //TODO Wait for hardware support for charging.
  //dataObject["batteryLevel"] = "";        //TODO Wait for hardware support for measuring battery level.
  //dataObject["additionalMetadata"] = "";  //TODO Check if anything should be put in here
  //dataObject["comment"] = "";             //TODO
  //dataObject["processingState"] = "";     //TOOD
  //dataObject["hardware"] = "";            //TODO Check if hardware should be added to API

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
  String hash = encodeToAAC(wav, acc);
  //dataObject["filehash"] = encodeToAAC(wav, acc);
  Serial.println("Done.");

  String jsonStr = JSON.stringify(dataObject);
  Serial.println(jsonStr);
  File dataFile = SD.open(audioDir + DATA_FILE, FILE_WRITE);
  dataFile.print(jsonStr);
  dataFile.close();

  beep(4);
  //======= CHECK IF SLEEP OR UPLOAD ====
  if (!shouldUploadNow()) hibernateUntilNextRecording();


  //======= Connect to WiFi ==============
  Serial.println("Connecting to WiFi...  ");
  WiFi.begin(ssid, password);
  bool wifiConnected = false;
  for (int i = 0; i<20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      break;
    }
    delay(500);
  }
  if (!wifiConnected) {
    WiFi.disconnect();
    Serial.println("Could not connect to WiFi, sleeping until next recording.");
    hibernateUntilNextRecording();
  }
  Serial.println("Connected to WiFi.");


  //======= Upload recordings ============
  Serial.println("Upload recording...");
  File dir = SD.open(RECORDINGS_DIR);
  while (true) {
    File f = dir.openNextFile();
    if (!f) break;
    File accFile = SD.open(String(f.name())+AAC_RECORDING_FILE, FILE_READ);
    //File dataFile = SD.open(String(f.name())+AAC_RECORDING_FILE, FILE_READ);
    String data = "{'foo':'bar'}"; //dataFile.readStringUntil('\n');
    if (postRecording(accFile, data)) {
      Serial.println("Upload success, deleting recording.");
      deleteDir(f.name());
      //SD.rmdir(f.name());
    }
  }

  Serial.println("Done.");


  hibernateUntilNextRecording();
}

void loop(){}