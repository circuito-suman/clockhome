# Clock Home — LED Matrix Clock

A 4-module MAX7219 LED matrix clock with DS3231 RTC and rotary encoder.

## Hardware
- 4x MAX7219 8x8 LED matrix modules (32x8 total)
- DS3231/DS1307/DS1302/PCF8563 RTC
- Rotary encoder with push button

## Variants

### arduino_esp32/
Original Arduino code for ESP32 using MD_Parola and RTClib libraries.

### bl602_bouffalo_sdk/
Port to BL602 using Bouffalo SDK (bouffalo_sdk).
- No Arduino dependencies
- Raw SPI via bflb_spi
- Raw I2C via bflb_i2c  
- GPIO interrupt-driven encoder
- Multi-RTC support: DS3231, DS1307, DS1302, PCF8563

#### BL602 Pin Mapping
| Function | GPIO |
|---|---|
| Encoder A | GPIO10 |
| Encoder B | GPIO11 |
| Encoder BTN | GPIO12 |
| MAX7219 CS | GPIO3 |
| SPI CLK | GPIO18 |
| SPI MOSI | GPIO19 |
| I2C SDA | GPIO4 |
| I2C SCL | GPIO5 |

#### Build
```bash
export BL_SDK_BASE=~/Documents/bouffalo_sdk
cd bl602_bouffalo_sdk
make CHIP=bl602 BOARD=bl602dk CROSS_COMPILE=riscv64-unknown-elf-
```
