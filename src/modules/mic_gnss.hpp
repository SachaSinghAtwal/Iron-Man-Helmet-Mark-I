// Implementation fragment included by src/main.cpp inside the HUD anonymous namespace.
bool init_mic() {
  g_mic_lock = xSemaphoreCreateMutex();
  if (!g_mic_lock) {
    return false;
  }

  // XIAO ESP32-S3 Sense PDM mic uses GPIO42 as CLK and GPIO41 as DATA.
  // The Arduino I2S PDM example uses SCK = -1, FS = CLK, SD = DATA.
  I2S.setAllPins(-1, MIC_CLK_PIN, MIC_DATA_PIN, -1, -1);
  if (!I2S.begin(PDM_MONO_MODE, MIC_SAMPLE_RATE, MIC_SAMPLE_BITS)) {
    return false;
  }
  I2S.setBufferSize(1024);
  g_mic_ok = true;
  g_voice_lock = xSemaphoreCreateMutex();
  update_voice_config();
  return true;
}

void update_mic_level() {
  if (!g_mic_ok) {
    return;
  }
  const uint32_t now = millis();
  if (now - g_last_mic_ms < MIC_LEVEL_INTERVAL_MS) {
    return;
  }
  g_last_mic_ms = now;
  if (xSemaphoreTake(g_mic_lock, 0) != pdTRUE) {
    return;
  }

  constexpr size_t kSamples = 256;
  int16_t samples[kSamples];
  const int bytes = I2S.read(samples, sizeof(samples));
  xSemaphoreGive(g_mic_lock);
  if (bytes <= 0) {
    return;
  }
  const size_t count = static_cast<size_t>(bytes) / sizeof(int16_t);
  if (count == 0) {
    return;
  }
  uint64_t sum = 0;
  int16_t peak = 0;
  for (size_t i = 0; i < count; ++i) {
    const int32_t v = samples[i];
    sum += static_cast<uint64_t>(v * v);
    const int16_t abs_v = static_cast<int16_t>(abs(v));
    if (abs_v > peak) {
      peak = abs_v;
    }
  }
  const float rms = sqrtf(static_cast<float>(sum) / static_cast<float>(count)) / 32768.0f;
  const float norm = clampf(rms * 4.0f, 0.0f, 1.0f);
  const float smoothed = g_mic_level * 0.8f + norm * 0.2f;
  g_mic_level = smoothed;
}

bool init_gnss() {
  g_gnss_lat = NAN;
  g_gnss_lon = NAN;
  g_gnss_no_data_warned = false;
  g_gnss_last_fix_ms = 0;
  g_gnss_last_time_ms = 0;
  log_event("GNSS_DIRECT_UART");
  start_gnss_baud_probe(0, millis(), true);
  return true;
}

