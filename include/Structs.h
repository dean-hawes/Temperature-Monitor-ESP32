#ifndef STRUCTS_H  // "If STRUCTS_H is not defined..."
#define STRUCTS_H  // "...then define it now."

#include <Arduino.h>

// STRUCTS ----------------------------------------------

typedef struct {
  float tempC;
  float humidity;
  bool valid;
  uint32_t lastReadMs;
} DHTData;



typedef struct {
  float latt, lon;
  bool valid;
  uint32_t unixTime;
} GPSData;


typedef struct {
  DHTData dhtdata;
  GPSData gpsdata;
  float zScore;
} SystemState;

#endif