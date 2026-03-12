#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include <time.h>

// ==========================================
// NETWORK & CONFIGURATION
// ==========================================
#if __has_include("config.h")
  #include "config.h"
#endif

// NTP Server Settings
const long gmtOffset_sec = 7200;      // (UTC+2) = 2 hours * 3600 seconds
const int  daylightOffset_sec = 0;    // No Daylight Savings in SA
const char* ntpServer = "pool.ntp.org";

// MQTT Client Setup
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMqttReport = 0;

// Web Server
WebServer server(80);

// ==========================================
// PIN DEFINITIONS
// ==========================================
// Sensors
const int numSensors = 4;
const int sensorPins[] = {22, 19, 18, 4}; // Pin 5 defaults to 1 when disconnected, removed

// Buttons
const int numButtons = 4;
const int buttonPins[] = {25, 26, 27, 32}; 

// Status LED
const int statusLedPin = 13;

// Motor Pins
const int motorIn1 = 23;        
const int motorIn2 = 21;        
const int motorEnable = 15;

// ==========================================
// SYSTEM STATE VARIABLES
// ==========================================
// Motor Properties
bool forwardDirection = false;       // Set to true/false for reverse
bool motorRunning = false;
String lastTrigger = "None";

// PWM Settings
const int freq = 20000;
const int pwmChannel = 0;
const int pwmTimer = 0; 
const int resolution = 10;
int dutyCycle = 1023;

// Ramp Engine Variables
int targetPWM = 0;                   // Target motor speed
int actualPWM = 0;                   // Actual current motor speed
unsigned long lastRampTime = 0; 
const int rampInterval = 10;         // Milliseconds between speed steps

// Time & Session Tracking
unsigned long motorStartTime = 0;
String currentSessionStr = "00:00:00";
String previousSessionStr = "00:00:00";
String lastMotorActivation = "Never";

// Component Tracking
String lastSensorTrigger[numSensors];
String lastButtonTrigger[numButtons];
bool lastBtnStates[numButtons] = {HIGH, HIGH, HIGH, HIGH};

// LED State Tracking
unsigned long lastLedBlink = 0;
bool ledState = false;

