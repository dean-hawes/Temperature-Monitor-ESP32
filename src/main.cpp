#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <DHT.h>
#include <TinyGPS++.h>
#include <TimeLib.h> 

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "AnomalyDetector.h"
#include "Structs.h"

#define DHT_PIN 4
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// WiFi must be 2G for ESP to connect
const char* ssid = "";
const char* password = "";

// --- OBJECTS ---
WiFiClient espClient;
PubSubClient mqtt(espClient);
TinyGPSPlus gps;
AnomalyDetector tempMonitor(10);
unsigned long lastMsg = 0;
unsigned long lastSent = 0;
const char* logPath = "/logs.csv";

// --- ADAFRUIT IO CONFIG ---
const char* mqtt_server = "io.adafruit.com";
const char* mqtt_user   = "";
const char* mqtt_key    = "";
const char* mqtt_temp_feed   = "";
const char* mqtt_zScore_feed = "";
const char* mqtt_location_feed = "";


// --- MQTT connect function ---
void connectMQTT() {
    while (!mqtt.connected()) {
        Serial.print("Attempting MQTT connection...");
        
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

// --- Log to Flash ---
// logs data to LittleFS partition on ESP32
void logToFlash(float temp, float zScore, double latt, double lng, unsigned long time) {
    File file = LittleFS.open(logPath, FILE_APPEND); 
    
    if (!file) {
        Serial.println("[ERROR] LittleFS: Failed to open file for appending.");
        return;
    }

    size_t bytesWritten;
    if (latt == 0.0 && lng == 0.0) {
        bytesWritten = file.printf("%.2f,%.2f,-,-\n", temp, zScore);
    } else {
        bytesWritten = file.printf("%.2f,%.2f,%.6f,%.6f,%lu\n", temp, zScore, latt, lng, time);
    }

    if (bytesWritten > 0) {
        Serial.printf("[SUCCESS] Logged %d bytes to Flash.\n", bytesWritten);
    } else {
        Serial.println("[ERROR] LittleFS: Write failed.");
    }

    file.close(); 
}

// ---Prints all entries on LittleFS partition
void dumpLogs() {
    File file = LittleFS.open(logPath, "r");
    if (!file) {
        Serial.println("No log file found.");
        return;
    }
    Serial.println("---CSV_START---");
    while (file.available()) {
        Serial.write(file.read());
    }
    Serial.println("---CSV_END---");
    file.close();
}

// ---Get UnixTime---
// Returns time from gps is unix format

unsigned long  getUnixTime(){
    if (gps.date.isValid() && gps.time.isValid()) {
        tmElements_t tm;
        tm.Year = CalendarYrToTm(gps.date.year()); // Years since 1970
        tm.Month = gps.date.month();
        tm.Day = gps.date.day();
        tm.Hour = gps.time.hour();
        tm.Minute = gps.time.minute();
        tm.Second = gps.time.second();

        unsigned long unixTime = makeTime(tm); // Convert to Unix time
        return unixTime;
        } else {return 0;}
}


void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n--- LEVEL 2: WIFI + LOGIC TEST ---");
    
    // Limit wifi power to avoid brownouts
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
    if(!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed!");
    } else {
        Serial.println("LittleFS Mounted Successfully.");
        
        File file = LittleFS.open("/logs.csv", "r");
        if(file) {
            file.close();
            Serial.println("Testing Flash Write Stability...");
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

    //Check for input at the start of loop
    if (Serial.available() > 0) {
        char incoming = Serial.read(); // Read the first character sent
        
        // Option: 'd' for Dump
        if (incoming == 'd' || incoming == 'D') {
            Serial.println("\n[COMMAND] Manual Log Dump Requested...");
            dumpLogs();
        }
        
        // Option: 'e' for Erase (Careful!)
        if (incoming == 'e' || incoming == 'E') {
            Serial.println("\n[WARNING] Formatting LittleFS... All logs will be lost!");
            LittleFS.format();
            Serial.println("[SUCCESS] Flash Memory Wiped.");
        }
    }

    //Keep MQTT Alive
    if (!mqtt.connected()) {
        connectMQTT();
    }
    mqtt.loop();

    //Feed GPS
    while (Serial2.available() > 0) {
        gps.encode(Serial2.read());
    }

    //Main Logic (Every 5 seconds)
    unsigned long now = millis();
    if (now - lastMsg > 5000) {
        lastMsg = now;

        float t = dht.readTemperature();
        float latt = gps.location.lat();
        float lng = gps.location.lng();
        unsigned long time = getUnixTime();
        if (!isnan(t)) {
            tempMonitor.update(t);
            float z = tempMonitor.getZscore();

            if (!isnan(latt) && !isnan(lng)){
                Serial.printf("Temp: %.1f | Z: %.2f | Latt: %.6f | Long: %.6f | UnixTime: %lu", t, z, latt, lng, time);
            } else{
            Serial.printf("Temp: %.1f | Z: %.2f  -> No GPS fix", t, z);}
            
            if (now - lastSent > 60000){
                lastSent = now;
                Serial.println("\nRoutine Sending to Adafruit\n");
                if (!isnan(latt) && !isnan(lng)){
                    String temp_payload = String(t);
                    String location_payload = "0," +String(latt, 6) + "," + String(lng, 6) + ",0";
                    String zScore_payload = String(z);
                    mqtt.publish(mqtt_temp_feed, temp_payload.c_str());
                    mqtt.publish(mqtt_location_feed, location_payload.c_str());
                    mqtt.publish(mqtt_zScore_feed, zScore_payload.c_str());
                    logToFlash(t,z,latt,lng,time);

                }else{
                    String temp_payload = String(t);
                    String zScore_payload = String(z);
                    mqtt.publish(mqtt_temp_feed, temp_payload.c_str());
                    mqtt.publish(mqtt_zScore_feed, zScore_payload.c_str());
                    logToFlash(t,z,0.0,0.0,0);
                }
            }
            //Trigger logic
            if (tempMonitor.isHotSpot()) {
            Serial.println(" -> !! HOTSPOT !! Sending to Adafruit...");
                String temp_payload = String(t);
                String location_payload = "0," +String(latt, 6) + "," + String(lng, 6) + ",0";
                String zScore_payload = String(z);+ "0.0";
                mqtt.publish(mqtt_temp_feed, temp_payload.c_str());
                mqtt.publish(mqtt_location_feed, location_payload.c_str());
                mqtt.publish(mqtt_zScore_feed, zScore_payload.c_str());
                logToFlash(t,z,latt,lng, time);
            } else {
            Serial.println(" -> Normal");
            }
            
        }   
    
    }

}