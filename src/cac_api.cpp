#include <Arduino.h>
#include <SD.h>
#include <HTTPClient.h>
#include "config.h"

char const *boundaryVals = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

String makeBoundaryStr() {
  String boundary = "";
  for (int i = 0; i < 20; i++) boundary += boundaryVals[random(62)];
  return boundary;
}


void registerDevice() {
  WiFiClient client;


  if (!client.connect(SERVER, PORT)) {
    Serial.println("http post connection failed");
    return;
  }


}


//https://github.com/adjavaherian/solar-server/blob/master/lib/Poster/Poster.cpp
void postRecording(File file, String metadata) {
  //WiFiClientSecure client;
  WiFiClient client;
  String boundary = makeBoundaryStr();
  String fileName = file.name();
  //String contentType = "image/jpeg";
  //String contentType = "audio/mpeg";
  //String contentType = "audio/x-wav";
  String contentType = "test/plain";



  if (!client.connect(SERVER, PORT)) {
    Serial.println("http post connection failed");
    return;
  }

  // post header
  String postHeader = "POST " + String(UPLOAD_PATH) + " HTTP/1.1\r\n";
  postHeader += "Host: " + String(SERVER) + ":" + PORT + "\r\n";
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