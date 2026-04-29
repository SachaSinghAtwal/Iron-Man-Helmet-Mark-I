// Implementation fragment included by src/main.cpp inside the HUD anonymous namespace.
int wifi_quality_from_rssi(int rssi) {
  if (rssi <= -100) return 0;
  if (rssi >= -50) return 100;
  return 2 * (rssi + 100);
}

String format_uptime(uint32_t seconds) {
  uint32_t h = seconds / 3600;
  uint32_t m = (seconds % 3600) / 60;
  uint32_t s = seconds % 60;
  char buf[16];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)s);
  return String(buf);
}

String format_time() {
  const time_t now = time(nullptr);
  if (now < 1704067200) {
    return String("--:--");
  }
  struct tm timeinfo;
  if (!localtime_r(&now, &timeinfo)) {
    return String("--:--");
  }
  char buf[16];
  strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
  return String(buf);
}

float read_soc_temp_c() {
#if defined(ARDUINO_ARCH_ESP32)
  return temperatureRead();
#else
  return NAN;
#endif
}

float read_power_bank_percent() {
#if BATTERY_SENSE_PIN >= 0
  const uint16_t raw = analogRead(BATTERY_SENSE_PIN);
  const float volts = (raw / 4095.0f) * 3.3f * BATTERY_SENSE_DIVIDER;
  const float span = BATTERY_SENSE_MAX_V - BATTERY_SENSE_MIN_V;
  if (span <= 0.01f) return -1.0f;
  const float pct = (volts - BATTERY_SENSE_MIN_V) / span;
  return clampf(pct, 0.0f, 1.0f) * 100.0f;
#else
  return -1.0f;
#endif
}

int read_heart_rate_bpm() {
#if HEART_RATE_SENSOR_PIN >= 0
  // Placeholder for a real heart rate sensor integration.
  // Return -1 if not implemented to avoid displaying fake data.
  (void)analogRead(HEART_RATE_SENSOR_PIN);
  return -1;
#else
  return -1;
#endif
}
