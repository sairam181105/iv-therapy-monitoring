#include <Arduino.h>

#define WINDOW_SIZE 5

float rateBuffer[WINDOW_SIZE] = {0};
int rateIndex = 0;

// --- Function Declaration ---
void sendData(float mlPerHour, float mlLeft, float percentageLeft, bool flowStopped, bool bottleEmpty, bool less10);

// --- Pin Definitions ---
const int IR_SENSOR_PIN = 14;
const int BUZZER_PIN = 13;
const int STATUS_LED = 12;

// --- Configuration ---
const int DROP_FACTOR = 20;
float fullCapacityMl = 500.0;

// --- Global Variables ---
volatile unsigned long lastDropTime = 0;
volatile unsigned long pulseInterval = 0;
volatile int totalDropsCounted = 0;
bool newDrop = false;

// Interrupt for drop detection
void IRAM_ATTR countDrop() {
  unsigned long now = millis();
  if (now - lastDropTime > 150) {
    pulseInterval = now - lastDropTime;
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

  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), countDrop, FALLING);

  Serial.println("IV Monitoring Started...");
}

void loop() {

  // =======================
  // 1. SERIAL INPUT (CAPACITY)
  // =======================
  if (Serial.available() > 0) {
    float inputVal = Serial.parseFloat();

    Serial.print("Received Capacity: ");
    Serial.println(inputVal);

    if (inputVal > 0) {
      fullCapacityMl = inputVal;
      totalDropsCounted = 0;
      lastDropTime = millis();
      digitalWrite(BUZZER_PIN, LOW);

      Serial.print("Updated Capacity to: ");
      Serial.println(fullCapacityMl);
    }

    while (Serial.available() > 0) Serial.read();
  }

  // =======================
  // VARIABLES
  // =======================
  float percentageLeft = 0;
  float mlLeft = 0;
  float mlUsed = 0;
  float mlPerHour = 0;
  float dpm = 0;

  static unsigned long lastWarn = 0;
  static unsigned long lastSend = 0;
  static unsigned long lastFlowSend = 0;

  // =======================
  // 2. DROP DETECTED
  // =======================
  if (newDrop) {
    newDrop = false;

    digitalWrite(STATUS_LED, HIGH);

    // Safe division
    if (pulseInterval > 0) {
      dpm = 60000.0 / pulseInterval;
    }

    // Raw rate
    float instantRate = (dpm * 60.0) / DROP_FACTOR;

    // Moving average smoothing
    rateBuffer[rateIndex] = instantRate;
    rateIndex = (rateIndex + 1) % WINDOW_SIZE;

    float sum = 0;
    for (int i = 0; i < WINDOW_SIZE; i++) {
      sum += rateBuffer[i];
    }
    mlPerHour = sum / WINDOW_SIZE;

    // Volume calculations
    mlUsed = (float)totalDropsCounted / DROP_FACTOR;
    mlLeft = fullCapacityMl - mlUsed;
    percentageLeft = (mlLeft / fullCapacityMl) * 100.0;

    // Safety clamp
    if (mlLeft < 0) mlLeft = 0;
    if (percentageLeft < 0) percentageLeft = 0;

    // Flags
    bool bottleEmpty = (percentageLeft <= 0.0);
    bool less10 = (percentageLeft > 0.0 && percentageLeft <= 10.0);
    bool flowStopped = false;

    // Alerts
    if (bottleEmpty) {
      tone(BUZZER_PIN, 1000, 500);
    } else if (less10) {
      tone(BUZZER_PIN, 2000, 500);
    }

    digitalWrite(STATUS_LED, LOW);

    // Send data every 1 second
    if (millis() - lastSend > 1000) {
      sendData(mlPerHour, mlLeft, percentageLeft, flowStopped, bottleEmpty, less10);
      lastSend = millis();
    }
  }

  // =======================
  // 3. FLOW STOPPED
  // =======================
  if (millis() - lastDropTime > 3000 && totalDropsCounted > 0) {

    mlUsed = (float)totalDropsCounted / DROP_FACTOR;
    mlLeft = fullCapacityMl - mlUsed;
    percentageLeft = (mlLeft / fullCapacityMl) * 100.0;

    if (mlLeft < 0) mlLeft = 0;
    if (percentageLeft < 0) percentageLeft = 0;

    bool bottleEmpty = (percentageLeft <= 0.0);
    bool less10 = (percentageLeft > 0.0 && percentageLeft <= 10.0);
    bool flowStopped = true;

    // Warning beep
    if (millis() - lastWarn > 500 && percentageLeft > 0) {
      tone(BUZZER_PIN, 1000, 100);
      lastWarn = millis();
    }

    // Send only once per second
    if (millis() - lastFlowSend > 1000) {
      sendData(mlPerHour, mlLeft, percentageLeft, flowStopped, bottleEmpty, less10);
      lastFlowSend = millis();
    }
  }
}

// =======================
// SEND JSON
// =======================
void sendData(float mlPerHour, float mlLeft, float percentageLeft, bool flowStopped, bool bottleEmpty, bool less10) {

  Serial.println(
    String("{\"rate\":") + mlPerHour +
    ",\"remaining_ml\":" + mlLeft +
    ",\"percent\":" + percentageLeft +
    ",\"flowStopped\":" + (flowStopped ? "true" : "false") +
    ",\"bottleEmpty\":" + (bottleEmpty ? "true" : "false") +
    ",\"less_than_10\":" + (less10 ? "true" : "false") +
    "}"
  );
}