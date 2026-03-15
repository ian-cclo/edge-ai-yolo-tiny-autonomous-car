/*******************************************************
 * Camera_2_Lcd.ino - Camera to TFT LCD
 *
 * Author: Kevin Chen
 * Date: 2023/11/16
 *
 * Version: 1.0.0
 *
 * This code was written by Kevin Chen.
 *
 * This is an open-source program, and it can be freely used, modified, and distributed under the following conditions:
 *
 * 1. The original copyright notice must not be removed or modified.
 * 2. Projects or products using this code must acknowledge the original author's name and the source in applicable documentation, websites, or other related materials.
 * 3. Any derivative work based on this code must state its origin and retain the original copyright notice in its documentation.
 * 4. This code must not be used for any activities that may infringe upon the rights of others, be unlawful, or harmful, whether in a commercial or non-commercial environment.
 *
 * This code is provided "as is" with no warranty, expressed or implied. The author is not liable for any losses or damages caused by using the code.
 *
 * Users of Camera_2_Lcd.ino assume all risks associated with its use, and the author shall not be held responsible for any consequences.
 *
 * For more information about our company, please visit: www.makdev.net
 *
 *  1.Based on AmebaILI9341.h, implement the functionality to display Camera output on TFT LCD and utilize the SD Card feature to achieve basic photo capture.
 *
 *  2.Before starting the project, please install the TJpg_Decoder library. In the library's configuration file, User_Config.h, comment out line 5 which reads: #define TJPGD_LOAD_SD_LIBRARY
 *******************************************************/

#include "WiFi.h"
#include "StreamIO.h"
#include "VideoStream.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "RTSP.h"
#include "SPI.h"
#include "AmebaST7789.h"
// Include the jpeg decoder library
#include "JPEGDEC.h"


// Default preset configurations for each video channel:
// Channel 0 : 1920 x 1080 30FPS H264
// Channel 1 : 1280 x 720  30FPS H264
// Channel 2 : 1280 x 720  30FPS MJPEG

// Default audio preset configurations:
// 0 :  8kHz Mono Analog Mic
// 1 : 16kHz Mono Analog Mic
// 2 :  8kHz Mono Digital PDM Mic
// 3 : 16kHz Mono Digital PDM Mic
#define CHANNEL 0
#define CHANNELJPEG 2

VideoSetting configV(CHANNEL);
VideoSetting configJPEG(VIDEO_VGA, CAM_NN_FPS, VIDEO_JPEG, 1);
AudioSetting configA(0);
Audio audio;
AAC aac;
RTSP rtsp;
StreamIO audioStreamer(1, 1);    // 1 Input Audio -> 1 Output AAC
StreamIO avMixStreamer(2, 1);    // 2 Input Video + Audio -> 1 Output RTSP

#define TFT_RESET AMB_D11
#define TFT_DC    AMB_D29
#define TFT_BL    AMB_D8
#define TFT_CS    SPI1_SS
#define ST7789_SPI_FREQUENCY 10000000

AmebaST7789 tft = AmebaST7789(TFT_CS, TFT_DC, TFT_RESET);

uint32_t img_addr = 0;
uint32_t img_len = 0;

JPEGDEC jpeg;

int JPEGDraw(JPEGDRAW *pDraw)
{
    tft.drawBitmap(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
    //Serial.println(millis());
    return 1;    // continue decode
} /* JPEGDraw() */

char ssid[] = "WIKA";    // your network SSID (name)
char pass[] = "91058769";        // your network password
int status = WL_IDLE_STATUS;


void printInfo(void)
{
    Serial.println("------------------------------");
    Serial.println("- Summary of Streaming -");
    Serial.println("------------------------------");
    Camera.printInfo();

    IPAddress ip = WiFi.localIP();

    Serial.println("- RTSP -");
    Serial.print("rtsp://");
    Serial.print(ip);
    Serial.print(":");
    rtsp.printInfo();

    Serial.println("- Audio -");
    audio.printInfo();
}

void setup()
{
    Serial.begin(115200);
    Serial.println("TFT ST7789 test Camera");

    // attempt to connect to Wifi network:
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network:
        status = WiFi.begin(ssid, pass);

        // wait 2 seconds for connection:
        delay(2000);
    }

    // Configure camera video channel with video format information
    // Adjust the bitrate based on your WiFi network quality
    // configV.setBitrate(2 * 1024 * 1024);     // Recommend to use 2Mbps for RTSP streaming to prevent network congestion
    Camera.configVideoChannel(CHANNEL, configV);
    Camera.configVideoChannel(CHANNELJPEG, configJPEG);
    Camera.videoInit();

    // Configure audio peripheral for audio data output
    audio.configAudio(configA);
    audio.begin();
    // Configure AAC audio encoder
    aac.configAudio(configA);
    aac.begin();

    // Configure RTSP with identical video format information and enable audio streaming
    rtsp.configVideo(configV);
    rtsp.configAudio(configA, CODEC_AAC);
    rtsp.begin();

    // Configure StreamIO object to stream data from audio channel to AAC encoder
    audioStreamer.registerInput(audio);
    audioStreamer.registerOutput(aac);
    if (audioStreamer.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }

    // Configure StreamIO object to stream data from video channel and AAC encoder to rtsp output
    avMixStreamer.registerInput1(Camera.getStream(CHANNEL));
    avMixStreamer.registerInput2(aac);
    avMixStreamer.registerOutput(rtsp);
    if (avMixStreamer.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }

    // Start data stream from video channel
    Camera.channelBegin(CHANNEL);
    Camera.channelBegin(CHANNELJPEG);

    delay(1000);
    printInfo();

    pinMode(AMB_D4, OUTPUT);   /* F_11 */
    pinMode(AMB_D5, OUTPUT);   /* F_12 */
    pinMode(AMB_D6, OUTPUT);   /* F_13 */
    pinMode(AMB_D7, OUTPUT);   /* F_14 */
    pinMode(AMB_D23, OUTPUT);   /* F_9 */
    pinMode(AMB_D24, OUTPUT);   /* E_6 */

    digitalWrite(AMB_D4, LOW);
    digitalWrite(AMB_D5, LOW);
    digitalWrite(AMB_D6, LOW);
    digitalWrite(AMB_D7, LOW);
    digitalWrite(AMB_D23, LOW);
    digitalWrite(AMB_D24, LOW);

    SPI1.setDefaultFrequency(ST7789_SPI_FREQUENCY);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    tft.begin();
    tft.setRotation(3);
}

void loop()
{
    Camera.getImage(CHANNELJPEG, &img_addr, &img_len);

    jpeg.openFLASH((uint8_t *)img_addr, img_len, JPEGDraw);
    jpeg.decode(0, 0, JPEG_SCALE_HALF);
    jpeg.close();

    tft.setFontSize(3);
    tft.println(WiFi.localIP());
}
