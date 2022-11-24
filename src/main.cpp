/*
Bird monitor for an ESP32. Currently a WIP.
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <esp_sleep.h>

#include "FS.h"

#include <driver/i2s.h>
#include "I2SSampler.h"
#include "I2SMEMSSampler.h"
#include "config.h"

#define SAMPLE_RATE 48000

//http://www.topherlee.com/software/pcm-tut-wavformat.html
typedef struct WavHeader {
  char riffHeader[4] = {'R', 'I', 'F', 'F'};
  int wavSize = 0;
  char wavHeader[4] = {'W', 'A', 'V', 'E'};
  char format[4] = {'f', 'm', 't', ' '};
  int formatLen = 16;
  uint16_t formatType = 1;
  uint16_t channels = 1;
  int sampleRate = SAMPLE_RATE;
  int byteRate = SAMPLE_RATE * 2;
  uint16_t bytesSamples = 2;
  uint16_t bitsPerSample = 16;
  char dataHeader[4] = {'d', 'a', 't', 'a'};
  int dataSize = 0;
};


// Digital I/O used
//V0.2
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18
#define SD_ENABLE     17
#define I2S_DOUT      33
#define I2S_BCLK      25
#define I2S_LRC       26
#define STATUS_LED    16

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n====================");
  Serial.println("Setup");
  delay(500);

  WavHeader h;
  

  


  //Serial.println();
 // for (int i = 0; i < sizeof(wav_header_t); i++) {
  //  Serial.println((uint8_t)&h+i);
 // }


  //======================================
  Serial.println("Init SD card");
  pinMode(SD_ENABLE, OUTPUT);
  digitalWrite(SD_ENABLE, LOW);
  delay(100);
  if (!SD.begin(SD_CS)) {
    Serial.println("Initialization of SD failed!");
  }
  File f = SD.open("/foo.txt", FILE_APPEND);
  f.println("bar");
  f.close();
  Serial.println("Finished writing to SD card test");

  //=======================================
  I2SSampler *input = new I2SMEMSSampler(I2S_NUM_0, i2s_mic_pins, i2s_mic_Config);
  int16_t *samples = (int16_t *)malloc(sizeof(int16_t) * 1024);
  Serial.println("Start recording");
  input->start();
  File r = SD.open("/recording.wav", FILE_WRITE);
  
  // Writing WAV header
  r.write((uint8_t *)&h, sizeof(WavHeader));
  int m_file_size = sizeof(WavHeader);

  // Turning on Microphone and record for a couple seconds before starting official recording
  Serial.print("turning on microphone...  ");
  unsigned long start = millis();
  while (millis() < start + 2000) {
    input->read(samples, 1024);
  }
  Serial.println("done.");

  // Start recording
  Serial.print("recording for 5 seconds...  ");
  start = millis();
  while (millis() < start+5000) {
    int samples_read = input->read(samples, 1024);
    r.write((uint8_t *)samples, samples_read*2);
    m_file_size += sizeof(int16_t) * samples_read;
  }

  // Stop recording
  input->stop();
  Serial.println("done.");

  h.wavSize = m_file_size - 8;
  h.dataSize = m_file_size - sizeof(WavHeader);

  /* Write out heaer file
  Serial.println("WavHeader");
  uint8_t* structPtr = (uint8_t*) &h;
  for (byte i = 0; i < sizeof(WavHeader); i++)  {
    Serial.print(*structPtr++, HEX);
    Serial.print(", ");
  }
  Serial.println();
  */
  r.seek(0);
  r.write((uint8_t *)&h, sizeof(WavHeader));
  
  // Finish 
  r.close();
  free(samples);
  Serial.println("Finished recording");
}

void loop(){
  Serial.println("main loop, nothign to do here yet..");
  delay(10000);
}