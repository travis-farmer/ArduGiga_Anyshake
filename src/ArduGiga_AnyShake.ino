/*
 * =================================================================================
 *  Project:      3-Channel High-Precision Seismic Data Acquisition Node
 *  Hardware:     Arduino Giga R1 WiFi, ADS1263 32-bit ADC, DS3231 RTC
 *  Target System: SeisComP / AnyShake Protocol Integration
 * =================================================================================
 * 
 *  Description:
 *  This sketch operates a high-resolution, 3-component (Z, N, E) seismic recording
 *  node. It reads differential analog voltage signals from geophones via an 
 *  ADS1263 32-bit ADC at a constant 100 Hz sampling rate. 
 * 
 *  Time Synchronization & Integrity:
 *  - Primary system time is kept precise via a DS3231 precision RTC over I2C.
 *  - The node connects to a local network over Wi-Fi to asynchronously update 
 *    the DS3231 clock against a local GPS-backed NTP time server.
 *  
 *  Data Framing:
 *  - Buffers 100 samples per channel into 1-second data packets.
 *  - Formats payloads into standard AnyShake ASCII strings with XOR checksums
 *    and streams them over high-speed USB Serial directly into SeisComP.
 * =================================================================================
 *  Authors:      Travis Farmer, Google Gemini, and the Open Seismic Community
 *  Repository:   https://github.com/travis-farmer/ardgiga_seismic_ads1263_AnyShake
 *  Version:      1.0.0
 *  License:      MIT License
 * 
 *  Copyright (c) 2026 Travis Farmer
 * 
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 * 
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 * =================================================================================

ADS1263 pin connections:
ADS1263 Pin,        Arduino Giga Pin,       Notes
VCC,                3.3V or 5V,             Check your breakout board's regulator requirements
GND,                GND,                    Ground plane connection
DIN (MOSI),         Pin 11 (SPI MOSI),      Data from Giga to ADC
DOUT (MISO),        Pin 12 (SPI MISO),      Data from ADC to Giga
SCLK,               Pin 13 (SPI SCK),       Clock signal
CS,                 Pin 10 (Configurable),  Chip Select
START,              Pin 9 (Configurable),   Hardware Sync / Conversion Start
DRDY,               Pin 8 (Configurable),   Data Ready indicator output

DS3231 pin connections:
DS3231 Pin,         Arduino Giga R1 Pin,    Notes
VCC,                3.3V or 5V,             Standard power rails.
GND,                GND,                    Common ground.
SDA,                SDA (Pin 20),           Hardware I2C Data.
SCL,                SCL (Pin 21),           Hardware I2C Clock.
*/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>          // Native Giga WiFi library
#include <WiFiUdp.h>
#include <RTClib.h>        // Adafruit RTClib for the DS3231
#include "arduino_secrets.h"
// --- CRITICAL FIX: Undefine conflicting STM32 hardware macros ---
#ifdef CRC
#undef CRC
#endif
#ifdef ADC1
#undef ADC1
#endif
#ifdef ADC2
#undef ADC2
#endif

#include <ADS126X.h>

// --- Configuration ---
const String DEVICE_ID = "9999"; 
const int SAMPLE_RATE = 100;     
const int CHANNEL_COUNT = 3;     
const unsigned long SAMPLE_INTERVAL_MS = 10; // 10ms = 100Hz

// --- Wi-Fi & NTP Settings ---
const char* WIFI_SSID     = WSSID;
const char* WIFI_PASSWORD = WPSWD;
const char* NTP_SERVER_IP = "192.168.1.84";  // Replace with your local GPS-NTP Pi IP
const unsigned int LOCAL_UDP_PORT = 2390;     // Local port to listen for UDP packets
const unsigned long NTP_SYNC_INTERVAL = 12 * 60 * 60 * 1000UL; // Sync DS3231 every 12 hours

// --- Hardware Pins ---
const int PIN_CS = 10;
const int PIN_START = 9;
const int PIN_DRDY = 8;

// --- Instantiations ---
ADS126X adc;
RTC_DS3231 rtc;
WiFiUDP udp;

// --- Data Buffers ---
int32_t bufferZ[SAMPLE_RATE];
int32_t bufferN[SAMPLE_RATE];
int32_t bufferE[SAMPLE_RATE];
int sampleIndex = 0;
unsigned long packetSequence = 0;
unsigned long lastSampleTime = 0;
unsigned long lastNTPSyncTime = 0;

// --- Clock Registers ---
uint8_t  activeHour = 0, activeMinute = 0, activeSecond = 0;
uint16_t activeYear = 2026;
uint8_t  activeMonth = 5,  activeDay = 18;

// --- Helper: Compute NMEA XOR Checksum ---
String calculateChecksum(String str) {
    uint8_t checksum = 0;
    for (unsigned int i = 0; i < str.length(); i++) {
        checksum ^= str[i];
    }
    char hexBuf[3];
    sprintf(hexBuf, "%02X", checksum);
    return String(hexBuf);
}

// --- Helper: Build AnyShake Timestamp (Formatted to Start of the Second) ---
String getRTCTimestamp() {
    char timeBuf[32];
    // AnyShake format: YYYYMMDDHHMMSS.000
    sprintf(timeBuf, "%04d%02d%02d%02d%02d%02d.000", 
            activeYear, activeMonth, activeDay,
            activeHour, activeMinute, activeSecond);
    return String(timeBuf);
}

