#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
// #include <Arduino_OPC_UA.h>
#include <PubSubClient.h>
#include <time.h>

// Network Credentials
#if __has_include("config.h")
  #include "config.h"
#endif

// Pin Arrays
// const int sensorPins[] = {22, 19, 18, 5, 4}; // 5 Sensors
// const int numSensors = 5;
const int sensorPins[] = {22, 19, 18, 4}; // 4 Sensors (Pin 5 causes problems because it defaults to 1 when not connected)
const int numSensors = 4;
const int buttonPins[] = {25, 26, 27, 32};      // 3 Buttons
const int numButtons = 4;

// Motor Pins
const int motorIn1 = 23;        
const int motorIn2 = 21;        
const int motorEnable = 15;

// Setting PWM properties
const int freq = 20000;
const int pwmChannel = 0;
const int pwmTimer = 0; 
const int resolution = 10;
// int dutyCycle = 225;
int dutyCycle = 1023;

int targetPWM = 0;      // What speed we WANT to be at
int actualPWM = 0;      // What speed the motor is ACTUALLY at right now
unsigned long lastRampTime = 0; 
const int rampInterval = 10; // How many milliseconds between each speed step


// State Variables
bool motorRunning = false;
String lastTrigger = "None";

// Time Tracking Variables
unsigned long motorStartTime = 0;
// unsigned long totalMotorRuntime = 0; // Accumulator for total time
// unsigned long currentSessionTime = 0; // Time for the current run
String currentSessionStr = "00:00:00";
String previousSessionStr = "00:00:00";

// Timestamps (Last Activation)
// unsigned long lastSensorTrigger[numSensors] = {0};
// unsigned long lastButtonTrigger[numButtons] = {0};
// unsigned long lastMotorActivation = 0;

// Timestamps of NTP Time
String lastSensorTrigger[numSensors];
String lastButtonTrigger[numButtons];
String lastMotorActivation = "Never";

const long gmtOffset_sec = 7200;      // (UTC+2) = 2 hours * 3600 seconds
const int  daylightOffset_sec = 0;    // (3600 with Daylight saving, 0 without Daylight saving) No Daylight Savings in SA
const char* ntpServer = "pool.ntp.org";

// MQTT Client Setup
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMqttReport = 0;

// Button tracking
bool lastBtnStates[numButtons] = {HIGH, HIGH, HIGH, HIGH};

WebServer server(80);

void setup() {
  Serial.begin(115200);

  // Initialize Sensors
  for(int i=0; i<numSensors; i++) {
    pinMode(sensorPins[i], INPUT);
  }

  // Motor Pins
  pinMode(motorIn1, OUTPUT); 
  pinMode(motorIn2, OUTPUT); 
  pinMode(motorEnable, OUTPUT);

  // Configure LED PWM functionalities
  ledcAttachChannel(motorEnable, freq, resolution, pwmChannel);
  ledcWrite(motorEnable, 0);
  
  // Initialize Buttons
  for(int i=0; i<numButtons; i++)  {
    pinMode(buttonPins[i], INPUT_PULLUP);

    // Set last state to CURRENT state so no "change" is detected on boot
    lastBtnStates[i] = digitalRead(buttonPins[i]);
  }
  // Motor initially off
  stopMotor();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  // Initialize NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Wait for time to sync (optional but helpful)
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Waiting for NTP time sync...");
  }
  Serial.println("NTP time synced!");

  // MQTT Config
  client.setServer(mqtt_server, 1883);
  
  // Setup Server
  server.on("/", handleRoot);
  server.on("/data", handleJSON);
  server.begin();
  MDNS.begin("latte-lab");
}



