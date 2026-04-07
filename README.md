# Industrial IoT: Smart Transformer Thermal Monitor

A real-time, edge-computing solution for monitoring high-value electrical assets using ESP32, LittleFS for data logging, and MQTT-based anomaly detection.

## Key Features:

-Statistical Anomaly Detection: Implements a sliding-window Z-score algorithm to identify thermal "hotspots" while filtering for sensor noise.

-Fail-Safe Logging: Utilizes LittleFS on-board flash memory to ensure data persistence during WiFi or power outages.

-Live Geospatial Telemetry: Maps real-time data to Adafruit IO using coordinate-tagged MQTT packets for asset tracking.

-Interactive Maintenance Console: Serial-based command interface for manual log retrieval and system diagnostics.

## System Architecture

-Hardware: ESP32 Microcontroller, DHT22/DS18B20 Sensors.

-Communication: MQTT (Message Queuing Telemetry Transport).

-Storage: LittleFS (SPIFFS successor) for local .csv buffering.

-Analysis: C++ Implementation of Standard Deviation and Mean calculations on a 25-sample rolling window.

## Mathmatical Logic

This thermal monitor utilizes Z-score analysis with a rolling window to test new values. To prevent false positives a
hotspot notification is only triggered when |Z| > 3.0 & Standard Deviation > 0.4

## Setup and installation

-Environment: Developed in VS Code with PlatformIO.

-Dependencies: In platform.ini file

Config: Use a secrets file for Wifi, and Adafruit feeds and keys to ensure security;

//---------------------------------------------------------

#ifndef SECRETS_H<br>
#define SECRETS_H<br>

#include <Arduino.h><br>

// --- ADAFRUIT IO CONFIG ---
const char* mqtt_server          = "io.adafruit.com";<br>
const char* mqtt_user            = "";<br>
const char* mqtt_key             = "";<br>
const char* mqtt_temp_feed       = "";<br>
const char* mqtt_zScore_feed     = "";<br>
const char* mqtt_location_feed   = "";<br>

// WiFi must be 2G for ESP to connect<br>
const char* ssid                 = "";<br>
const char* password             = "";<br>

#endif<br>

//-------------------------------------------------------------

Copy and place the above section into a Secrets.h file in the include folder.


## Adafruit Dashboard
<img width="1358" height="465" alt="adafruit_dashboard" src="https://github.com/user-attachments/assets/ab854a36-4840-4ce8-b48a-661651e72d7c" />


## Circtuit Diagram

<img width="801" height="506" alt="image" src="https://github.com/user-attachments/assets/1aa344b5-aa38-4605-93e0-b8e8a483269c" />

## PCB Design

<img width="1182" height="712" alt="image" src="https://github.com/user-attachments/assets/fb36cb89-5c63-4b06-bdde-21ea6ac59bc1" />

## Hardware Design

-The custom PCB was designed in KiCad 9.0 with a focus on industrial reliability and RF signal integrity.

-Layer Stackup: 2-layer FR4 board with a 1.6mm thickness.

-Signal Integrity: Bottom-side ground plane utilized for EMI shielding, with a dedicated 10mm keep-out zone under the ESP32 antenna.

-Serviceability: Dual 1x15 female headers allow for modular replacement of the ESP32 DevKit without desoldering.


<img width="1083" height="604" alt="TEMPGPS" src="https://github.com/user-attachments/assets/56ec9acf-df51-48a1-b0af-115c6db7b2e5" />