// ==========================================
// SETUP ROUTINE
// ==========================================
void setup() {
  Serial.begin(115200);

  // Initialize Sensors
  for (int i = 0; i < numSensors; i++) {
    pinMode(sensorPins[i], INPUT);
  }

  // Initialize Motor Pins
  pinMode(motorIn1, OUTPUT); 
  pinMode(motorIn2, OUTPUT); 
  pinMode(motorEnable, OUTPUT);

  // Configure Motor PWM
  ledcAttachChannel(motorEnable, freq, resolution, pwmChannel);
  ledcWrite(motorEnable, 0);
  stopMotor(); // Ensure motor is off on boot
  
  // Initialize Buttons
  for (int i = 0; i < numButtons; i++)  {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastBtnStates[i] = digitalRead(buttonPins[i]); // Record initial state
  }

  // Initialize Status LED (Start as OFF)
  pinMode(statusLedPin, OUTPUT);
  digitalWrite(statusLedPin, HIGH);

  // Connect to Wi-Fi with blinking LED indicator
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    digitalWrite(statusLedPin, LOW);  // Turn LED ON
    delay(250);
    digitalWrite(statusLedPin, HIGH); // Turn LED OFF
    delay(250);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  // Initialize NTP Time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)){
    Serial.println("Waiting for NTP time sync...");
  }
  Serial.println("NTP time synced!");

  // Setup MQTT & Web Server
  client.setServer(mqtt_server, 1883);
  server.on("/", handleRoot);
  server.on("/data", handleJSON);
  server.begin();
  MDNS.begin("latte-lab");
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  server.handleClient();

  // Maintain MQTT Connection
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // System is online: ensure LED is ON
  digitalWrite(statusLedPin, LOW);

  // --- NON-BLOCKING PWM RAMP ENGINE ---
  if (millis() - lastRampTime >= rampInterval) {
    lastRampTime = millis();

    if (actualPWM < targetPWM) {
      if (actualPWM < 400 && targetPWM > 0) {
        actualPWM = 400; // Jump instantly to movement threshold
      } else {
        actualPWM += 5;  // Increase speed gradually
      }
      if (actualPWM > targetPWM) actualPWM = targetPWM;
    } 
    else if (actualPWM > targetPWM) {
      actualPWM -= 10;   // Decrease speed gradually
      if (actualPWM < targetPWM) actualPWM = targetPWM;
    }

    ledcWrite(motorEnable, actualPWM);

    if (actualPWM == 0) {
      digitalWrite(motorIn1, LOW);
      digitalWrite(motorIn2, LOW);
    }
  }

  // --- MASTER BUTTON LOGIC (Button 0 / Pin 25) ---
  bool mainBtnActive = (digitalRead(buttonPins[0]) == LOW); 

  // Detect state change on Master Button
  if (mainBtnActive != (lastBtnStates[0] == LOW)) {
    if (mainBtnActive) {
      // Button Pushed IN -> Start Motor
      lastButtonTrigger[0] = getTimestamp();
      startMotor("Button 0");
    } else {
      // Button Popped OUT -> Emergency Stop
      stopMotor();
      lastTrigger = "MASTER STOP";
      sendMqttUpdate(); // Force immediate dashboard update
    }
    lastBtnStates[0] = mainBtnActive ? LOW : HIGH;
  }

  // --- REGULAR BUTTON LOGIC (Buttons 1 to 3) ---
  for (int i = 1; i < numButtons; i++) {
    bool currentState = digitalRead(buttonPins[i]);
    
    if (currentState != lastBtnStates[i]) {
      // Only trigger if pressed AND Master Button is IN
      if (mainBtnActive == true) { 
        lastButtonTrigger[i] = getTimestamp();
        startMotor("Button " + String(i));
      }
      lastBtnStates[i] = currentState;
    }
  }

  // --- MOTION SENSOR LOGIC ---
  for (int i = 0; i < numSensors; i++) {
    // Only allow sensors to trigger if Master Button is IN
    if (digitalRead(sensorPins[i]) == HIGH && mainBtnActive == true) {
      lastSensorTrigger[i] = getTimestamp();
      
      if (motorRunning) {
        motorRunning = false;
        lastTrigger = "Sensor " + String(i + 1);
        previousSessionStr = formatTime(millis() - motorStartTime);
        currentSessionStr = "00:00:00";
        sendMqttUpdate(); 
      }
    }
  }

  // --- MOTOR STATE TRACKING ---
  if (motorRunning) {
    runMotor();
    currentSessionStr = formatTime(millis() - motorStartTime);
  } else {
    stopMotor();
  }

  // --- PERIODIC MQTT UPDATE (1 Second) ---
  if (millis() - lastMqttReport > 1000) {
    sendMqttUpdate();
    lastMqttReport = millis();
  }

  delay(20); 
}

// ==========================================
// MOTOR CONTROL FUNCTIONS
// ==========================================
void startMotor(String source) {
  if (!motorRunning) {
    motorRunning = true;
    lastTrigger = source;
    motorStartTime = millis();
    lastMotorActivation = getTimestamp();
    targetPWM = 1023; // Set goal to full speed

    if (forwardDirection) {
      digitalWrite(motorIn1, HIGH);
      digitalWrite(motorIn2, LOW);
    } else {
      digitalWrite(motorIn1, LOW);
      digitalWrite(motorIn2, HIGH);
    }
  }
}

void runMotor() { 
  targetPWM = 1023; 

  if (forwardDirection) {
    digitalWrite(motorIn1, HIGH);
    digitalWrite(motorIn2, LOW);
  } else {
    digitalWrite(motorIn1, LOW);
    digitalWrite(motorIn2, HIGH);
  }
}

void stopMotor() {
  motorRunning = false;
  targetPWM = 0; 
}

// ==========================================
// MQTT & NETWORK FUNCTIONS
// ==========================================
void sendMqttUpdate() {
  // Master Motor Update
  StaticJsonDocument<512> doc; 
  doc["motor_active"] = motorRunning;
  doc["last_trigger"] = lastTrigger;
  doc["last_activated"] = lastMotorActivation;
  doc["current_session_time"] = currentSessionStr;
  doc["previous_session_time"] = previousSessionStr;
  doc["total_uptime"] = formatTime(millis());
  
  char buffer[512];
  serializeJson(doc, buffer);
  client.publish("factory/motor1", buffer);
  
  // Individual Button Topics
  for(int i = 0; i < numButtons; i++) {
    StaticJsonDocument<128> btnDoc;
    String topic = "factory/button" + String(i);
    
    btnDoc["status"] = (digitalRead(buttonPins[i]) == LOW) ? "ON" : "OFF";
    btnDoc["last_activated"] = lastButtonTrigger[i]; 
    
    char btnBuffer[128];
    serializeJson(btnDoc, btnBuffer);
    client.publish(topic.c_str(), btnBuffer);
  }

  // Individual Sensor Topics
  for(int i = 0; i < numSensors; i++) {
    StaticJsonDocument<128> snrDoc;
    String topic = "factory/sensor" + String(i+1);
    
    snrDoc["status"] = (digitalRead(sensorPins[i]) == HIGH) ? "MOTION" : "CLEAR";
    snrDoc["last_activated"] = lastSensorTrigger[i]; 
    
    char snrBuffer[128];
    serializeJson(snrDoc, snrBuffer);
    client.publish(topic.c_str(), snrBuffer);
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    
    if (client.connect("ESP32_Motor_Unit")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());

      // Fast blink 4 times (equals 2000ms delay)
      for (int i = 0; i < 4; i++) {
        digitalWrite(statusLedPin, LOW);  // ON
        delay(250);
        digitalWrite(statusLedPin, HIGH); // OFF
        delay(250);
      }
    }
  }
}

// ==========================================
// HELPER & WEB DASHBOARD FUNCTIONS
// ==========================================
String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)){
    return "Time Error";
  }
  char timeStringBuff[20]; 
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

String formatTime(unsigned long totalMillis) {
  unsigned long totalSeconds = totalMillis / 1000;
  int seconds = totalSeconds % 60;
  int minutes = (totalSeconds / 60) % 60;
  int hours = (totalSeconds / 3600);
  
  char buffer[15];
  sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
  return String(buffer);
}

void handleJSON() {
  StaticJsonDocument<512> doc; 
  doc["motor_active"] = motorRunning;
  doc["last_trigger"] = lastTrigger;
  doc["last_activated"] = lastMotorActivation; 
  doc["current_session_time"] = currentSessionStr;
  doc["previous_session_time"] = previousSessionStr;
  doc["total_uptime"] = formatTime(millis());
  doc["sensor_status"] = digitalRead(sensorPins[0]) == HIGH ? "MOTION" : "CLEAR";

  String buf;
  serializeJson(doc, buf);
  server.send(200, "application/json", buf);
}

void handleRoot() {
  String html = "<html><head><title>Latte-Lab Dashboard</title>";
  html += "<style>body{font-family:sans-serif; text-align:center; background:#f4f4f4; padding-top:20px;}";
  html += ".card{background:white; padding:20px; border-radius:10px; display:inline-block; box-shadow:0 4px 8px rgba(0,0,0,0.1); width:420px;}";
  html += ".timer-main{font-size:48px; font-weight:bold; color:#2c3e50; margin:10px 0;}";
  html += ".timer-prev{font-size:18px; color:#7f8c8d; margin-bottom:20px;}";
  html += ".status-box{padding:10px; border-radius:5px; color:white; font-weight:bold;}</style></head>";
  
  html += "<body><div class='card'><h1>Latte-Lab Control</h1>";
  html += "<p>Status: <span id='mBox' class='status-box' style='background:#95a5a6;'>...</span></p>";
  
  html += "<div class='timer-main' id='runClock'>00:000</div>";
  html += "<div class='timer-prev'>Previous Run: <span id='prevClock'>00:000</span></div>";
  
  html += "<p>Trigger: <span id='trig' style='color:#e67e22;'>-</span></p>";
  html += "<p> API: <a href='/data' style='color:#3498db;'>/data</a></p>";
  
  html += "<script>setInterval(function() {";
  html += "  fetch('/data').then(r => r.json()).then(d => {";
  html += "    document.getElementById('mBox').innerHTML = d.motor_active ? 'RUNNING' : 'STOPPED';";
  html += "    document.getElementById('mBox').style.background = d.motor_active ? '#2ecc71' : '#e74c3c';";
  html += "    document.getElementById('runClock').innerHTML = d.current_session_time;";
  html += "    document.getElementById('prevClock').innerHTML = d.previous_session_time;";
  html += "    document.getElementById('trig').innerHTML = d.last_trigger;";
  html += "  });";
  html += "}, 100);</script></body></html>"; 

  server.send(200, "text/html", html);
}