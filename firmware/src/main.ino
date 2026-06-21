/**
 * Moonshine Cooler - Smart Cooling System
 * ESP8266 Main Firmware
 * 
 * Автономное охлаждение самогонного аппарата
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============ КОНФИГУРАЦИЯ ============

// WiFi
const char* SSID = "YOUR_SSID";
const char* PASSWORD = "YOUR_PASSWORD";

// GPIO Пины
const int TEMP_SENSOR_PIN = D4;      // DS18B20 датчик
const int COMPRESSOR_RELAY_PIN = D5; // Реле компрессора
const int FAN_RELAY_PIN = D6;        // Реле вентилятора
const int LCD_SDA_PIN = D2;          // I2C SDA для дисплея
const int LCD_SCL_PIN = D1;          // I2C SCL для дисплея

// Дисплей LCD I2C (адрес обычно 0x27 или 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Датчики температуры
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

// Веб-сервер
ESP8266WebServer server(80);

// ============ ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ============

struct SystemState {
  float temp_liquid = 0.0;      // Температура охлаждающей жидкости
  float temp_vapor = 0.0;       // Температура паров (если есть второй датчик)
  bool compressor_on = false;   // Статус компрессора
  bool fan_on = false;          // Статус вентилятора
  bool auto_mode = false;       // Автоматический режим
  float target_temp = 25.0;     // Целевая температура
  float temp_hysteresis = 2.0;  // Гистерезис
  unsigned long last_update = 0; // Время последнего обновления
} state;

// ============ ФУНКЦИИ ============

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("   Moonshine Cooler System Starting");
  Serial.println("========================================");
  
  // Инициализация GPIO
  pinMode(COMPRESSOR_RELAY_PIN, OUTPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);
  digitalWrite(COMPRESSOR_RELAY_PIN, LOW);
  digitalWrite(FAN_RELAY_PIN, LOW);
  
  // Инициализация дисплея
  Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Moonshine Cooler");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  
  // Инициализация датчиков температуры
  sensors.begin();
  Serial.println("[SENSORS] DS18B20 initialized");
  
  // Подключение к WiFi
  connectToWiFi();
  
  // Настройка веб-сервера
  setupWebServer();
  server.begin();
  
  Serial.println("[SETUP] System initialized successfully");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP: " + WiFi.localIP().toString());
}

void loop() {
  // Обработка HTTP запросов
  server.handleClient();
  
  // Обновление датчиков (каждые 1000ms)
  if (millis() - state.last_update >= 1000) {
    updateTemperatures();
    updateControlLogic();
    updateDisplay();
    state.last_update = millis();
  }
  
  delay(10);
}

// ============ WiFi ============

void connectToWiFi() {
  Serial.print("[WiFi] Connecting to ");
  Serial.println(SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("[WiFi] Connected!");
    Serial.print("[WiFi] IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("[WiFi] Connection failed!");
  }
}

// ============ ДАТЧИКИ ============

void updateTemperatures() {
  sensors.requestTemperatures();
  
  if (sensors.getDeviceCount() > 0) {
    state.temp_liquid = sensors.getTempCByIndex(0);
    Serial.print("[TEMP] Liquid: ");
    Serial.print(state.temp_liquid);
    Serial.println("°C");
  }
  
  if (sensors.getDeviceCount() > 1) {
    state.temp_vapor = sensors.getTempCByIndex(1);
    Serial.print("[TEMP] Vapor: ");
    Serial.print(state.temp_vapor);
    Serial.println("°C");
  }
}

// ============ УПРАВЛЕНИЕ ============

void updateControlLogic() {
  if (state.auto_mode) {
    // Автоматический режим с гистерезисом
    if (state.temp_liquid >= state.target_temp + state.temp_hysteresis) {
      // Включить охлаждение
      setCompressor(true);
    } else if (state.temp_liquid <= state.target_temp - state.temp_hysteresis) {
      // Выключить охлаждение
      setCompressor(false);
    }
  }
}

void setCompressor(bool on) {
  if (state.compressor_on != on) {
    state.compressor_on = on;
    digitalWrite(COMPRESSOR_RELAY_PIN, on ? HIGH : LOW);
    Serial.print("[CONTROL] Compressor: ");
    Serial.println(on ? "ON" : "OFF");
  }
}

void setFan(bool on) {
  if (state.fan_on != on) {
    state.fan_on = on;
    digitalWrite(FAN_RELAY_PIN, on ? HIGH : LOW);
    Serial.print("[CONTROL] Fan: ");
    Serial.println(on ? "ON" : "OFF");
  }
}

// ============ ДИСПЛЕЙ ============

void updateDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(state.temp_liquid, 1);
  lcd.print("C ");
  
  if (state.auto_mode) {
    lcd.print("AUTO");
  } else {
    lcd.print("MAN ");
  }
  
  if (state.compressor_on) {
    lcd.print(" C");
  }
  if (state.fan_on) {
    lcd.print("F");
  }
  
  lcd.setCursor(0, 1);
  if (state.auto_mode) {
    lcd.print("Target:");
    lcd.print(state.target_temp, 1);
    lcd.print("C");
  } else {
    lcd.print("Manual mode");
  }
}

// ============ ВЕБ-СЕРВЕР ============

void setupWebServer() {
  // REST API endpoints
  
  // GET /api/status - Получить текущий статус системы
  server.on("/api/status", HTTP_GET, []() {
    handleStatus();
  });
  
  // POST /api/compressor - Управление компрессором
  server.on("/api/compressor", HTTP_POST, []() {
    handleCompressorControl();
  });
  
  // POST /api/fan - Управление вентилятором
  server.on("/api/fan", HTTP_POST, []() {
    handleFanControl();
  });
  
  // POST /api/mode - Изменить режим (auto/manual)
  server.on("/api/mode", HTTP_POST, []() {
    handleModeChange();
  });
  
  // POST /api/temperature - Установить целевую температуру
  server.on("/api/temperature", HTTP_POST, []() {
    handleTemperatureSet();
  });
  
  // GET / - Главная страница (веб-интерфейс)
  server.on("/", HTTP_GET, []() {
    handleRoot();
  });
  
  // Статические файлы CSS и JS
  server.on("/css/style.css", HTTP_GET, []() {
    server.sendHeader("Content-Type", "text/css");
    server.send(200, "text/css", getCSSContent());
  });
  
  server.on("/js/app.js", HTTP_GET, []() {
    server.sendHeader("Content-Type", "application/javascript");
    server.send(200, "application/javascript", getJSContent());
  });
  
  Serial.println("[SERVER] Web server configured");
}

void handleStatus() {
  DynamicJsonDocument doc(512);
  
  doc["temp_liquid"] = state.temp_liquid;
  doc["temp_vapor"] = state.temp_vapor;
  doc["compressor_on"] = state.compressor_on;
  doc["fan_on"] = state.fan_on;
  doc["auto_mode"] = state.auto_mode;
  doc["target_temp"] = state.target_temp;
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  
  server.sendHeader("Content-Type", "application/json");
  server.send(200, "application/json", response);
}

void handleCompressorControl() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (!error) {
      bool on = doc["on"] | false;
      setCompressor(on);
      
      DynamicJsonDocument response(128);
      response["success"] = true;
      response["compressor_on"] = state.compressor_on;
      
      String json;
      serializeJson(response, json);
      server.send(200, "application/json", json);
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    }
  }
}

void handleFanControl() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (!error) {
      bool on = doc["on"] | false;
      setFan(on);
      
      DynamicJsonDocument response(128);
      response["success"] = true;
      response["fan_on"] = state.fan_on;
      
      String json;
      serializeJson(response, json);
      server.send(200, "application/json", json);
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    }
  }
}

void handleModeChange() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (!error) {
      state.auto_mode = doc["auto"] | false;
      
      if (!state.auto_mode) {
        setCompressor(false);
        setFan(false);
      }
      
      DynamicJsonDocument response(128);
      response["success"] = true;
      response["auto_mode"] = state.auto_mode;
      
      String json;
      serializeJson(response, json);
      server.send(200, "application/json", json);
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    }
  }
}

void handleTemperatureSet() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (!error) {
      float temp = doc["target_temp"] | 25.0;
      if (temp >= 0 && temp <= 100) {
        state.target_temp = temp;
        
        DynamicJsonDocument response(128);
        response["success"] = true;
        response["target_temp"] = state.target_temp;
        
        String json;
        serializeJson(response, json);
        server.send(200, "application/json", json);
      } else {
        server.send(400, "application/json", "{\"error\":\"Temperature out of range\"}");
      }
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    }
  }
}

void handleRoot() {
  server.send(200, "text/html", getHTMLContent());
}

// ============ HTML/CSS/JS КОНТЕНТ ============

String getHTMLContent() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Moonshine Cooler - System Control</title>
  <link rel="stylesheet" href="/css/style.css">
</head>
<body>
  <div class="container">
    <header>
      <h1>🌡️ Moonshine Cooler</h1>
      <p>Система управления охлаждением</p>
    </header>
    
    <main>
      <!-- Статус датчиков -->
      <section class="status-section">
        <h2>Температура</h2>
        <div class="temperature-display">
          <div class="temp-card">
            <label>Охлаждающая жидкость</label>
            <div class="temp-value" id="tempLiquid">-°C</div>
          </div>
          <div class="temp-card">
            <label>Пары</label>
            <div class="temp-value" id="tempVapor">-°C</div>
          </div>
        </div>
      </section>
      
      <!-- Режимы работы -->
      <section class="mode-section">
        <h2>Режим работы</h2>
        <div class="mode-buttons">
          <button class="mode-btn active" id="manualBtn" onclick="setMode(false)">
            🎛️ Ручной
          </button>
          <button class="mode-btn" id="autoBtn" onclick="setMode(true)">
            🤖 Автомат
          </button>
        </div>
      </section>
      
      <!-- Автоматический режим -->
      <section class="auto-section" id="autoSection" style="display:none;">
        <h2>Автоматическое управление</h2>
        <div class="control-group">
          <label for="targetTemp">Целевая температура (°C):</label>
          <input type="number" id="targetTemp" min="0" max="100" value="25" step="0.5">
          <button onclick="setTargetTemperature()">Установить</button>
        </div>
        <div class="info-box">
          <p>Система автоматически включит охлаждение при превышении целевой температуры</p>
        </div>
      </section>
      
      <!-- Ручной режим -->
      <section class="manual-section" id="manualSection">
        <h2>Ручное управление</h2>
        <div class="control-buttons">
          <div class="control-card">
            <h3>Компрессор</h3>
            <button class="control-btn" id="compressorBtn" onclick="toggleCompressor()">
              ВЫКЛ
            </button>
          </div>
          <div class="control-card">
            <h3>Вентилятор</h3>
            <button class="control-btn" id="fanBtn" onclick="toggleFan()">
              ВЫКЛ
            </button>
          </div>
        </div>
      </section>
      
      <!-- Статус системы -->
      <section class="status-info">
        <h2>Статус</h2>
        <div class="status-grid">
          <div class="status-item">
            <span>Компрессор:</span>
            <span id="statusCompressor" class="status-badge off">ВЫКЛ</span>
          </div>
          <div class="status-item">
            <span>Вентилятор:</span>
            <span id="statusFan" class="status-badge off">ВЫКЛ</span>
          </div>
          <div class="status-item">
            <span>Режим:</span>
            <span id="statusMode" class="status-badge">Ручной</span>
          </div>
        </div>
      </section>
    </main>
    
    <footer>
      <p>Moonshine Cooler v1.0 | Smart Cooling System</p>
    </footer>
  </div>
  
  <script src="/js/app.js"></script>
</body>
</html>
)rawliteral";
}

String getCSSContent() {
  return R"rawliteral(
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  min-height: 100vh;
  padding: 20px;
  color: #333;
}

.container {
  max-width: 800px;
  margin: 0 auto;
  background: white;
  border-radius: 15px;
  box-shadow: 0 20px 60px rgba(0,0,0,0.3);
  overflow: hidden;
}

header {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  padding: 30px;
  text-align: center;
}

header h1 {
  font-size: 2.5em;
  margin-bottom: 10px;
}

header p {
  font-size: 1.1em;
  opacity: 0.9;
}

main {
  padding: 30px;
}

section {
  margin-bottom: 30px;
}

h2 {
  color: #667eea;
  margin-bottom: 20px;
  font-size: 1.5em;
  border-bottom: 2px solid #f0f0f0;
  padding-bottom: 10px;
}

/* Температура */
.temperature-display {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 20px;
  margin-bottom: 20px;
}

