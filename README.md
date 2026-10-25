# Smart IV Infusion Therapy Monitoring System

An IoT-enabled healthcare monitoring solution designed to automate intravenous (IV) drip tracking, calculate real-time flow rates, and provide bedside and remote clinical alerts.

---

## Features

- **Automated Drop Counting**: Optical infrared sensor detects individual drop cadence via hardware interrupts.
- **Bedside Audio-Visual Alarms**: Buzzer and LED activate when infusion stops unexpectedly or volume drops below 10%.
- **Live Telemetry Streaming**: Real-time broadcast of fluid level, volume remaining, and infusion rate over WebSockets.
- **Dedicated Dashboards**:
  - `nurse.html`: Central monitoring hub for medical staff with audible emergency alerts.
  - `patient.html`: Bedside monitor showing current drip status.
- **Patient Management Service**: Lightweight Flask service for managing infusion assignments.

---

## Hardware Pin Connections

| Peripheral | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **IR Sensor Output** | `GPIO 14` | Drip pulse detection (Interrupt) |
| **Piezo Buzzer** | `GPIO 13` | Critical warning alarm |
| **Status LED** | `GPIO 12` | Low-level / fault indicator |

---

## Tech Stack

- **Firmware**: C++ (Arduino Framework), PlatformIO
- **Gateway Server**: Node.js, Express, Socket.IO, `@serialport`
- **Data Microservice**: Python, Flask
- **Frontend**: HTML5, CSS3, Vanilla JavaScript

---

## Getting Started

### 1. ESP32 Firmware
1. Open the project directory in VS Code with the **PlatformIO IDE** extension.
2. In `platformio.ini`, confirm your ESP32 COM port (`upload_port` and `monitor_port`).
3. Flash the firmware:
   ```bash
   pio run --target upload
## Demo Video
https://github.com/sairam181105/iv-therapy-monitoring/raw/main/assets/Demo.mp4
