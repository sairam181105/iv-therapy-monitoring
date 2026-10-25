#include <WiFi.h>
#include <PubSubClient.h>

// --- Network Config ---
const char* ssid = "Marias";
const char* password = "123456789";
const char* mqtt_server = "broker.hivemq.com"; 

// --- Pins ---
const int IR_SENSOR_PIN = 14;      
const int BUZZER_PIN = 12;         
const int STATUS_LED = 13;         

// --- Configuration ---
const int DROP_FACTOR = 20;        
float fullCapacityMl = 500.0;      

// --- Variables ---
volatile unsigned long pulseInterval = 0;
volatile unsigned long lastDropTime = 0;
volatile int totalDropsCounted = 0;
bool newDrop = false;

WiFiClient espClient;
PubSubClient client(espClient);

void IRAM_ATTR countDrop() {
  unsigned long now = millis();
  pulseInterval = now - lastDropTime;
  if (now - lastDropTime > 150) { // 150ms debounce
    lastDropTime = now;
    totalDropsCounted++;
    newDrop = true;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  client.setServer(mqtt_server, 1883);
  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), countDrop, FALLING);
}

void loop() {
  if (!client.connected()) {
    client.connect("ESP32_IV_Monitor");
  }
  client.loop();

  // 1. Handle Serial Input (For manual capacity reset)
  if (Serial.available() > 0) {
    float inputVal = Serial.parseFloat();
    if (inputVal > 0) {
      fullCapacityMl = inputVal;
      totalDropsCounted = 0;
      noTone(BUZZER_PIN);
      Serial.println("Capacity Reset!");
    }
  }

  // 2. Handle Logic on Every Drop
  if (newDrop) {
    newDrop = false;
    
    // Calculations from your edit
    float dpm = 60000.0 / pulseInterval;
    float mlPerHour = (dpm * 60.0) / DROP_FACTOR;
    float mlUsed = (float)totalDropsCounted / DROP_FACTOR;
    float mlLeft = fullCapacityMl - mlUsed;
    float percentageLeft = (mlLeft / fullCapacityMl) * 100.0;

    // --- ALARM LOGIC (10% Threshold) ---
    if (percentageLeft <= 10.0 && percentageLeft > 0) {
      tone(BUZZER_PIN, 2000, 70); 
    } else {
      noTone(BUZZER_PIN);
    }

    // --- WIFI PUBLISH ---
    // Sending all three metrics to the dashboard
    String payload = "{";
    payload += "\"rate\":" + String(mlPerHour, 1) + ",";
    payload += "\"ml\":" + String(mlLeft, 1) + ",";
    payload += "\"perc\":" + String(percentageLeft, 1);
    payload += "}";
    
    client.publish("hospital/iv/data", payload.c_str());

    // Local Serial Debug
    Serial.print("Rate: "); Serial.print(mlPerHour, 1);
    Serial.print(" | Rem: "); Serial.print(mlLeft, 1);
    Serial.print(" | %: "); Serial.println(percentageLeft, 1);

    digitalWrite(STATUS_LED, HIGH); delay(30); digitalWrite(STATUS_LED, LOW);
  }
}