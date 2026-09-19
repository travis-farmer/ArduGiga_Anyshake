# ArduGiga AnyShake

An open-source, high-precision 3-component seismic digitizer built on the **Arduino Giga R1 WiFi** (STM32H7) and Texas Instruments **ADS1263** 32-bit ADC. This node streams real-time seismic waveform data natively using the `anyshake` protocol over serial to a **SeisComP** server.

---

## 🚀 Features

* **High-Resolution Digitization:** 32-bit delta-sigma ADC sampling via Texas Instruments ADS1263.
* **Local NTP Time Synchronization:** Onboard STM32 RTC synchronized via local Wi-Fi NTP using Mbed OS C-time routines (`set_time()`, `gmtime()`), eliminating the need for external GPS modules or RTC chips.
* **Native Protocol Integration:** Direct serial streaming implementing the standard `anyshake` format for SeisComP integration.
* **Self-Contained Build:** Custom `SPI1`-adapted `ADS126X` driver bundled within `lib/ADS126X/` for reproducible PlatformIO builds.
* **Open Source:** Released under the permissive **MIT License**.

---

## 🛠 Hardware Architecture

* **Microcontroller:** Arduino Giga R1 WiFi (Dual ARM Cortex-M7 @ 480MHz / Cortex-M4 @ 240MHz)
* **ADC:** TI ADS1263 32-bit ADC Breakout
* **Network / Time Sync:** Onboard Murata 1DX Wi-Fi module syncing local NTP

### Pinout Configuration (Arduino Giga R1 to ADS1263)

> **Note:** Communication utilizes the secondary hardware SPI bus (`SPI1`) to prevent bus conflicts on the primary header pins. **SPI** is wired to the **ICSP** header, whereas **SPI1** is wired to the pins D11 - D13.

| Signal Name | Giga R1 Pin | ADS1263 Pin | Description |
| :--- | :--- | :--- | :--- |
| **CIPO (MISO)** | `D12` (`CIPO1`) | `DOUT` / `DRDY` | SPI1 Data Input |
| **COPI (MOSI)** | `D11` (`COPI1`) | `DIN` | SPI1 Data Output |
| **SCK** | `D13` (`SCK1`) | `SCLK` | SPI1 Clock Line |
| **CS** | Digital Pin `10` | `CS` | Chip Select |
| **START** | Digital Pin `2` | `START` | Active HIGH for continuous sampling |
| **PWDN** | Digital Pin `3` | `PWDN` | Active HIGH for power enabled |
| **3.3V** | `3.3V` | `DVDD` | Digital Power Supply (3.3V Logic) |
| **5V** | `5V` | `AVDD` | Analog Power Supply |
| **GND** | `GND` | `DGND` / `AGND` | Common Ground |

---

### Configuration

Open the `.ino` sketch and update the network credentials and NTP server IP:

```cpp
// --- Wi-Fi & NTP Settings ---
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* NTP_SERVER_IP = "192.168.1.100";  // Replace with your local NTP server IP
const String DEVICE_ID    = "9999";           // Station ID matching SeisComP inventory
```
