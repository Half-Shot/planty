/*
  This application sends a UDP packet to a given host every
  interval, containing data from different sensors.

  Written by Half-Shot <will@half-shot.uk> 2020
*/

#define LOWPOWER 1
/* #define SERIALDEBUG 1 */

#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include "arduino_secrets.h" 

#ifdef LOWPOWER
#include "ArduinoLowPower.h"
#endif



char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
IPAddress hostIp (192, 168, 0, 48);
unsigned int hostPort = 5000;
int sleepInterval = 1000 * 60 * 15; // 5 minutes

int status = WL_DISCONNECTED;     // the WiFi radio's status
byte buffer[10]; // one byte for checksum, one byte for temperature, one byte for moisutre, one byte for photo, two for connection attempts, 4 for txn number
uint16_t wifiAttempts = 0;
uint32_t txnNumber = 0;
WiFiUDP Udp;

void setup() {
  // Initialize serial and wait for port to open:
  #ifdef SERIALDEBUG
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  #endif

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    #ifdef SERIALDEBUG
    Serial.println("Communication with WiFi module failed!");
    #endif
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    #ifdef SERIALDEBUG
    Serial.println("Please upgrade the firmware");
    #endif
  }
}

bool connectWifi() {
  // Connect to WPA/WPA2 network:
  while (status != WL_CONNECTED) {
    if (status == WL_IDLE_STATUS) {
      status = WiFi.status();
    } else {
      status = WiFi.begin(ssid, pass);
    }
    wifiAttempts++;
    if (wifiAttempts % 8 == 0) {
      // Power down to save battery life. Try again on the next cycle. 
      return false;
    }
    // wait 4 seconds for connection:
    delay(5000);
  }
  wifiAttempts = 0;
  return true;
}

void buildBuffer() {
    buffer[1] = 10; // temp
    buffer[2] = 32; // moist
    buffer[3] = 150; // photo

    // Wifi attempts
    buffer[4] = wifiAttempts >> 8; 
    buffer[5] = wifiAttempts % 255;

    // Txn number
    buffer[6] = (txnNumber & 0xff000000) >> 24;
    buffer[7] = (txnNumber & 0x00ff0000) >> 16;
    buffer[8] = (txnNumber & 0x0000ff00) >> 8;
    buffer[9] = (txnNumber & 0x000000ff);

    // Checksum
    buffer[0] = ((buffer[1]*5) + (buffer[2]*2) + (buffer[3]*3) + wifiAttempts + txnNumber) % 255;
}

void loop() {
  // Bring up interface
  if (connectWifi()) {
    Udp.begin(5001);
    int result = -1;
    buildBuffer();

    // Write packet
    Udp.beginPacket(hostIp, hostPort);
    Udp.write(buffer, 10);
    Udp.flush();
    result = Udp.endPacket();
    txnNumber++;

    #ifdef SERIALDEBUG
    if (result != 1) {
      Serial.println("Failed to send packet");
    }
    #endif

    delay(1000);
    Udp.stop();
  }

  // Stop interfaces
  WiFi.end();
  status = WL_DISCONNECTED;
  // Sleep
  #ifdef LOW_POWER
  LowPower.sleep(sleepInterval);
  #else
  delay(sleepInterval);
  #endif
}