.temp-card {
  background: #f8f9fa;
  padding: 20px;
  border-radius: 10px;
  text-align: center;
  border: 2px solid #e9ecef;
}

.temp-card label {
  display: block;
  color: #666;
  margin-bottom: 10px;
  font-weight: 600;
}

.temp-value {
  font-size: 2.5em;
  color: #667eea;
  font-weight: bold;
}

/* Режимы */
.mode-buttons {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.mode-btn {
  padding: 15px;
  border: 2px solid #ddd;
  background: white;
  border-radius: 8px;
  cursor: pointer;
  font-size: 1.1em;
  font-weight: 600;
  transition: all 0.3s;
}

.mode-btn:hover {
  border-color: #667eea;
  background: #f0f0f0;
}

.mode-btn.active {
  background: #667eea;
  color: white;
  border-color: #667eea;
}

/* Автоматический режим */
.auto-section {
  background: #f0f7ff;
  padding: 20px;
  border-radius: 10px;
  border-left: 4px solid #667eea;
}

.control-group {
  display: grid;
  gap: 10px;
  margin-bottom: 15px;
}

.control-group label {
  font-weight: 600;
  color: #333;
}

.control-group input {
  padding: 10px;
  border: 2px solid #ddd;
  border-radius: 6px;
  font-size: 1em;
}

.control-group button {
  padding: 10px 20px;
  background: #667eea;
  color: white;
  border: none;
  border-radius: 6px;
  cursor: pointer;
  font-weight: 600;
  transition: background 0.3s;
}

.control-group button:hover {
  background: #764ba2;
}

.info-box {
  background: white;
  padding: 15px;
  border-radius: 6px;
  margin-top: 10px;
  border-left: 4px solid #667eea;
}

/* Ручной режим */
.control-buttons {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 20px;
}

.control-card {
  background: #f8f9fa;
  padding: 20px;
  border-radius: 10px;
  text-align: center;
}

.control-card h3 {
  color: #333;
  margin-bottom: 15px;
  font-size: 1.2em;
}

.control-btn {
  width: 100%;
  padding: 20px;
  border: 2px solid #ddd;
  background: white;
  border-radius: 8px;
  cursor: pointer;
  font-size: 1.2em;
  font-weight: bold;
  transition: all 0.3s;
}

.control-btn.on {
  background: #28a745;
  color: white;
  border-color: #28a745;
}

.control-btn.off {
  background: #dc3545;
  color: white;
  border-color: #dc3545;
}

/* Статус */
.status-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 15px;
}

