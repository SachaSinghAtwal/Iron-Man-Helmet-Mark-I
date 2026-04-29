// Implementation fragment included by src/main.cpp inside the HUD anonymous namespace.
String json_escape(const String &input) {
  String out;
  out.reserve(input.length() + 8);
  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input[i];
    if (c == '\\') {
      out += "\\\\";
    } else if (c == '"') {
      out += "\\\"";
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else {
      out += c;
    }
  }
  return out;
}

void store_clip_frame(const uint8_t *jpg_buf, size_t jpg_len) {
  if (!jpg_buf || jpg_len == 0 || jpg_len > CLIP_MAX_FRAME_BYTES) {
    return;
  }
  const uint32_t now = millis();
  if (now - g_clip_last_store_ms < CLIP_STORE_INTERVAL_MS) {
    return;
  }

  uint8_t *copy = static_cast<uint8_t *>(ps_malloc(jpg_len));
  if (!copy) {
    copy = static_cast<uint8_t *>(malloc(jpg_len));
  }
  if (!copy) {
    return;
  }
  memcpy(copy, jpg_buf, jpg_len);

  if (g_clip_lock && xSemaphoreTake(g_clip_lock, pdMS_TO_TICKS(10)) != pdTRUE) {
    free(copy);
    return;
  }

  ClipFrame &slot = g_clip_frames[g_clip_next];
  if (slot.data) {
    free(slot.data);
  }
  slot.data = copy;
  slot.len = jpg_len;
  slot.t = now;
  g_clip_next = (g_clip_next + 1) % CLIP_RING_CAP;
  g_clip_last_store_ms = now;

  if (g_clip_lock) {
    xSemaphoreGive(g_clip_lock);
  }
}

bool send_base64_frame_chunk(httpd_req_t *req, const ClipFrame &frame) {
  if (!frame.data || frame.len == 0) {
    return true;
  }

  size_t out_len = 0;
  (void)mbedtls_base64_encode(nullptr, 0, &out_len, frame.data, frame.len);
  char *encoded = static_cast<char *>(ps_malloc(out_len + 1));
  if (!encoded) {
    encoded = static_cast<char *>(malloc(out_len + 1));
  }
  if (!encoded) {
    return false;
  }

  const int rc = mbedtls_base64_encode(
      reinterpret_cast<unsigned char *>(encoded),
      out_len + 1,
      &out_len,
      frame.data,
      frame.len);
  if (rc != 0) {
    free(encoded);
    return false;
  }
  encoded[out_len] = '\0';

  esp_err_t res = httpd_resp_send_chunk(req, "\"data:image/jpeg;base64,", HTTPD_RESP_USE_STRLEN);
  if (res == ESP_OK) {
    res = httpd_resp_send_chunk(req, encoded, out_len);
  }
  if (res == ESP_OK) {
    res = httpd_resp_send_chunk(req, "\"", HTTPD_RESP_USE_STRLEN);
  }

  free(encoded);
  return res == ESP_OK;
}

void set_voice_text(const String &text) {
  if (!g_voice_lock) {
    return;
  }
  if (xSemaphoreTake(g_voice_lock, pdMS_TO_TICKS(20)) != pdTRUE) {
    return;
  }
  g_voice_text = text;
  g_voice_seq++;
  xSemaphoreGive(g_voice_lock);
}

bool init_camera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  g_camera_ok = false;
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    log_event(String("CAM_INIT_ERR_0x") + String(static_cast<uint32_t>(err), HEX));
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (!sensor) {
    log_event("CAM_SENSOR_MISSING");
    esp_camera_deinit();
    return false;
  }

  String sensor_name = "UNKNOWN";
  if (sensor->id.PID == OV2640_PID) {
    sensor_name = "OV2640";
  } else if (sensor->id.PID == OV3660_PID) {
    sensor_name = "OV3660";
  } else if (sensor->id.PID == OV5640_PID) {
    sensor_name = "OV5640";
  }
  log_event(String("CAM_SENSOR_") + sensor_name + "_0x" + String(sensor->id.PID, HEX));

  if (sensor->id.PID == OV5640_PID) {
    sensor->set_framesize(sensor, psramFound() ? FRAMESIZE_VGA : FRAMESIZE_QVGA);
    sensor->set_quality(sensor, psramFound() ? 12 : 15);
    sensor->set_brightness(sensor, 0);
    sensor->set_saturation(sensor, -1);
  } else if (sensor->id.PID == OV3660_PID) {
    sensor->set_vflip(sensor, 1);
    sensor->set_brightness(sensor, 1);
    sensor->set_saturation(sensor, -2);
  }

  g_camera_ok = true;
  return true;
}

bool recover_camera() {
  if (g_camera_recovering) {
    return false;
  }
  g_camera_recovering = true;
  esp_camera_deinit();
  delay(40);
  const bool ok = init_camera();
  log_event(ok ? "CAM_REINIT_OK" : "CAM_REINIT_FAIL");
  g_camera_recovering = false;
  return ok;
}

void sync_time_if_wifi() {
  if (g_ntp_started || WiFi.status() != WL_CONNECTED) {
    return;
  }

  const uint32_t now = millis();
  if (g_last_ntp_attempt_ms > 0 && (now - g_last_ntp_attempt_ms) < NTP_RETRY_INTERVAL_MS) {
    return;
  }
  g_last_ntp_attempt_ms = now;

  setenv("TZ", TIMEZONE, 1);
  tzset();
  configTzTime(TIMEZONE, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, NTP_SYNC_TIMEOUT_MS)) {
    g_ntp_started = true;
    log_event("NTP_OK");
  } else {
    log_event("NTP_FAIL");
  }
}

bool wait_for_wifi(uint32_t timeout_ms) {
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

bool connect_wifi_once(uint8_t attempt, uint32_t timeout_ms) {
  log_event(String("WIFI_TRY_") + String(attempt));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (wait_for_wifi(timeout_ms)) {
    log_event("WIFI_OK");
    log_event(String("IP_") + WiFi.localIP().toString());
    g_last_wifi_retry_ms = millis();
    return true;
  }
  log_event(String("WIFI_STATUS_") + String(static_cast<int>(WiFi.status())));
  return false;
}

bool connect_wifi() {
  WiFi.persistent(false);
  WiFi.disconnect(true, false);
  delay(250);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  for (uint8_t attempt = 1; attempt <= WIFI_CONNECT_ATTEMPTS; ++attempt) {
    const uint32_t timeout = attempt == 1 ? WIFI_CONNECT_TIMEOUT_MS : WIFI_CONNECT_TIMEOUT_MS / 2;
    if (connect_wifi_once(attempt, timeout)) {
      return true;
    }
    WiFi.disconnect(false, false);
    delay(750);
  }

  log_event("WIFI_FAIL");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);
  log_event("AP_MODE");
  log_event(String("AP_IP_") + WiFi.softAPIP().toString());
  g_last_wifi_retry_ms = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  return false;
}

void maintain_wifi() {
  if (WiFi.status() == WL_CONNECTED) {
    sync_time_if_wifi();
    return;
  }
  const uint32_t now = millis();
  if (now - g_last_wifi_retry_ms < WIFI_RECONNECT_INTERVAL_MS) {
    return;
  }
  g_last_wifi_retry_ms = now;
  if ((WiFi.getMode() & WIFI_AP) == 0) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);
  }
  WiFi.setSleep(false);
  log_event("WIFI_RETRY");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (wait_for_wifi(5000)) {
    log_event("WIFI_RECOVERED");
    log_event(String("IP_") + WiFi.localIP().toString());
    sync_time_if_wifi();
  }
}
