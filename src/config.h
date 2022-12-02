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

#define WAV_RECORDING_FILE "/recording.wav"
#define AAC_RECORDING_FILE "/recording.aac"

#define MIN_HIBERNATION 40*60
#define MAX_HIBERNATION 60*60
#define RECORDING_DURATION 10000

#define RECORDINGS_DIR "/recordings"

#define NUM_RECORDINGS_WHEN_UPLOAD 10

#define SERVER "192.168.1.9"
#define PORT 8051
#define UPLOAD_PATH "/upload"