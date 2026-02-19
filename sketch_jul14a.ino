#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

#define AHT20_ADDR 0x38

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // I2C 
  Wire.begin(17, 18);

  // 初始化 OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED not found");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Boot...");
  display.display();

  // 初始化 AHT20
  Wire.beginTransmission(AHT20_ADDR);
  Wire.write(0xBE);
  Wire.write(0x08);
  Wire.write(0x00);
  Wire.endTransmission();

  delay(10);
}

void loop() {

  // 触发测量
  Wire.beginTransmission(AHT20_ADDR);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  Wire.endTransmission();

  delay(80);

  // 读取数据
  Wire.requestFrom(AHT20_ADDR, 6);

  float temperature = 0;
  float humidity = 0;

  if (Wire.available() == 6) {
    uint8_t data[6];
    for (int i = 0; i < 6; i++) data[i] = Wire.read();

    uint32_t rawHum =
      ((uint32_t)data[1] << 16 |
       (uint32_t)data[2] << 8 |
       data[3]) >> 4;

    humidity = rawHum * 100.0 / 1048576.0;

    uint32_t rawTemp =
      (((uint32_t)data[3] & 0x0F) << 16) |
      ((uint32_t)data[4] << 8) |
      data[5];

    temperature = rawTemp * 200.0 / 1048576.0 - 50;
  }

  // 串口输出
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" C   ");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  // OLED 显示
  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("T:");
  display.print(temperature, 1);
  display.println("C");

  display.setCursor(0, 32);
  display.print("H:");
  display.print(humidity, 1);
  display.println("%");

  display.display();

  delay(2000);
}
