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

## [2.0.0] - 2026-09-19

### Added
- Integrated customized `ADS126X` library into local project structure (`lib/ADS126X/`), making the repository fully self-contained and version-controlled.
- Added explicit GPIO pin controls for `START` (Pin 2) and `PWDN` (Pin 3) in `setup()` to enforce continuous conversions and active power state.
- Added explicit SPI settings configuration targeting **SPI Mode 1** at 1 MHz clock frequency for stable high-resolution ADC register operations.

### Changed
- **BREAKING CHANGE**: Migrated all ADC SPI communication routines from default `SPI` bus to `SPI1` to align with Arduino Giga R1 physical header assignments (COPI1 / CIPO1 / SCK1).
- Updated internal `ADS126X` library calls to target `SPI1` hardware transactions natively.

### Fixed
- Fixed persistent `-1` (`0xFFFFFFFF`) read values from the ADS1263 ADC caused by bus assignment conflicts on the main SPI header pins.