void loop() {
  server.handleClient();

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Changing the speed of the motor
  // NON-BLOCKING RAMP ENGINE
  if (millis() - lastRampTime >= rampInterval) {
    lastRampTime = millis();

    if (actualPWM < targetPWM) {
      // Increase speed
      if (actualPWM < 400 && targetPWM > 0) {
        actualPWM = 400; // Jump instantly to the "movement" threshold
      } else {
        actualPWM += 5;
      }
      if (actualPWM > targetPWM) {
        actualPWM = targetPWM;
      }
    } 
    else if (actualPWM > targetPWM) {
      actualPWM -= 10; // Decrease speed (stops slightly faster than it starts)
      if (actualPWM < targetPWM) actualPWM = targetPWM;
    }

    // Apply the speed to the motor
    ledcWrite(motorEnable, actualPWM);

    // If we reached 0, shut off the direction pins completely
    if (actualPWM == 0) {
      digitalWrite(motorIn1, LOW);
      digitalWrite(motorIn2, LOW);
    }
  }
  // END RAMP ENGINE

  // Button Logic 
  for (int i = 0; i < numButtons; i++) {
    bool currentState = digitalRead(buttonPins[i]);
    // if ((lastBtnStates[i] == HIGH && currentState == LOW) || (lastBtnStates[i] == LOW && currentState == HIGH)) {
    if (currentState != lastBtnStates[i]) {
      lastButtonTrigger[i] = getTimestamp();
      startMotor("Button " + String(i));
      lastBtnStates[i] = currentState;
    }
  }

  // Motion Logic 
  for (int i = 0; i < numSensors; i++) {
    if (digitalRead(sensorPins[i]) == HIGH) {
      lastSensorTrigger[i] = getTimestamp();
      
      // If motor is running, this sensor stops it
      if (motorRunning) {
        motorRunning = false;
        lastTrigger = "Sensor " + String(i + 1);
        previousSessionStr = formatTime(millis() - motorStartTime);
        currentSessionStr = "00:00:00";
        sendMqttUpdate(); // Report immediately
      }
    }
  }

  if (motorRunning) {
    runMotor();
    // startMotor();
    unsigned long elapsed = millis() - motorStartTime;
    currentSessionStr = formatTime(elapsed);
  } else {
    stopMotor();
  }

  // PERIODIC MQTT UPDATE (Every 1 second)
  if (millis() - lastMqttReport > 1000) {
    sendMqttUpdate();
    lastMqttReport = millis();
  }

  delay(20); 
}

void startMotor(String source) {
  if (!motorRunning) {
    motorRunning = true;
    lastTrigger = source;
    motorStartTime = millis();
    lastMotorActivation = getTimestamp();

    targetPWM = 1023; // Set the goal to full speed
    // Set direction pins immediately
    digitalWrite(motorIn1, HIGH);
    digitalWrite(motorIn2, LOW);
  }
}


void runMotor() { 
  // digitalWrite(motorIn1, HIGH);
  // digitalWrite(motorIn2, LOW);

  // // Get current duty - if it's already at max, don't do anything
  // // We use a static variable or a global to track the current speed
  // static int currentPWM = 0;

  // if (currentPWM < 1023) {
  //   // Ramp up quickly but smoothly
  //   for (int i = currentPWM; i <= 1023; i += 5) {
  //       Serial.print("Speed: ");
  //       Serial.println(i);
  //       ledcWrite(motorEnable, i);
  //       currentPWM = i;
  //       delay(20); // Total ramp time approx 0.4 seconds

  //   }
  // }

    targetPWM = 1023; // Set the goal to full speed
    // Set direction pins immediately
    digitalWrite(motorIn1, HIGH);
    digitalWrite(motorIn2, LOW);



}

void stopMotor() {
  // // Ramp down
  // for (int i = 1023; i >= 0; i -= 10) {
  //     Serial.print("Speed: ");
  //     Serial.println(i);
  //     ledcWrite(motorEnable, i);
  //     delay(20);
  // }
  
  // // Completely cut power and pins
  // ledcWrite(motorEnable, 0);
  // digitalWrite(motorIn1, LOW);
  // digitalWrite(motorIn2, LOW);


  motorRunning = false;
  targetPWM = 0; // Set the goal to zero
  // We don't turn off In1/In2 yet; we wait for the ramp to hit 0
}

