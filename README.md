# ESP32-S3 + OLED + AHT20 Wiring

## Components

- ESP32-S3 dev board
- I²C OLED display (SSD1306, 0x3C)
- AHT20 temperature & humidity sensor (0x38)

---

## I²C Pins (ESP32-S3)

- SDA → GPIO17  
- SCL → GPIO18  

---

## Power (3.3 V)

3V3 → OLED VCC + AHT20 VDD  
GND → OLED GND + AHT20 GND  

---

## OLED Connections

VCC → 3V3  
GND → GND  
SDA → GPIO17  
SCL → GPIO18  

---

## AHT20 Connections

VDD → 3V3  
GND → GND  
SDA → GPIO17  
SCL → GPIO18  

---

## Notes

- Both devices share the same I²C bus  
- Use 3.3 V only (not 5 V)  
- Initialize I²C in code:

