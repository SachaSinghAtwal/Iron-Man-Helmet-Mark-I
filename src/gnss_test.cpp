#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

#if __has_include("secrets.h")
#include "secrets.h"
#endif

#ifndef GNSS_RX_PIN
#define GNSS_RX_PIN 44
#endif

#ifndef GNSS_TX_PIN
#define GNSS_TX_PIN 43
#endif

#ifndef GNSS_BAUD
#define GNSS_BAUD 9600
#endif

#ifndef GNSS_UART_PORT
#define GNSS_UART_PORT 1
#endif
#ifndef GNSS_HW_UART_PORT
#define GNSS_HW_UART_PORT 0
#endif
#ifndef GNSS_USE_XIAO_DEFAULT_UART_PINS
#define GNSS_USE_XIAO_DEFAULT_UART_PINS 1
#endif

#ifndef GNSS_MIN_SATELLITES
#define GNSS_MIN_SATELLITES 4
#endif

#ifndef GNSS_MAX_HDOP
#define GNSS_MAX_HDOP 3.5f
#endif

#ifndef GNSS_SPEED_SAMPLE_MIN_INTERVAL_MS
#define GNSS_SPEED_SAMPLE_MIN_INTERVAL_MS 3000
#endif

#ifndef GNSS_SPEED_SAMPLE_MAX_INTERVAL_MS
#define GNSS_SPEED_SAMPLE_MAX_INTERVAL_MS 10000
#endif

#ifndef GNSS_SPEED_DISTANCE_NOISE_M
#define GNSS_SPEED_DISTANCE_NOISE_M 2.5f
#endif

#ifndef GNSS_SPEED_BLEND_NEW
#define GNSS_SPEED_BLEND_NEW 0.35f
#endif

#ifndef GNSS_SPEED_MAX_MPS
#define GNSS_SPEED_MAX_MPS 80.0f
#endif
#ifndef GNSS_AUTOSCAN_BAUD
#define GNSS_AUTOSCAN_BAUD 1
#endif
#ifndef GNSS_BAUD_PROBE_WINDOW_MS
#define GNSS_BAUD_PROBE_WINDOW_MS 2500
#endif
#ifndef GNSS_VALID_SENTENCE_TIMEOUT_MS
#define GNSS_VALID_SENTENCE_TIMEOUT_MS 10000
#endif

