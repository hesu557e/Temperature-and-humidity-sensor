#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED
#define OLED_W 128
#define OLED_H 64
#define OLED_I2C 0x3C

// AHT20
#define AHT20_ADDR 0x38

// I2C pins
#define SDA_PIN 17
#define SCL_PIN 18

Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);

// WiFi AP
const char* ssid = "ESP32_AHT20_DEMO";
const char* pass = "12345678";

// network
WebServer http(80);
WebSocketsServer ws(81);

// simple page
const char PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>AHT20</title>
<style>
body{font-family:sans-serif;padding:20px}
.box{margin:12px 0;padding:14px;border:1px solid #ddd;border-radius:10px}
.val{font-size:40px}
</style>
</head>
<body>

<h2>ESP32 Sensor</h2>

<div class="box">
  <div>Temperature</div>
  <div class="val" id="t">--</div>
</div>

<div class="box">
  <div>Humidity</div>
  <div class="val" id="h">--</div>
</div>

<div id="s">connecting...</div>

<script>
let t = document.getElementById('t');
let h = document.getElementById('h');
let s = document.getElementById('s');

function start(){
  let sock = new WebSocket(`ws://${location.hostname}:81/`);

  sock.onopen = ()=> s.innerText="connected";
  sock.onclose = ()=> { s.innerText="retry..."; setTimeout(start,800); };

  sock.onmessage = (e)=>{
    try{
      let d = JSON.parse(e.data);
      if(d.t!=null) t.innerText=d.t.toFixed(1);
      if(d.h!=null) h.innerText=d.h.toFixed(1);
    }catch{}
  };
}
start();
</script>

</body>
</html>
)rawliteral";


// --- sensor ---

void ahtInit() {
  Wire.beginTransmission(AHT20_ADDR);
  Wire.write(0xBE);
  Wire.write(0x08);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(10);
}

bool ahtRead(float &t, float &h) {
  Wire.beginTransmission(AHT20_ADDR);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) return false;

  delay(80);

  Wire.requestFrom(AHT20_ADDR, 6);
  if (Wire.available() != 6) return false;

  uint8_t d[6];
  for (int i = 0; i < 6; i++) d[i] = Wire.read();

  uint32_t rh = (((uint32_t)d[1] << 16) | ((uint32_t)d[2] << 8) | d[3]) >> 4;
  h = rh * 100.0f / 1048576.0f;

  uint32_t rt = (((uint32_t)(d[3] & 0x0F)) << 16) | ((uint32_t)d[4] << 8) | d[5];
  t = rt * 200.0f / 1048576.0f - 50.0f;

  return true;
}


// --- ws debug ---
void wsEvent(uint8_t id, WStype_t type, uint8_t*, size_t) {
  if (type == WStype_CONNECTED) {
    Serial.printf("client %u connected\n", id);
  } else if (type == WStype_DISCONNECTED) {
    Serial.printf("client %u left\n", id);
  }
}


// --- setup ---
void setup() {
  Serial.begin(115200);
  delay(300);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C)) {
    Serial.println("OLED fail");
    while (1);
  }

  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(0, 0);
  oled.print("Boot");
  oled.display();

  ahtInit();

  WiFi.softAP(ssid, pass);
  IPAddress ip = WiFi.softAPIP();

  Serial.println(ip);

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("AP ready");
  oled.println(ssid);
  oled.println(ip);
  oled.display();

  http.on("/", []() {
    http.send_P(200, "text/html", PAGE);
  });

  http.begin();

  ws.begin();
  ws.onEvent(wsEvent);
}


// --- loop ---
void loop() {
  http.handleClient();
  ws.loop();

  float t = 0, h = 0;
  bool ok = ahtRead(t, h);

  if (ok) {
    Serial.printf("%.2f C  %.2f %%\n", t, h);
  }

  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(0, 0);

  oled.print("T:");
  oled.print(ok ? String(t,1) : "--");
  oled.println("C");

  oled.setCursor(0, 32);
  oled.print("H:");
  oled.print(ok ? String(h,1) : "--");
  oled.println("%");

  oled.display();

  if (ok) {
    String msg = String("{\"t\":") + String(t,2) + ",\"h\":" + String(h,2) + "}";
    ws.broadcastTXT(msg);
  }

  delay(1000);
}