// void runMotor() { 
//   digitalWrite(motorIn1, HIGH);
//   digitalWrite(motorIn2, LOW);
//   analogWrite(motorEnable, 255);
// }

// void stopMotor() {
//   digitalWrite(motorIn1, LOW);
//   digitalWrite(motorIn2, LOW);
//   analogWrite(motorEnable, 0);
// }


void sendMqttUpdate() {
  // Master Motor Update
  StaticJsonDocument<512> doc; 
  doc["motor_active"] = motorRunning;
  doc["last_trigger"] = lastTrigger;
  // doc["last_motor_activation"] = formatTime(lastMotorActivation); 
  doc["last_activated"] = lastMotorActivation;
  // doc["last_activated_date"] = getDateStamp();

  // doc["current_run_duration_sec"] = currentSessionTime;
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
    // btnDoc["last_activated"] = formatTime(lastButtonTrigger[i]); // Formatted string
    btnDoc["last_activated"] = lastButtonTrigger[i]; // Formatted string
    // btnDoc["last_activated_date"] = getDateStamp();
    
    char btnBuffer[128];
    serializeJson(btnDoc, btnBuffer);
    client.publish(topic.c_str(), btnBuffer);
  }

  // Individual Sensor Topics
  for(int i = 0; i < numSensors; i++) {
    StaticJsonDocument<128> snrDoc;
    String topic = "factory/sensor" + String(i+1);
    
    snrDoc["status"] = (digitalRead(sensorPins[i]) == HIGH) ? "MOTION" : "CLEAR";
    // snrDoc["last_activated"] = formatTime(lastSensorTrigger[i]); // Formatted string
    snrDoc["last_activated"] = lastSensorTrigger[i]; // Formatted string
    // snrDoc["last_activated_date"] = getDateStamp();

    
    char snrBuffer[128];
    serializeJson(snrDoc, snrBuffer);
    client.publish(topic.c_str(), snrBuffer);
  }

  // Serial.println("Success!");

}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32_Motor_Unit")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

String getTimestamp() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return "Time Error";
  }
  char timeStringBuff[20]; 
  // Format: HH:MM:SS
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

// String getDateStamp() {
//   struct tm timeinfo;
//   if(!getLocalTime(&timeinfo)){
//     return "Date Error";
//   }
//   char dateStringBuff[20]; 
//   // Format: Day/Month/Year
//   strftime(dateStringBuff, sizeof(dateStringBuff), "%d/%m/%Y", &timeinfo);
//   return String(dateStringBuff);
// }

// JSON for SAP
void handleJSON() {
  StaticJsonDocument<512> doc; 
  doc["motor_active"] = motorRunning;
  doc["last_trigger"] = lastTrigger;
  // doc["last_motor_activation"] = formatTime(lastMotorActivation); 
  doc["last_activated"] = lastMotorActivation; 
  // doc["last_activated_date"] = getDateStamp(); 
  // doc["current_run_duration_sec"] = currentSessionTime;
  doc["current_session_time"] = currentSessionStr;
  doc["previous_session_time"] = previousSessionStr;
  doc["total_uptime"] = formatTime(millis());
  doc["sensor_status"] = digitalRead(sensorPins[0]) == HIGH ? "MOTION" : "CLEAR";

  String buf;
  serializeJson(doc, buf);
  server.send(200, "application/json", buf);
}

// Dashboard
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
  html += "}, 100);</script></body></html>"; // Refreshes faster (100ms) for millisecond accuracy

  server.send(200, "text/html", html);
}

// Helper function to format milliseconds to MM:SSS
String formatTime(unsigned long totalMillis) {
  unsigned long totalSeconds = totalMillis / 1000;
  int seconds = totalSeconds % 60;
  int minutes = (totalSeconds / 60) % 60;
  int hours = (totalSeconds / 3600);
  
  char buffer[15];
  // Formats to HH:MM:SS
  sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
  return String(buffer);
}