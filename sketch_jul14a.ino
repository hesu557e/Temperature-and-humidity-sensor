#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define AHT20_ADDR 0x38

// 你的 I2C 接法：SDA=17, SCL=18
#define I2C_SDA 17
#define I2C_SCL 18

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ===== WiFi SoftAP 配置 =====
const char* AP_SSID = "ESP32_AHT20_DEMO";
const char* AP_PASS = "12345678";

// ===== HTTP / WebSocket =====
WebServer server(80);
// WebSocketsServer 需要一个端口，常用 81
WebSocketsServer webSocket = WebSocketsServer(81);

// ===== 网页（直接由 ESP32 提供）=====
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>ESP32 AHT20 Live</title>
  <style>
    body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Arial; padding:20px; line-height:1.4}
    .wrap{max-width:420px}
    .card{border:1px solid #ddd; border-radius:14px; padding:16px; margin:12px 0}
    .label{color:#666; font-size:14px}
    .value{font-size:44px; margin-top:6px}
    .status{color:#666; font-size:13px; margin-top:10px}
  </style>
</head>
<body>
  <div class="wrap">
    <h2>ESP32 AHT20 Live Data</h2>

    <div class="card">
      <div class="label">Temperature</div>
      <div class="value" id="t">--</div>
      <div class="label">°C</div>
    </div>

    <div class="card">
      <div class="label">Humidity</div>
      <div class="value" id="h">--</div>
      <div class="label">%</div>
    </div>

    <div class="status" id="s">connecting...</div>
  </div>

<script>
  const tEl = document.getElementById('t');
  const hEl = document.getElementById('h');
  const sEl = document.getElementById('s');

  function connect() {
    // 注意：WebSocket 端口是 81（不是 80）
    const wsUrl = `ws://${location.hostname}:81/`;
    const ws = new WebSocket(wsUrl);

    ws.onopen = () => sEl.textContent = "WebSocket connected ✅";
    ws.onclose = () => { sEl.textContent = "disconnected, retrying..."; setTimeout(connect, 800); };
    ws.onerror = () => sEl.textContent = "error";

    ws.onmessage = (ev) => {
      // 期望收到 JSON: {"t":25.12,"h":40.33}
      try {
        const obj = JSON.parse(ev.data);
        if (typeof obj.t === "number") tEl.textContent = obj.t.toFixed(1);
        if (typeof obj.h === "number") hEl.textContent = obj.h.toFixed(1);
      } catch(e) {
        sEl.textContent = "bad data";
      }
    };
  }
  connect();
</script>
</body>
</html>
)rawliteral";

// ===== AHT20 初始化 / 读取 =====
void initAHT20() {
  // 你原来那段初始化保持不动
  Wire.beginTransmission(AHT20_ADDR);
  Wire.write(0xBE);
  Wire.write(0x08);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(10);
}

bool readAHT20(float &temperature, float &humidity) {
  // 触发测量
  Wire.beginTransmission(AHT20_ADDR);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) return false;

  delay(80);

  // 读取数据
  Wire.requestFrom(AHT20_ADDR, 6);
  if (Wire.available() != 6) return false;

  uint8_t data[6];
  for (int i = 0; i < 6; i++) data[i] = Wire.read();

  uint32_t rawHum =
      (((uint32_t)data[1] << 16) |
       ((uint32_t)data[2] << 8) |
       (uint32_t)data[3]) >> 4;
  humidity = rawHum * 100.0f / 1048576.0f;

  uint32_t rawTemp =
      (((uint32_t)(data[3] & 0x0F)) << 16) |
      ((uint32_t)data[4] << 8) |
      (uint32_t)data[5];
  temperature = rawTemp * 200.0f / 1048576.0f - 50.0f;

  return true;
}

// ===== WebSocket 事件回调（可选：看连接状态）=====
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("WS client #%u connected\n", num);
      break;
    case WStype_DISCONNECTED:
      Serial.printf("WS client #%u disconnected\n", num);
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // I2C 初始化
  Wire.begin(I2C_SDA, I2C_SCL);

  // OLED 初始化
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED not found");
    while (1) {}
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Boot...");
  display.display();

  // AHT20 初始化
  initAHT20();

  // ===== 开 WiFi 热点 =====
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP SSID: "); Serial.println(AP_SSID);
  Serial.print("AP IP: "); Serial.println(ip); // 通常 192.168.4.1

  // OLED 显示 WiFi 信息
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("WiFi SoftAP Ready");
  display.print("SSID: "); display.println(AP_SSID);
  display.print("IP: "); display.println(ip);
  display.println("Open Safari:");
  display.println("http://192.168.4.1");
  display.display();

  // ===== HTTP：返回网页 =====
  server.on("/", []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/ping", []() {
    server.send(200, "text/plain", "ok");
  });

  server.begin();

  // ===== WebSocket：端口 81 =====
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
}

void loop() {
  // 必须在 loop 里处理客户端
  server.handleClient();
  webSocket.loop();

  float temperature = 0.0f, humidity = 0.0f;
  bool ok = readAHT20(temperature, humidity);

  // 串口输出
  if (ok) {
    Serial.printf("Temp: %.2f C   Humidity: %.2f %%\n", temperature, humidity);
  } else {
    Serial.println("AHT20 read failed");
  }

  // OLED 显示（保留你原来风格）
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("T:");
  if (ok) display.print(temperature, 1);
  else display.print("--");
  display.println("C");

  display.setCursor(0, 32);
  display.print("H:");
  if (ok) display.print(humidity, 1);
  else display.print("--");
  display.println("%");
  display.display();

  // WebSocket 推送（JSON）
  if (ok) {
    String msg = String("{\"t\":") + String(temperature, 2) + ",\"h\":" + String(humidity, 2) + "}";
    // 广播给所有连接的网页客户端
    webSocket.broadcastTXT(msg);
  }

  delay(1000); // 你要更“实时”就改 200~500ms
}
