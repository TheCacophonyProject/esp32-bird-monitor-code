#include <Arduino.h>
#include <driver/i2s.h>
#include "I2SSampler.h"
#include "I2SMEMSSampler.h"
#include "recorder.h"
#include <SD.h>

#define SAMPLE_RATE 48000

// i2s config for reading from I2S
i2s_config_t i2s_mic_Config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

// i2s microphone pins
i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};

//http://www.topherlee.com/software/pcm-tut-wavformat.html
struct WavHeader {
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


void makeRecording(File r, int duration) {
  I2SSampler *input = new I2SMEMSSampler(I2S_NUM_0, i2s_mic_pins, i2s_mic_Config, true);
  int16_t *samples = (int16_t *)malloc(sizeof(int16_t) * 1024);
  Serial.println("Start recording");
  input->start();
  //File r = SD.open("/recording.wav", FILE_WRITE);
  
  // Writing WAV header
  WavHeader h;
  r.write((uint8_t *)&h, sizeof(WavHeader));
  int m_file_size = sizeof(WavHeader);

  // Turning on Microphone and record for a couple seconds before starting official recording
  Serial.println("turning on microphone...  ");
  unsigned long start = millis();
  while (millis() < start + 5000) {
    input->read(samples, 1024);
  }
  //Serial.println("done.");

  // Start recording
  Serial.println("recording for " + String(duration/1000) + " seconds...  ");
  start = millis();
  while (millis() < start+duration) {
    int samples_read = input->read(samples, 1024);
    r.write((uint8_t *)samples, samples_read*2);
    m_file_size += sizeof(int16_t) * samples_read;
  }

  // Stop recording
  input->stop();
  Serial.println("done.");

  h.wavSize = m_file_size - 8;
  h.dataSize = m_file_size - sizeof(WavHeader);

  r.seek(0);
  r.write((uint8_t *)&h, sizeof(WavHeader));
  
  // Finish 
  r.close();
  free(samples);
  Serial.println("Finished recording");
}