#include <WiFi.h>
#include <WebServer.h>
#include "sensor.h"
#include "led.h"
#include <ThingSpeak.h>

// Настройки WiFi
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Пины датчиков температуры воды
#define FIRST_WATER_SENSOR 23         // Пин первого
#define SECOND_WATER_SENSOR 22        // Пин второго
#define THIRD_WATER_SENSOR 19         // Пин третьего
// Пины датчиков температуры масла
//#define FIRST_OIL_SENSOR 3          // Пин первого
//#define SECOND_OIL_SENSOR 22        // Пин второго
//#define THIRD_OIL_SENSOR 22         // Пин третьего

// Пины светодиодов воды
#define WARNING_WATER_LED 17    // Предупреждение
#define ALARM_WATER_LED 5       // Тревога
// Пины светодиодов масла
//#define WARNING_OIL_LED 4     // Предупреждение
//#define ALARM_OIL_LED 15      // Тревога

WebServer server(80);

// Переменные для отображения
float temperatureWater = 0.0;
unsigned long uptime = 0;
bool systemStatus = true;
String lastUpdate = "00:00:00";

// HTML страница с автоматическим обновлением
const char* htmlPage = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Monitoring ESP32</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      max-width: 800px;
      margin: 0 auto;
      padding: 20px;
      background-color: #f5f5f5;
    }
    .container {
      background-color: white;
      border-radius: 10px;
      padding: 20px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
    }
    .data-card {
      background-color: #f0f8ff;
      border-left: 5px solid #4CAF50;
      padding: 15px;
      margin-bottom: 15px;
      border-radius: 5px;
    }
    .status-badge {
      display: inline-block;
      padding: 5px 10px;
      border-radius: 20px;
      color: white;
      font-weight: bold;
    }
    .online { background-color: #4CAF50; }
    .offline { background-color: #f44336; }
    h1 { color: #333; }
    .value {
      font-size: 24px;
      font-weight: bold;
      color: #2196F3;
    }
    .unit {
      font-size: 16px;
      color: #757575;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Monitoring ESP32</h1>
    
    <div class="data-card">
      <h2>System Information</h2>
      <p>Status: <span class="status-badge %STATUSCLASS%">%STATUS%</span></p>
      <p>Time of work: <span class="value">%UPTIME%</span> <span class="unit">seconds</span></p>
      <p>Last update: <span class="value">%LASTUPDATE%</span></p>
    </div>
    
    <div class="data-card">
      <h2>Sensors</h2>
      <p>Temperature of water: <span class="value">%TEMPERATUREWATER%</span> <span class="unit">°C</span></p>
    </div>
    
    <div class="data-card">
      <h2>Networks parameters</h2>
      <p>IP address: %IPADDRESS%</p>
      <p>MAC address: %MACADDRESS%</p>
      <p>Signal WiFi: %WIFISIGNAL% <span class="unit">dBm</span></p>
    </div>
  </div>
  
  <script>
    // Автоматическое обновление каждые 3 секунды
    setInterval(function() {
      fetch('/data')
        .then(response => response.json())
        .then(data => {
          // Обновляем все элементы на странице
          document.querySelector('.status-badge').className = 
            'status-badge ' + (data.systemStatus ? 'online' : 'offline');
          document.querySelector('.status-badge').textContent = 
            data.systemStatus ? 'ONLINE' : 'OFFLINE';
          document.querySelectorAll('.value')[0].textContent = data.uptime;
          document.querySelectorAll('.value')[1].textContent = data.lastUpdate;
          document.querySelectorAll('.value')[2].textContent = data.temperatureWater;
          document.getElementById('wifiSignal').textContent = data.wifiSignal;
        });
    }, 3000);
  </script>
</body>
</html>
)rawliteral";

// ThingSpeak information
char thingSpeakAddress[] = "api.thingspeak.com";
unsigned long channelID = "YOUR_CHANNEL_ID";
char readAPIKey[] = "YOUR_THINGSPEAK_READAPIKEY";
char writeAPIKey[] = "YOUR_THINGSPEAK_WRITEAPIKEY";
const unsigned long postingInterval = 180L * 1000L;
unsigned int dataFieldOne = 1;                            // Field to write temperature data
unsigned int dataFieldTwo = 2;                            // Field to write temperature data
unsigned int dataFieldThree = 3;                          // Field to write elapsed time data
long lastUpdateThingSpeak = 0;
WiFiClient client;

/*int write2TSData( long TSChannel, unsigned int TSField1, float field1Data, unsigned int TSField2, long field2Data, unsigned int TSField3, long field3Data ) {

  ThingSpeak.setField( TSField1, field1Data );
  ThingSpeak.setField( TSField2, field2Data );
  ThingSpeak.setField( TSField3, field3Data );

  int writeSuccess = ThingSpeak.writeFields( TSChannel, writeAPIKey );
  return writeSuccess;
}*/
int write2TSData( long TSChannel, unsigned int TSField1, float field1Data, unsigned int TSField2, long field2Data ) {

  ThingSpeak.setField( TSField1, field1Data );
  ThingSpeak.setField( TSField2, field2Data );

  int writeSuccess = ThingSpeak.writeFields( TSChannel, writeAPIKey );
  return writeSuccess;
}

const int waterPins[] = {FIRST_WATER_SENSOR, SECOND_WATER_SENSOR, THIRD_WATER_SENSOR};
//const int oilPins[] = {FIRST_OIL_SENSOR, SECOND_OIL_SENSOR, THIRD_OIL_SENSOR};
TempSensors waterSensors(
  waterPins,
  WARNING_THRESHOLD_WATER,
  ALARM_THRESHOLD_WATER
);

/*TempSensors oilSensors(
  oilPins,
  WARNING_THRESHOLD_OIL,
  ALARM_THRESHOLD_OIL
  );*/

TempLEDs waterLEDs(WARNING_WATER_LED, ALARM_WATER_LED);
//TempLEDs oilLEDs(WARNING_OIL_LED, ALARM_OIL_LED);

void setup() {
  Serial.begin(9600);
  // Подключаемся к WiFi
  WiFi.begin(ssid, password);
  Serial.print("Подключение к WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());

  // Настраиваем маршруты
  server.on("/", handleRoot);
  server.on("/data", handleData);
  
  server.begin();
  Serial.println("HTTP сервер запущен");


  waterSensors.startSensors();
  //oilSensors.startSensors();

  ThingSpeak.begin( client );
}

void handleRoot() {
  float currentTemp = waterSensors.averTemp; // Получаем текущую температуру
  String page = htmlPage;
  
  page.replace("%STATUS%", systemStatus ? "ONLINE" : "OFFLINE");
  page.replace("%STATUSCLASS%", systemStatus ? "online" : "offline");
  page.replace("%UPTIME%", String(uptime));
  page.replace("%LASTUPDATE%", lastUpdate);
  page.replace("%TEMPERATUREWATER%", String(currentTemp));
  page.replace("%IPADDRESS%", WiFi.localIP().toString());
  page.replace("%MACADDRESS%", WiFi.macAddress());
  page.replace("%WIFISIGNAL%", String(WiFi.RSSI()));
  
  server.send(200, "text/html", page);
}

void handleData() {
  String json = "{";
  json += "\"temperatureWater\":" + String(waterSensors.averTemp) + ",";
  json += "\"uptime\":" + String(uptime) + ",";
  json += "\"systemStatus\":" + String(systemStatus ? "true" : "false") + ",";
  json += "\"lastUpdate\":\"" + lastUpdate + "\",";
  json += "\"wifiSignal\":" + String(WiFi.RSSI());
  json += "}";
  
  server.send(200, "application/json", json);
}

void updateVariables(float temperature) {
  // Здесь можно обновлять значения переменных
  uptime = millis() / 1000;
  
  // Пример: имитация изменения значений датчиков
  temperatureWater = temperature;
  
  // Обновляем время последнего обновления
  unsigned long seconds = uptime % 60;
  unsigned long minutes = (uptime / 60) % 60;
  unsigned long hours = (uptime / 3600);
  lastUpdate = String(hours) + ":" + String(minutes) + ":" + String(seconds);
}

void loop() {
  server.handleClient();
  

  waterSensors.updateTempratures();
  //oilSensors.updateTempratures();

  waterSensors.updateBroken();
  //oilSensors.updateBroken();

  waterSensors.getAverage();

  //oilSensors.getAverage();

  waterLEDs.changeMod( waterSensors.checkWarning() );
  //oilLEDs.changeMod( oilSensors.checkWarning() );

  static unsigned long lastUpdateTime = 0;
  if (millis() - lastUpdateTime >= 1000) {
    updateVariables(waterSensors.averTemp);
    lastUpdateTime = millis();
  }

  // Update only if the posting time is exceeded
  if (millis() - lastUpdateThingSpeak >=  postingInterval) {
    lastUpdateThingSpeak = millis();
    Serial.println("Temp Water= " + String( waterSensors.averTemp ));
    //write2TSData( channelID , dataFieldOne , waterSensors.averTemp , dataFieldTwo , oilSensors.averTemp , dataFieldThree , millis() );
    write2TSData( channelID , dataFieldOne , waterSensors.averTemp, dataFieldThree , millis() );
  }
}
