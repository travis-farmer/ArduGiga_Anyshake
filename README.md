# 3-Channel High-Precision Seismic Data Acquisition Node

A high-resolution, 3-component ($Z$, $N$, $E$) seismic data acquisition system built around the **Arduino Giga R1 WiFi** and the **ADS1263 32-bit ADC**. Designed to interface directly with **SeisComP** using the **AnyShake** streaming protocol over USB Serial, with time synchronization anchored via a **DS3231 RTC** and a local **NTP server**.

This project is setup for VScode/PlatformIO, and will need modification to be built with the Arduino IDE!

---

## Features

- **3-Axis Differential Acquisition:** Captures vertical ($Z$), north-south ($N$), and east-west ($E$) seismic channels simultaneously.
- **High Resolution:** Uses the Texas Instruments ADS1263 32-bit ADC for high dynamic range velocity measurements.
- **Fixed 100 Hz Sample Rate:** Strictly timed $10\text{ ms}$ sampling loop outputs 100 samples/channel/second.
- **Precision Time Synchronization:** Combines an onboard DS3231 RTC with background Wi-Fi updates from a local GPS-backed NTP server to eliminate time drift.
- **SeisComP Native Integration:** Formats 1-second buffered frames into standard AnyShake ASCII packet strings with XOR checksum verification.

---

## Hardware Architecture & Wiring

### Components
- **Microcontroller:** Arduino Giga R1 WiFi (STM32H7 dual-core)
- **ADC Board:** Waveshare ADS1263 32-bit ADC module
- **Real-Time Clock:** DS3231 Precision I2C RTC Module
- **Sensors:** 3x Geophones (4.5 Hz or similar velocity sensors)

### Pinout Mapping

#### 1. ADS1263 ADC Pinout (SPI)
| ADS1263 Pin | Arduino Giga R1 Pin | Function |
| :--- | :--- | :--- |
| **VCC** | `5V` / `3.3V` | System Power |
| **GND** | `GND` | Common Ground |
| **CS** | `Pin 10` | SPI Chip Select |
| **DIN (MOSI)** | `SPI MOSI` | SPI Data Input |
| **DOUT (MISO)**| `SPI MISO` | SPI Data Output |
| **SCLK** | `SPI SCK` | SPI Clock |
| **START** | `Pin 9` | Conversion Control |
| **DRDY** | `Pin 8` | Data Ready Input |

#### 2. Analog Input Mappings (Geophones)
| Channel | ADS1263 Differential Pair | Sensor Connection |
| :--- | :--- | :--- |
| **Vertical ($Z$)** | `AIN0` / `AIN1` | Geophone 1 ($Z$-Axis) |
| **North-South ($N$)** | `AIN2` / `AIN3` | Geophone 2 ($N$-Axis) |
| **East-West ($E$)** | `AIN4` / `AIN5` | Geophone 3 ($E$-Axis) |

#### 3. DS3231 RTC Pinout (I2C)
| DS3231 Pin | Arduino Giga R1 Pin | Function |
| :--- | :--- | :--- |
| **VCC** | `3.3V` | Logic Power |
| **GND** | `GND` | Common Ground |
| **SDA** | `SDA` (Pin 20) | I2C Data |
| **SCL** | `SCL` (Pin 21) | I2C Clock |

---

## Software Setup

### Prerequisites & Libraries
Install the following dependencies in the **Arduino IDE** or **PlatformIO**:

1. **Board Package:** Arduino Mbed OS Giga Boards (via Arduino Board Manager)
2. **ADS126X Library:** Install `ADS126X` (Included in libs folder, modified for this project only)
3. **RTC Library:** Install `RTClib` by Adafruit
4. **Networking:** Built-in `WiFi` and `WiFiUdp` libraries for Arduino Giga

### Configuration

Open the `.ino` sketch and update the network credentials and NTP server IP:

```cpp
// --- Wi-Fi & NTP Settings ---
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* NTP_SERVER_IP = "192.168.1.100";  // Replace with your local NTP server IP
const String DEVICE_ID    = "9999";           // Station ID matching SeisComP inventory
```
