# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Planned
- Hardware interrupt-driven $1\text{ Hz}$ tick line using the DS3231 SQW output pin.
- Support for internal SD card backup logging during network outages.

---

## [1.0.0] - 2026-09-15

### Added
- **Hardware Architecture:** Support for Arduino Giga R1 WiFi core processing unit.
- **3-Axis Data Acquisition:** Differential 3-channel ($Z$, $N$, $E$) geophone sampling at 100 Hz using the Texas Instruments ADS1263 32-bit ADC over SPI.
- **Precision Time Synchronization:** Integrated DS3231 I2C RTC timekeeping with non-blocking Wi-Fi NTP synchronization against a local GPS-backed NTP server.
- **Data Protocol:** AnyShake ASCII packet framing with standard NMEA XOR checksum verification.
- **Documentation:** Added `README.md`, `LICENSE` (MIT), and `CHANGELOG.md`.

[1.0.0]: https://github.com/travis-farmer/ardgiga_seismic_ads1263_AnyShake/releases/tag/v1.0.0
