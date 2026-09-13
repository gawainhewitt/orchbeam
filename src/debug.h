#pragma once

#include <Arduino.h>
#include "config.h"
#include "./storage/sd_logger.h"

// Forward declare SD logger
#ifdef DEBUG_ON
void logToSD(const char* msg);
#endif

#ifdef DEBUG_ON 
  #define DEBUG(x) do { \
      Serial.println(x); \
      logToSD(x); \
  } while(0)
  
  #define DEBUGF(x, ...) do { \
      Serial.printf(x, __VA_ARGS__); \
      char _buf[256]; \
      snprintf(_buf, sizeof(_buf), x, __VA_ARGS__); \
      logToSD(_buf); \
  } while(0)
#else
  #define DEBUG(x)
  #define DEBUGF(x, ...)
#endif