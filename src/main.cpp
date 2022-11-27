#include "UDHttp.h"

/*
Bird monitor for an ESP32. Currently a WIP.
*/

/* TODO
- Make regular recordings with metadata files (timestamp, GPS)
- Compress to MP3
- Connect to API
- Use 4G modem
- Use GPS
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


#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "HUAWEI-G7K8QX";
const char* password = "55501994";


#define SAMPLE_RATE 48000

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

char const *boundaryVals = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

String makeBoundaryStr() {
  String boundary = "";
  for (int i = 0; i < 20; i++) boundary += boundaryVals[random(62)];
  return boundary;
}


//https://github.com/adjavaherian/solar-server/blob/master/lib/Poster/Poster.cpp
void post(String host, int port, String path, File file, String metadata) {
  //WiFiClientSecure client;
  WiFiClient client;
  String boundary = makeBoundaryStr();
  String fileName = file.name();
  //String contentType = "image/jpeg";
  //String contentType = "audio/mpeg";
  //String contentType = "audio/x-wav";
  String contentType = "test/plain";
  
  
  
  char hostChar[50];
  host.toCharArray(hostChar, host.length()+1);

  if (!client.connect(hostChar, port)) {
    Serial.println("http post connection failed");
    return;
  }

  // post header
  String postHeader = "POST " + path + " HTTP/1.1\r\n";
  postHeader += "Host: " + host + ":" + port + "\r\n";
  postHeader += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
  postHeader += "Accept: */*\r\n";
  postHeader += "Accept-Encoding: gzip,deflate\r\n";
  postHeader += "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n";
  postHeader += "Keep-Alive: 300\r\n";
  postHeader += "Connection: keep-alive\r\n";
  postHeader += "Accept-Language: en-us\r\n";

  // key header
  //String keyHeader = "";
  String keyHeader = "--" + boundary + "\r\n";
  keyHeader += "Content-Disposition: form-data; name=\"data\"\r\n\r\n";
  //keyHeader += "${filename}\r\n";
  keyHeader += metadata+"\r\n";

  // request header
  String requestHead = "--" + boundary + "\r\n";
  requestHead += "Content-Disposition: form-data; name=\"file\"; filename=\"" + fileName + "\"\r\n";
  requestHead += "Content-Type: " + contentType + "\r\n\r\n";

  // request tail
  String tail = "\r\n--" + boundary + "--\r\n\r\n";

  // content length
  int contentLength = keyHeader.length() + requestHead.length() + file.size() + tail.length();
  postHeader += "Content-Length: " + String(contentLength, DEC) + "\n\n";


  // send post header
  char charBuf0[postHeader.length() + 1];
  postHeader.toCharArray(charBuf0, postHeader.length() + 1);
  client.write("a");
  client.write(charBuf0);
  Serial.print(charBuf0);

  // send key header
  char charBufKey[keyHeader.length() + 1];
  keyHeader.toCharArray(charBufKey, keyHeader.length() + 1);
  client.write(charBufKey);
  Serial.print(charBufKey);

  // send request buffer
  char charBuf1[requestHead.length() + 1];
  requestHead.toCharArray(charBuf1, requestHead.length() + 1);
  client.write(charBuf1);
  Serial.print(charBuf1);

  // create buffer
  const int bufSize = 2048;
  byte clientBuf[bufSize];
  int clientCount = 0;

  while (file.available()) {
    clientBuf[clientCount] = file.read();
    clientCount++;
    if (clientCount > (bufSize - 1)) {
      client.write((const uint8_t *)clientBuf, bufSize);
      clientCount = 0;
    }
  }

  if (clientCount > 0) {
    client.write((const uint8_t *)clientBuf, clientCount);
    Serial.println("Sent LAST buffer");
  }

  // send tail
  char charBuf3[tail.length() + 1];
  tail.toCharArray(charBuf3, tail.length() + 1);
  client.write(charBuf3);
  Serial.print(charBuf3);

  // Read all the lines of the reply from server and print them to Serial
  Serial.println("request sent");
  String responseHeaders = "";

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
    responseHeaders += line;
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }

  String line = client.readStringUntil('\n');

  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");

  // close the file:
  file.close();

}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n====================");
  Serial.println("Setup");
  delay(500);

  WavHeader h;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");


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

  File file = SD.open("/foo.txt", FILE_READ);  
  post("192.168.1.9", 8051, "/upload", file, "{'foo': 'bar'}");

  return;

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
  while (millis() < start+60000) {
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