#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <DHT.h>
#include <ArduinoJson.h>

// WiFi credentials
const char *ssid = "shiva wifi";
const char *password = "khanalshiva";

// Web server on port 80
WebServer server(80);

// DHT setup
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// HTML with canvas-based speedometer UI
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Weather Speedometer</title>
  <style>
    body {
      margin: 0;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      height: 100vh;
      background: #2c3e50;
      color: #fff;
      font-family: 'Segoe UI', sans-serif;
    }
    canvas {
      background: #ecf0f1;
      border-radius: 50%;
      box-shadow: 0 10px 20px rgba(0,0,0,0.3);
    }
    h2 { margin: 20px; }
  </style>
</head>
<body>
  <h2>🌡 Temperature & 💧 Humidity</h2>
  <canvas id="gaugeTemp" width="250" height="250"></canvas>
  <canvas id="gaugeHum" width="250" height="250"></canvas>

<script>
function drawGauge(canvasId, value, label, max, color) {
  const canvas = document.getElementById(canvasId);
  const ctx = canvas.getContext('2d');
  const w = canvas.width, h = canvas.height;
  ctx.clearRect(0, 0, w, h);

  const centerX = w / 2;
  const centerY = h / 2;
  const radius = w / 2 - 20;
  const startAngle = Math.PI;
  const endAngle = 0;

  // Draw arc background
  ctx.beginPath();
  ctx.arc(centerX, centerY, radius, startAngle, endAngle, false);
  ctx.lineWidth = 20;
  ctx.strokeStyle = "#bdc3c7";
  ctx.stroke();

  // Draw arc value
  const angle = startAngle + (value / max) * Math.PI;
  ctx.beginPath();
  ctx.arc(centerX, centerY, radius, startAngle, angle, false);
  ctx.lineWidth = 20;
  ctx.strokeStyle = color;
  ctx.stroke();

  // Draw text
  ctx.fillStyle = "#2c3e50";
  ctx.font = "20px Segoe UI";
  ctx.textAlign = "center";
  ctx.fillText(label, centerX, centerY - 10);
  ctx.font = "26px bold";
  ctx.fillText(value.toFixed(1), centerX, centerY + 30);
}

// Fetch sensor data
async function updateData() {
  try {
    const res = await fetch('/data');
    const json = await res.json();
    drawGauge('gaugeTemp', json.temperature, "Temperature (°C)", 50, "#e74c3c");
    drawGauge('gaugeHum', json.humidity, "Humidity (%)", 100, "#3498db");
  } catch (err) {
    console.error("Error fetching:", err);
  }
}

setInterval(updateData, 10000);
updateData();
</script>
</body>
</html>
)rawliteral";

// Serve HTML page
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Serve temperature and humidity as JSON
void handleData() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    server.send(500, "application/json", "{\"error\": \"Sensor failure\"}");
    return;
  }

  StaticJsonDocument<100> doc;
  doc["temperature"] = temp;
  doc["humidity"] = hum;
  String jsonStr;
  serializeJson(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