void update_gnss() {
  bool saw_serial = false;
  while (GNSS.available()) {
    saw_serial = true;
    g_gps.encode(static_cast<char>(GNSS.read()));
  }

  const uint32_t now = millis();
  if (saw_serial) {
    g_gnss_serial_seen = true;
  }
  g_gnss_passed_checksum = g_gps.passedChecksum();
  g_gnss_failed_checksum = g_gps.failedChecksum();
  if (g_gnss_passed_checksum > 0) {
    if (!g_gnss_data_seen) {
      log_gnss_baud_event("GNSS_NMEA_", g_gnss_active_baud);
    }
    g_gnss_data_seen = true;
    g_gnss_baud_locked = true;
    g_gnss_last_valid_sentence_ms = now;
  }
#if GNSS_AUTOSCAN_BAUD
  if (!g_gnss_baud_locked && (now - g_gnss_baud_probe_start_ms) >= GNSS_BAUD_PROBE_WINDOW_MS && g_gnss_passed_checksum == 0) {
    advance_gnss_baud_probe(now);
    return;
  }
  if (g_gnss_baud_locked &&
      g_gnss_last_valid_sentence_ms > 0 &&
      (now - g_gnss_last_valid_sentence_ms) >= GNSS_VALID_SENTENCE_TIMEOUT_MS) {
    log_event("GNSS_NMEA_LOST");
    start_gnss_baud_probe(0, now, false);
    return;
  }
#endif
  g_gnss_satellites = gnss_satellite_count();
  g_gnss_hdop = gnss_hdop_value();
  g_gnss_fix_age_ms = g_gps.location.isValid() ? g_gps.location.age() : 0xFFFFFFFFUL;
  g_gnss_quality = gnss_quality_score();
  const bool reliable_fix = gnss_has_reliable_fix();
  if (!g_gnss_data_seen && now > 5000 && !g_gnss_no_data_warned) {
    g_gnss_no_data_warned = true;
    log_event(g_gnss_serial_seen ? "GNSS_SERIAL_NO_NMEA" : "GNSS_NO_DATA");
  }

  if (g_gps.location.isValid()) {
    g_gnss_lat = g_gps.location.lat();
    g_gnss_lon = g_gps.location.lng();
    if (g_gps.location.isUpdated() && reliable_fix) {
      const uint32_t sample_ms = now;
      if (!isnan(g_gnss_prev_lat) && !isnan(g_gnss_prev_lon) && g_gnss_prev_fix_sample_ms > 0 && sample_ms > g_gnss_prev_fix_sample_ms) {
        const uint32_t elapsed_ms = sample_ms - g_gnss_prev_fix_sample_ms;
        if (elapsed_ms > GNSS_SPEED_SAMPLE_MAX_INTERVAL_MS) {
          g_gnss_speed_ms = 0.0f;
          g_gnss_prev_lat = g_gnss_lat;
          g_gnss_prev_lon = g_gnss_lon;
          g_gnss_prev_fix_sample_ms = sample_ms;
        } else if (elapsed_ms >= GNSS_SPEED_SAMPLE_MIN_INTERVAL_MS) {
          const double elapsed_s = static_cast<double>(elapsed_ms) / 1000.0;
          const double distance_m = haversine_distance_m(g_gnss_prev_lat, g_gnss_prev_lon, g_gnss_lat, g_gnss_lon);
          const float computed_speed = distance_m <= GNSS_SPEED_DISTANCE_NOISE_M
                                           ? 0.0f
                                           : static_cast<float>(distance_m / elapsed_s);
          if (computed_speed >= 0.0f && computed_speed <= GNSS_SPEED_MAX_MPS) {
            if (g_gnss_speed_ms < 0.0f) {
              g_gnss_speed_ms = computed_speed;
            } else {
              g_gnss_speed_ms =
                  g_gnss_speed_ms * (1.0f - GNSS_SPEED_BLEND_NEW) + computed_speed * GNSS_SPEED_BLEND_NEW;
            }
          }
          g_gnss_prev_lat = g_gnss_lat;
          g_gnss_prev_lon = g_gnss_lon;
          g_gnss_prev_fix_sample_ms = sample_ms;
        }
      } else {
        g_gnss_speed_ms = 0.0f;
        g_gnss_prev_lat = g_gnss_lat;
        g_gnss_prev_lon = g_gnss_lon;
        g_gnss_prev_fix_sample_ms = sample_ms;
      }
      g_gnss_last_fix_ms = sample_ms;
    } else if (g_gps.location.isUpdated()) {
      reset_gnss_speed_tracking();
    }
  }

  if (GNSS_TIME_SYNC_ENABLE &&
      !g_ntp_started &&
      WiFi.status() != WL_CONNECTED &&
      reliable_fix &&
      g_gps.date.isValid() &&
      g_gps.time.isValid() &&
      g_gps.date.year() >= 2024 &&
      g_gps.date.year() <= 2099) {
    if (now - g_gnss_last_time_ms >= GNSS_TIME_SYNC_INTERVAL_MS) {
      struct tm t;
      memset(&t, 0, sizeof(t));
      t.tm_year = g_gps.date.year() - 1900;
      t.tm_mon = g_gps.date.month() - 1;
      t.tm_mday = g_gps.date.day();
      t.tm_hour = g_gps.time.hour();
      t.tm_min = g_gps.time.minute();
      t.tm_sec = g_gps.time.second();
      time_t epoch = utc_epoch_from_tm(t);
      if (epoch != static_cast<time_t>(-1)) {
        struct timeval tv;
        tv.tv_sec = epoch;
        tv.tv_usec = 0;
        settimeofday(&tv, nullptr);
        g_gnss_last_time_ms = now;
      }
    }
  }

  const bool has_position = g_gnss_data_seen &&
                            g_gps.location.isValid() &&
                            g_gps.location.age() < GNSS_FIX_TIMEOUT_MS;
  g_gnss_has_fix = has_position;
  if (!g_gnss_has_fix) {
    reset_gnss_speed_tracking();
  }
}