.status-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 15px;
  background: #f8f9fa;
  border-radius: 8px;
  border: 1px solid #e9ecef;
}

.status-item span:first-child {
  font-weight: 600;
  color: #333;
}

.status-badge {
  padding: 5px 15px;
  border-radius: 20px;
  font-weight: 600;
  font-size: 0.9em;
}

.status-badge.on {
  background: #d4edda;
  color: #155724;
}

.status-badge.off {
  background: #f8d7da;
  color: #721c24;
}

footer {
  background: #f8f9fa;
  padding: 20px;
  text-align: center;
  color: #666;
  border-top: 1px solid #e9ecef;
}

/* Адаптивный дизайн */
@media (max-width: 600px) {
  header h1 {
    font-size: 2em;
  }
  
  .temperature-display {
    grid-template-columns: 1fr;
  }
  
  .control-buttons {
    grid-template-columns: 1fr;
  }
  
  main {
    padding: 20px;
  }
}
)rawliteral";
}

String getJSContent() {
  return R"rawliteral(
const API_BASE = '';
let autoMode = false;

// Инициализация
document.addEventListener('DOMContentLoaded', () => {
  updateStatus();
  setInterval(updateStatus, 1000);
});

// Получить статус
async function updateStatus() {
  try {
    const response = await fetch(`${API_BASE}/api/status`);
    const data = await response.json();
    
    document.getElementById('tempLiquid').textContent = data.temp_liquid.toFixed(1) + '°C';
    document.getElementById('tempVapor').textContent = data.temp_vapor.toFixed(1) + '°C';
    
    autoMode = data.auto_mode;
    updateUIMode();
    
    if (data.compressor_on) {
      document.getElementById('compressorBtn').classList.add('on');
      document.getElementById('compressorBtn').textContent = 'ВКЛ';
      document.getElementById('statusCompressor').textContent = 'ВКЛ';
      document.getElementById('statusCompressor').classList.remove('off');
      document.getElementById('statusCompressor').classList.add('on');
    } else {
      document.getElementById('compressorBtn').classList.remove('on');
      document.getElementById('compressorBtn').textContent = 'ВЫКЛ';
      document.getElementById('statusCompressor').textContent = 'ВЫКЛ';
      document.getElementById('statusCompressor').classList.add('off');
      document.getElementById('statusCompressor').classList.remove('on');
    }
    
    if (data.fan_on) {
      document.getElementById('fanBtn').classList.add('on');
      document.getElementById('fanBtn').textContent = 'ВКЛ';
      document.getElementById('statusFan').textContent = 'ВКЛ';
      document.getElementById('statusFan').classList.remove('off');
      document.getElementById('statusFan').classList.add('on');
    } else {
      document.getElementById('fanBtn').classList.remove('on');
      document.getElementById('fanBtn').textContent = 'ВЫКЛ';
      document.getElementById('statusFan').textContent = 'ВЫКЛ';
      document.getElementById('statusFan').classList.add('off');
      document.getElementById('statusFan').classList.remove('on');
    }
  } catch (error) {
    console.error('Error updating status:', error);
  }
}

// Переключить режим
function setMode(auto) {
  fetch(`${API_BASE}/api/mode`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ auto: auto })
  }).then(() => updateStatus());
}