// --- Send raw NTP request packet ---
void sendNTPpacket(const char* address) {
    byte packetBuffer[48];
    memset(packetBuffer, 0, 48);
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    
    udp.beginPacket(address, 123); // NTP requests are sent on port 123
    udp.write(packetBuffer, 48);
    udp.endPacket();
}

// --- Asynchronous NTP Query Handler ---
void syncRTCOverNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] WiFi lost! Skipping sync.");
        return;
    }

    Serial.println("[NTP] Requesting time update...");
    while (udp.parsePacket() > 0); // Flush any stale buffer data
    
    sendNTPpacket(NTP_SERVER_IP);
    
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500) {
        int size = udp.parsePacket();
        if (size >= 48) {
            byte packetBuffer[48];
            udp.read(packetBuffer, 48);

            // Transmit timestamp is in bytes 40-43
            unsigned long secsSince1900;
            secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];

            // Convert to Unix Epoch (seconds since Jan 1, 1970)
            const unsigned long seventyYears = 2208988800UL;
            unsigned long epochTime = secsSince1900 - seventyYears;

            // Note: Keep RTC set in pure UTC to avoid local DST drift!
            rtc.adjust(DateTime(epochTime));
            lastNTPSyncTime = millis();
            Serial.println("[NTP] DS3231 synchronized successfully!");
            return;
        }
    }
    Serial.println("[NTP] Sync timed out. Operating on RTC internal oscillator.");
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    
    // Initialize DS3231 I2C interface
    if (!rtc.begin()) {
        Serial.println("[SYS] Critical Error: DS3231 RTC not found!");
        while (1);
    }

    // Connect to network
    Serial.print("[WIFI] Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long wifiWait = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiWait < 10000) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] Connected! IP Address: ");
        Serial.println(WiFi.localIP());
        udp.begin(LOCAL_UDP_PORT);
        syncRTCOverNTP(); // Fetch initial NTP sync straight away
    } else {
        Serial.println("\n[WIFI] Connection failed. Using current DS3231 time.");
    }

    // Initialize ADS1263 SPI settings
    adc.begin(PIN_CS);
    pinMode(PIN_START, OUTPUT);
    pinMode(PIN_DRDY, INPUT);

    adc.reset();
    delay(10);

    // Set sample tracking rates
    adc.setRate(ADS126X_RATE_400); 

    // Enable conversion runs
    digitalWrite(PIN_START, HIGH);
    adc.startADC1();
    
    lastSampleTime = millis();
}

void loop() {
    unsigned long currentTime = millis();
    
    // 1. Asynchronous check to update the DS3231 over NTP
    if (currentTime - lastNTPSyncTime >= NTP_SYNC_INTERVAL) {
        syncRTCOverNTP();
    }
    
    // 2. High-Resolution 100Hz Signal Acquisition Loop
    if (currentTime - lastSampleTime >= SAMPLE_INTERVAL_MS) {
        lastSampleTime += SAMPLE_INTERVAL_MS;
        
        // At the absolute first step of our new 1-second block, lock down the timestamp
        if (sampleIndex == 0) {
            DateTime now = rtc.now();
            activeHour   = now.hour();
            activeMinute = now.minute();
            activeSecond = now.second();
            activeDay    = now.day();
            activeMonth  = now.month();
            activeYear   = now.year();
        }

        // Grab values across differential channel mappings
        bufferZ[sampleIndex] = adc.readADC1(ADS126X_AIN0, ADS126X_AIN1);
        bufferN[sampleIndex] = adc.readADC1(ADS126X_AIN2, ADS126X_AIN3);
        bufferE[sampleIndex] = adc.readADC1(ADS126X_AIN4, ADS126X_AIN5);
        
        sampleIndex++;
        
        // 3. Once 100 samples are buffered (Exactly 1 Second), compile and dispatch
        if (sampleIndex >= SAMPLE_RATE) {
            String dataZStr = "";
            String dataNStr = "";
            String dataEStr = "";
            
            for (int i = 0; i < SAMPLE_RATE; i++) {
                dataZStr += String(bufferZ[i]) + " ";
                dataNStr += String(bufferN[i]) + " ";
                dataEStr += String(bufferE[i]) + " ";
            }
            dataZStr.trim();
            dataNStr.trim();
            dataEStr.trim();
            
            // Build the core payload string mapped to the locked DS3231 timestamp
            String payload = "AS," + DEVICE_ID + "," + 
                             String(packetSequence) + "," + 
                             getRTCTimestamp() + "," + 
                             String(SAMPLE_RATE) + "," + 
                             String(CHANNEL_COUNT) + "," + 
                             dataZStr + "," + dataNStr + "," + dataEStr;
            
            String csum = calculateChecksum(payload);
            String finalPacket = "$" + payload + "*" + csum;
            
            // Stream the packet string frame to your SeisComP node interface
            Serial.println(finalPacket); 
            
            // Reset block variables for the next cycle
            sampleIndex = 0;
            packetSequence++;
            if (packetSequence > 99999) packetSequence = 0;
        }
    }
}