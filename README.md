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

#ifndef SECRETS_H
#define SECRETS_H

#include <Arduino.h>

// --- ADAFRUIT IO CONFIG ---
const char* mqtt_server          = "io.adafruit.com";
const char* mqtt_user            = "";
const char* mqtt_key             = "";
const char* mqtt_temp_feed       = "";
const char* mqtt_zScore_feed     = "";
const char* mqtt_location_feed   = "";

// WiFi must be 2G for ESP to connect
const char* ssid                 = "";
const char* password             = "";

#endif

//-------------------------------------------------------------

Copy and place the above section into a Secrets.h file in the include folder.


## Adafruit Dashboard
<img width="1358" height="465" alt="adafruit_dashboard" src="https://github.com/user-attachments/assets/ab854a36-4840-4ce8-b48a-661651e72d7c" />


## Circtuit Diagram