// Обновить UI режима
function updateUIMode() {
  const autoBtn = document.getElementById('autoBtn');
  const manualBtn = document.getElementById('manualBtn');
  const autoSection = document.getElementById('autoSection');
  const manualSection = document.getElementById('manualSection');
  const statusMode = document.getElementById('statusMode');
  
  if (autoMode) {
    autoBtn.classList.add('active');
    manualBtn.classList.remove('active');
    autoSection.style.display = 'block';
    manualSection.style.display = 'none';
    statusMode.textContent = 'Автомат';
  } else {
    manualBtn.classList.add('active');
    autoBtn.classList.remove('active');
    autoSection.style.display = 'none';
    manualSection.style.display = 'block';
    statusMode.textContent = 'Ручной';
  }
}

// Управление компрессором
async function toggleCompressor() {
  const isOn = document.getElementById('compressorBtn').classList.contains('on');
  await fetch(`${API_BASE}/api/compressor`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ on: !isOn })
  });
  updateStatus();
}

// Управление вентилятором
async function toggleFan() {
  const isOn = document.getElementById('fanBtn').classList.contains('on');
  await fetch(`${API_BASE}/api/fan`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ on: !isOn })
  });
  updateStatus();
}

// Установить целевую температуру
async function setTargetTemperature() {
  const temp = parseFloat(document.getElementById('targetTemp').value);
  if (temp >= 0 && temp <= 100) {
    await fetch(`${API_BASE}/api/temperature`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ target_temp: temp })
    });
    updateStatus();
  }
}
)rawliteral";
}