namespace {
HardwareSerial gnssSerial(GNSS_HW_UART_PORT);
TinyGPSPlus gps;

char nmeaLine[128];
size_t nmeaLen = 0;
uint32_t bootMs = 0;
uint32_t lastSummaryMs = 0;
uint32_t lastChars = 0;
bool warnedNoData = false;
double prevLat = NAN;
double prevLon = NAN;
uint32_t prevFixMs = 0;
float computedSpeedMs = -1.0f;
bool serialSeen = false;
bool validNmeaSeen = false;
bool baudLocked = false;
uint8_t baudIndex = 0;
uint32_t baudProbeStartMs = 0;
uint32_t lastValidSentenceMs = 0;
unsigned long activeBaud = GNSS_BAUD;
static const unsigned long kBaudCandidates[] = {
  static_cast<unsigned long>(GNSS_BAUD),
  9600UL,
  38400UL,
  57600UL,
  115200UL,
  19200UL,
  4800UL
};
static const size_t kBaudCandidateCount = sizeof(kBaudCandidates) / sizeof(kBaudCandidates[0]);

double haversineDistanceM(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg) {
  static const double kEarthRadiusM = 6371000.0;
  const double lat1 = lat1Deg * DEG_TO_RAD;
  const double lon1 = lon1Deg * DEG_TO_RAD;
  const double lat2 = lat2Deg * DEG_TO_RAD;
  const double lon2 = lon2Deg * DEG_TO_RAD;
  const double dlat = lat2 - lat1;
  const double dlon = lon2 - lon1;
  const double sinDlat = sin(dlat * 0.5);
  const double sinDlon = sin(dlon * 0.5);
  const double a = sinDlat * sinDlat + cos(lat1) * cos(lat2) * sinDlon * sinDlon;
  const double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  return kEarthRadiusM * c;
}

bool reliableFix() {
  if (!gps.location.isValid() || gps.location.age() >= 5000) {
    return false;
  }
  if (gps.satellites.isValid() && gps.satellites.value() < GNSS_MIN_SATELLITES) {
    return false;
  }
  if (gps.hdop.isValid() && gps.hdop.hdop() > GNSS_MAX_HDOP) {
    return false;
  }
  return true;
}

void resetTracking(bool clearLocation) {
  gps = TinyGPSPlus();
  serialSeen = false;
  validNmeaSeen = false;
  baudLocked = false;
  lastValidSentenceMs = 0;
  computedSpeedMs = -1.0f;
  prevFixMs = 0;
  if (clearLocation) {
    prevLat = NAN;
    prevLon = NAN;
  }
}

void beginGnssUart(unsigned long baud) {
  gnssSerial.end();
  gnssSerial.setRxBufferSize(1024);
#if GNSS_USE_XIAO_DEFAULT_UART_PINS
  gnssSerial.begin(baud, SERIAL_8N1, -1, -1);
#else
  gnssSerial.begin(baud, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
#endif
  activeBaud = baud;
}

void startBaudProbe(uint8_t nextIndex, uint32_t now, bool clearLocation) {
  baudIndex = nextIndex % kBaudCandidateCount;
  baudProbeStartMs = now;
  beginGnssUart(kBaudCandidates[baudIndex]);
  resetTracking(clearLocation);
  Serial.print(F("GNSS baud probe: "));
  Serial.println(activeBaud);
}

void advanceBaudProbe(uint32_t now) {
#if GNSS_AUTOSCAN_BAUD
  startBaudProbe(static_cast<uint8_t>((baudIndex + 1) % kBaudCandidateCount), now, false);
#else
  (void)now;
#endif
}

void printSummary() {
  Serial.println();
  Serial.println(F("=== GNSS SUMMARY ==="));
  Serial.print(F("UART baud: "));
  Serial.println(activeBaud);
  Serial.print(F("Baud locked: "));
  Serial.println(baudLocked ? F("YES") : F("NO"));
  Serial.print(F("Chars processed: "));
  Serial.println(gps.charsProcessed());
  Serial.print(F("Passed checksum: "));
  Serial.println(gps.passedChecksum());
  Serial.print(F("Sentences with fix data: "));
  Serial.println(gps.sentencesWithFix());
  Serial.print(F("Failed checksum: "));
  Serial.println(gps.failedChecksum());
  Serial.print(F("Satellites: "));
  if (gps.satellites.isValid()) {
    Serial.println(gps.satellites.value());
  } else {
    Serial.println(F("--"));
  }
  Serial.print(F("HDOP: "));
  if (gps.hdop.isValid()) {
    Serial.println(gps.hdop.hdop());
  } else {
    Serial.println(F("--"));
  }
  Serial.print(F("Location valid: "));
  Serial.println(gps.location.isValid() ? F("YES") : F("NO"));
  Serial.print(F("Reliable fix: "));
  Serial.println(reliableFix() ? F("YES") : F("NO"));
  if (gps.location.isValid()) {
    Serial.print(F("Latitude: "));
    Serial.println(gps.location.lat(), 6);
    Serial.print(F("Longitude: "));
    Serial.println(gps.location.lng(), 6);
    Serial.print(F("Location age (ms): "));
    Serial.println(gps.location.age());
  }
  Serial.print(F("Speed valid: "));
  Serial.println(gps.speed.isValid() ? F("YES") : F("NO"));
  if (gps.speed.isValid()) {
    Serial.print(F("Speed km/h: "));
    Serial.println(gps.speed.kmph(), 2);
    Serial.print(F("Speed mph: "));
    Serial.println(gps.speed.mph(), 2);
  }
  Serial.print(F("Speed m/s (distance/time): "));
  if (computedSpeedMs >= 0.0f) {
    Serial.println(computedSpeedMs, 2);
  } else {
    Serial.println(F("--"));
  }
  Serial.print(F("Date valid: "));
  Serial.println(gps.date.isValid() ? F("YES") : F("NO"));
  Serial.print(F("Time valid: "));
  Serial.println(gps.time.isValid() ? F("YES") : F("NO"));
  Serial.println(F("===================="));
  Serial.println();
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println();
  Serial.println(F("GNSS test boot"));
  Serial.print(F("RX pin: "));
  Serial.println(GNSS_RX_PIN);
  Serial.print(F("TX pin: "));
  Serial.println(GNSS_TX_PIN);
  Serial.print(F("Baud: "));
  Serial.println(GNSS_BAUD);
  Serial.println(F("Transport: HardwareSerial UART (GNSS fix path)"));
  bootMs = millis();
  startBaudProbe(0, bootMs, true);
  Serial.println(F("Waiting for GNSS data..."));
}

void loop() {
  bool sawSerialThisLoop = false;
  while (gnssSerial.available()) {
    sawSerialThisLoop = true;
    const char c = static_cast<char>(gnssSerial.read());
    gps.encode(c);

    if (c == '\n') {
      nmeaLine[nmeaLen] = '\0';
      if (nmeaLen > 0) {
        Serial.print(F("[NMEA] "));
        Serial.println(nmeaLine);
      }
      nmeaLen = 0;
    } else if (c != '\r' && nmeaLen < sizeof(nmeaLine) - 1) {
      nmeaLine[nmeaLen++] = c;
    }
  }
  if (sawSerialThisLoop) {
    serialSeen = true;
  }

  const uint32_t now = millis();
  if (gps.passedChecksum() > 0) {
    validNmeaSeen = true;
    baudLocked = true;
    lastValidSentenceMs = now;
  }

#if GNSS_AUTOSCAN_BAUD
  if (!baudLocked && (now - baudProbeStartMs) >= GNSS_BAUD_PROBE_WINDOW_MS && gps.passedChecksum() == 0) {
    advanceBaudProbe(now);
    return;
  }
  if (baudLocked && lastValidSentenceMs > 0 && (now - lastValidSentenceMs) >= GNSS_VALID_SENTENCE_TIMEOUT_MS) {
    Serial.println(F("GNSS NMEA timeout, restarting baud scan."));
    startBaudProbe(0, now, false);
    return;
  }
#endif

  if (gps.location.isUpdated()) {
    const uint32_t sampleMs = now;
    if (reliableFix()) {
      const double lat = gps.location.lat();
      const double lon = gps.location.lng();
      if (!isnan(prevLat) && !isnan(prevLon) && prevFixMs > 0 && sampleMs > prevFixMs) {
        const uint32_t elapsedMs = sampleMs - prevFixMs;
        if (elapsedMs > GNSS_SPEED_SAMPLE_MAX_INTERVAL_MS) {
          computedSpeedMs = 0.0f;
          prevLat = lat;
          prevLon = lon;
          prevFixMs = sampleMs;
        } else if (elapsedMs >= GNSS_SPEED_SAMPLE_MIN_INTERVAL_MS) {
          const double elapsedS = static_cast<double>(elapsedMs) / 1000.0;
          const double distanceM = haversineDistanceM(prevLat, prevLon, lat, lon);
          const float sampleSpeedMs = distanceM <= GNSS_SPEED_DISTANCE_NOISE_M
                                          ? 0.0f
                                          : static_cast<float>(distanceM / elapsedS);
          if (sampleSpeedMs >= 0.0f && sampleSpeedMs <= GNSS_SPEED_MAX_MPS) {
            if (computedSpeedMs < 0.0f) {
              computedSpeedMs = sampleSpeedMs;
            } else {
              computedSpeedMs =
                  computedSpeedMs * (1.0f - GNSS_SPEED_BLEND_NEW) + sampleSpeedMs * GNSS_SPEED_BLEND_NEW;
            }
          }
          prevLat = lat;
          prevLon = lon;
          prevFixMs = sampleMs;
        }
      } else {
        computedSpeedMs = 0.0f;
        prevLat = lat;
        prevLon = lon;
        prevFixMs = sampleMs;
      }
    } else {
      computedSpeedMs = -1.0f;
      prevLat = NAN;
      prevLon = NAN;
      prevFixMs = 0;
    }
  }

  if (!warnedNoData && now - bootMs > 5000 && !validNmeaSeen) {
    warnedNoData = true;
    Serial.println();
    if (serialSeen) {
      Serial.println(F("SERIAL DATA IS PRESENT, BUT NO VALID GNSS NMEA SENTENCES HAVE BEEN DECODED."));
      Serial.println(F("This usually means the baud rate is wrong or the incoming data is corrupted."));
    } else {
      Serial.println(F("NO GPS DATA RECEIVED."));
      Serial.println(F("Check module orientation, pin seating, solder joints, and antenna connection."));
    }
    Serial.println();
  }

  if (now - lastSummaryMs > 2000) {
    lastSummaryMs = now;
    if (gps.charsProcessed() != lastChars || gps.location.isUpdated() || gps.speed.isUpdated()) {
      lastChars = gps.charsProcessed();
      printSummary();
    }
  }
}
