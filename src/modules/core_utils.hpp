// Implementation fragment included by src/main.cpp inside the HUD anonymous namespace.
void update_voice_config();

void log_event(const String &msg) {
  if (msg.length() == 0) {
    return;
  }
  g_logs[g_log_head] = msg;
  g_log_head = (g_log_head + 1) % LOG_CAP;
  Serial.println(msg);
}

void log_serial_only(const String &msg) {
  if (msg.length() == 0) {
    return;
  }
  Serial.println(msg);
}

float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

double haversine_distance_m(double lat1_deg, double lon1_deg, double lat2_deg, double lon2_deg) {
  static const double kEarthRadiusM = 6371000.0;
  const double lat1 = lat1_deg * DEG_TO_RAD;
  const double lon1 = lon1_deg * DEG_TO_RAD;
  const double lat2 = lat2_deg * DEG_TO_RAD;
  const double lon2 = lon2_deg * DEG_TO_RAD;
  const double dlat = lat2 - lat1;
  const double dlon = lon2 - lon1;
  const double sin_dlat = sin(dlat * 0.5);
  const double sin_dlon = sin(dlon * 0.5);
  const double a = sin_dlat * sin_dlat +
                   cos(lat1) * cos(lat2) * sin_dlon * sin_dlon;
  const double c = 2.0 * atan2(sqrt(a), sqrt(std::max(0.0, 1.0 - a)));
  return kEarthRadiusM * c;
}

int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void reset_gnss_speed_tracking() {
  g_gnss_speed_ms = -1.0f;
  g_gnss_prev_lat = NAN;
  g_gnss_prev_lon = NAN;
  g_gnss_prev_fix_sample_ms = 0;
}

void log_gnss_baud_event(const char *prefix, unsigned long baud) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%s%lu", prefix, baud);
  log_serial_only(String(buf));
}

void begin_gnss_uart(unsigned long baud) {
  GNSS.end();
  GNSS.setRxBufferSize(1024);
#if GNSS_USE_XIAO_DEFAULT_UART_PINS
  GNSS.begin(baud, SERIAL_8N1, -1, -1);
#else
  GNSS.begin(baud, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
#endif
  g_gnss_active_baud = baud;
}

void reset_gnss_parser(bool clear_location) {
  g_gps = TinyGPSPlus();
  g_gnss_satellites = 0;
  g_gnss_fix_age_ms = 0xFFFFFFFFUL;
  g_gnss_hdop = NAN;
  g_gnss_quality = 0;
  g_gnss_passed_checksum = 0;
  g_gnss_failed_checksum = 0;
  g_gnss_last_valid_sentence_ms = 0;
  g_gnss_data_seen = false;
  g_gnss_serial_seen = false;
  g_gnss_baud_locked = false;
  g_gnss_has_fix = false;
  if (clear_location) {
    g_gnss_lat = NAN;
    g_gnss_lon = NAN;
  }
  reset_gnss_speed_tracking();
}

void start_gnss_baud_probe(uint8_t index, uint32_t now, bool clear_location) {
  g_gnss_baud_index = index % GNSS_BAUD_CANDIDATE_COUNT;
  g_gnss_baud_probe_start_ms = now;
  begin_gnss_uart(GNSS_BAUD_CANDIDATES[g_gnss_baud_index]);
  reset_gnss_parser(clear_location);
  log_gnss_baud_event("GNSS_BAUD_", g_gnss_active_baud);
}

void advance_gnss_baud_probe(uint32_t now) {
#if GNSS_AUTOSCAN_BAUD
  const uint8_t next_index = static_cast<uint8_t>((g_gnss_baud_index + 1) % GNSS_BAUD_CANDIDATE_COUNT);
  start_gnss_baud_probe(next_index, now, false);
#else
  (void)now;
#endif
}

uint32_t gnss_satellite_count() {
  return g_gps.satellites.isValid() ? g_gps.satellites.value() : 0;
}

float gnss_hdop_value() {
  return g_gps.hdop.isValid() ? static_cast<float>(g_gps.hdop.hdop()) : NAN;
}

bool gnss_has_reliable_fix() {
  if (!g_gps.location.isValid() || g_gps.location.age() >= GNSS_FIX_TIMEOUT_MS) {
    return false;
  }
  if (g_gps.satellites.isValid() && g_gps.satellites.value() < GNSS_MIN_SATELLITES) {
    return false;
  }
  const float hdop = gnss_hdop_value();
  if (!isnan(hdop) && hdop > GNSS_MAX_HDOP) {
    return false;
  }
  return true;
}

int gnss_quality_score() {
  if (!g_gps.location.isValid()) {
    return 0;
  }

  int score = g_gps.location.age() < GNSS_FIX_TIMEOUT_MS ? 35 : 15;
  const uint32_t satellites = gnss_satellite_count();
  if (satellites > 0) {
    score += clampi(static_cast<int>(satellites) * 8, 0, 40);
  } else {
    score -= 10;
  }

  const float hdop = gnss_hdop_value();
  if (isnan(hdop)) {
    score -= 5;
  } else if (hdop <= 1.2f) {
    score += 25;
  } else if (hdop <= 2.0f) {
    score += 18;
  } else if (hdop <= GNSS_MAX_HDOP) {
    score += 10;
  } else {
    score -= clampi(static_cast<int>((hdop - GNSS_MAX_HDOP) * 12.0f), 10, 35);
  }

  if (!gnss_has_reliable_fix()) {
    score = std::min(score, 65);
  }

  return clampi(score, 0, 100);
}

int64_t days_from_civil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;
}

time_t utc_epoch_from_tm(const struct tm &t) {
  const int y = t.tm_year + 1900;
  const unsigned m = static_cast<unsigned>(t.tm_mon + 1);
  const unsigned d = static_cast<unsigned>(t.tm_mday);
  const int64_t days = days_from_civil(y, m, d);
  const int64_t secs = days * 86400LL + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
  return static_cast<time_t>(secs);
}

