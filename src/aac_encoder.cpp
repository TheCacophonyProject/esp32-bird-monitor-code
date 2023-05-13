#include <Arduino.h>
//#include <sha1.h>
#include <SD.h>

#include "AACEncoderFDK.h"

using namespace aac_fdk;

File out;

void dataCallback(uint8_t *aac_data, size_t len) {
    out.write(aac_data, len);
    //Sha1.write(aac_data, len);
}

AACEncoderFDK aac(dataCallback);
AudioInfo info;
int16_t buffer[512];

String getHash(uint8_t* hash) {
    int i;
    String hashStr;
    for (i=0; i<20; i++) {
        hashStr += ("0123456789abcdef"[hash[i]>>4]);
        hashStr += ("0123456789abcdef"[hash[i]&0xf]);
    }
    return hashStr;
}

// Convert to m4a `ffmpeg -i in.aac -vn -acodec copy out.m4a`
String encodeToAAC(File in, File o) {
    //Sha1.init();
    out = o;
    in.seek(44); // Skip wav header 

    info.channels = 1;
    info.sample_rate = 48000;

    //aac(dataCallback);
    aac.setAfterburner(1);                 // Higher quality for more processing and ram usage.
    //aac.setAudioObjectType(2);            // Not too sure what the best type is.
    aac.setVariableBitrateMode(5);          // Set quality from low to High (1..5).
    aac.begin(info);

    Serial.println("writing...");
    unsigned long long total = 0;
    unsigned long long count = 0;
    unsigned long start = millis();
    while (in.available()) {
        size_t s = in.read((uint8_t *)buffer, 512*2);
        count += s;
        for (int i = 0; i < s/2; i++) {
            total += buffer[i];
            buffer[i] = buffer[i]-2700;
        }
        aac.write((uint8_t*)buffer, s);
    }
    unsigned long duration = millis() - start;
    Serial.println("processing took " + String(duration) + " milliseconds.");
    Serial.println(total);
    Serial.println(count);
    Serial.println(total/count);
    out.close();
    Serial.println("done");
    return "sha1";//getHash(Sha1.result());
}