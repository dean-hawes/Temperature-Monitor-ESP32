#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <DHT.h>
#include <TinyGPS++.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "AnomalyDetector.h"
#include "Structs.h"

#define DHT_PIN 4
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

const char* ssid = "yourwifi";
const char* password = "wifipassword";

// --- OBJECTS ---
WiFiClient espClient;
PubSubClient mqtt(espClient);
TinyGPSPlus gps;
AnomalyDetector tempMonitor(10);
unsigned long lastMsg = 0;

// --- ADAFRUIT IO CONFIG ---
const char* mqtt_server = "io.adafruit.com";
const char* mqtt_user   = "adafruit username";
const char* mqtt_key    = "adafruit key";
const char* mqtt_feed   = "adafruit feed";


void connectMQTT() {
    while (!mqtt.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Client ID, Username, Password
        if (mqtt.connect("ESP32_Client", mqtt_user, mqtt_key)) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqtt.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n--- LEVEL 2: WIFI + LOGIC TEST ---");
    
    // KEEP THE WORKING WIFI SETTINGS
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_11dBm); 
    WiFi.begin(ssid, password);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");

    Serial.println("\n--- LEVEL 3: FLASH STORAGE TEST ---");
    
    // 1. Try to mount the file system
    if(!LittleFS.begin(true)) { // 'true' forces a format if it's broken
        Serial.println("LittleFS Mount Failed!");
    } else {
        Serial.println("LittleFS Mounted Successfully.");
        
        // 2. Try a test write - this is the "Stress Test"
        File file = LittleFS.open("/log.csv", FILE_WRITE);
        if(file) {
            file.println("Testing Flash Write Stability...");
            file.close();
            Serial.println("Flash Write SUCCESS!");
        } else {
            Serial.println("File Open Failed!");
        }
    }

    Serial.println("\n--- LEVEL 4: SENSOR INITIALIZATION ---");
    
    // 1. Start DHT
    dht.begin();
    float t = dht.readTemperature();
    if (isnan(t)) {
        Serial.println("[FAIL] DHT not found or wiring loose.");
    } else {
        Serial.printf("[SUCCESS] DHT Reading: %.1f C\n", t);
    }

    // 2. Start GPS Serial
    // Even if it doesn't have a fix yet, starting the hardware UART is the test.
    Serial2.begin(9600, SERIAL_8N1, 22, 23); 
    Serial.println("[SUCCESS] Serial2 (GPS) Hardware Port Opened.");

    mqtt.setServer(mqtt_server, 1883);
}

void loop() {
    // 1. Keep MQTT Alive
    if (!mqtt.connected()) {
        connectMQTT();
    }
    mqtt.loop();

    // 2. Feed GPS
    while (Serial2.available() > 0) {
        gps.encode(Serial2.read());
    }

    // 3. Main Logic (Every 5 seconds)
    unsigned long now = millis();
    if (now - lastMsg > 5000) {
        lastMsg = now;

        float t = dht.readTemperature();
        if (!isnan(t)) {
            tempMonitor.update(t);
            float z = tempMonitor.getZscore();

            Serial.printf("Temp: %.1f | Z: %.2f ", t, z);
            

            // 4. Trigger logic
            if (tempMonitor.isHotSpot()) {
                Serial.println(" -> !! HOTSPOT !! Sending to Adafruit...");
                
                // Format: "Temp, Lat, Lng"
                String payload = String(t) + "," + 
                                 String(gps.location.lat(), 6) + "," + 
                                 String(gps.location.lng(), 6);
                
                mqtt.publish(mqtt_feed, payload.c_str());
            } else {
                Serial.println(" -> Normal");
            }
        }
    }
}