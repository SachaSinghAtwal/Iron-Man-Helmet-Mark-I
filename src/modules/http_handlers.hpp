// Implementation fragment included by src/main.cpp inside the HUD anonymous namespace.
static const char *STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
static const char *STREAM_BOUNDARY = "\r\n--frame\r\n";
static const char *STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = nullptr;
  esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, OPTIONS");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
  g_streaming = true;

  while (true) {
    bool lock_taken = false;
    if (g_camera_lock) {
      if (xSemaphoreTake(g_camera_lock, pdMS_TO_TICKS(30)) != pdTRUE) {
        delay(2);
        continue;
      }
      lock_taken = true;
    }
    fb = esp_camera_fb_get();
    if (!fb) {
      g_camera_fail_count++;
      if (lock_taken) {
        xSemaphoreGive(g_camera_lock);
      }
      if (g_camera_fail_count >= 3) {
        g_camera_fail_count = 0;
        if (!recover_camera()) {
          res = ESP_FAIL;
          break;
        }
      }
      delay(10);
      continue;
    } else {
      g_camera_fail_count = 0;
      uint8_t *jpg_buf = fb->buf;
      size_t jpg_len = fb->len;

      if (fb->format != PIXFORMAT_JPEG) {
        bool jpeg_converted = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
        esp_camera_fb_return(fb);
        fb = nullptr;
        if (!jpeg_converted) {
          res = ESP_FAIL;
        }
      }

      if (res == ESP_OK) {
        char part_buf[64];
        size_t hlen = snprintf(part_buf, sizeof(part_buf), STREAM_PART, jpg_len);
        res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
        if (res == ESP_OK) {
          res = httpd_resp_send_chunk(req, part_buf, hlen);
        }
        if (res == ESP_OK) {
          res = httpd_resp_send_chunk(req, (const char *)jpg_buf, jpg_len);
        }
        store_clip_frame(jpg_buf, jpg_len);
      }

      if (fb) {
        esp_camera_fb_return(fb);
        fb = nullptr;
      }
      if (lock_taken) {
        xSemaphoreGive(g_camera_lock);
      }

      uint32_t now = millis();
      g_frame_count++;
      if (now - g_last_fps_ts >= 1000) {
        g_fps = (g_frame_count * 1000) / (now - g_last_fps_ts);
        g_frame_count = 0;
        g_last_fps_ts = now;
      }
    }

    delay(1);
    if (res != ESP_OK) {
      break;
    }
  }
  g_streaming = false;
  return res;
}

esp_err_t snapshot_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  bool lock_taken = false;
  if (g_camera_lock) {
    if (xSemaphoreTake(g_camera_lock, pdMS_TO_TICKS(80)) != pdTRUE) {
      return httpd_resp_send_500(req);
    }
    lock_taken = true;
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    if (lock_taken) {
      xSemaphoreGive(g_camera_lock);
    }
    return httpd_resp_send_500(req);
  }

  uint8_t *jpg_buf = fb->buf;
  size_t jpg_len = fb->len;
  bool converted = false;
  if (fb->format != PIXFORMAT_JPEG) {
    if (!frame2jpg(fb, 80, &jpg_buf, &jpg_len)) {
      esp_camera_fb_return(fb);
      if (lock_taken) {
        xSemaphoreGive(g_camera_lock);
      }
      return httpd_resp_send_500(req);
    }
    esp_camera_fb_return(fb);
    fb = nullptr;
    converted = true;
  }

  store_clip_frame(jpg_buf, jpg_len);
  esp_err_t res = httpd_resp_send(req, (const char *)jpg_buf, jpg_len);
  if (converted) {
    free(jpg_buf);
  } else if (fb) {
    esp_camera_fb_return(fb);
  }
  if (lock_taken) {
    xSemaphoreGive(g_camera_lock);
  }
  return res;
}

esp_err_t clip_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  struct ClipIndex {
    uint8_t idx;
    uint32_t t;
  };
  std::vector<ClipIndex> frames;
  const uint32_t now = millis();

  if (g_clip_lock && xSemaphoreTake(g_clip_lock, pdMS_TO_TICKS(60)) == pdTRUE) {
    for (uint8_t i = 0; i < CLIP_RING_CAP; ++i) {
      const ClipFrame &frame = g_clip_frames[i];
      if (frame.data && frame.len > 0 && (now - frame.t) <= CLIP_WINDOW_MS) {
        frames.push_back({i, frame.t});
      }
    }
    xSemaphoreGive(g_clip_lock);
  }

  std::sort(frames.begin(), frames.end(), [](const ClipIndex &a, const ClipIndex &b) {
    return a.t < b.t;
  });

  if (frames.size() < 3) {
    const char *json = "{\"ok\":false,\"error\":\"clip_unavailable\"}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  }

  esp_err_t res = httpd_resp_send_chunk(req, "{\"ok\":true,\"frame_ms\":", HTTPD_RESP_USE_STRLEN);
  char meta[80];
  snprintf(meta, sizeof(meta), "%lu,\"frames\":[", static_cast<unsigned long>(CLIP_STORE_INTERVAL_MS));
  if (res == ESP_OK) {
    res = httpd_resp_send_chunk(req, meta, HTTPD_RESP_USE_STRLEN);
  }

  bool first = true;
  for (const ClipIndex &item : frames) {
    uint8_t *copy = nullptr;
    size_t len = 0;
    uint32_t captured = 0;

    if (g_clip_lock && xSemaphoreTake(g_clip_lock, pdMS_TO_TICKS(60)) == pdTRUE) {
      const ClipFrame &source = g_clip_frames[item.idx];
      if (source.data && source.len > 0 && source.t == item.t) {
        len = source.len;
        captured = source.t;
        copy = static_cast<uint8_t *>(ps_malloc(len));
        if (!copy) {
          copy = static_cast<uint8_t *>(malloc(len));
        }
        if (copy) {
          memcpy(copy, source.data, len);
        }
      }
      xSemaphoreGive(g_clip_lock);
    }

    if (!copy) {
      continue;
    }

    if (res == ESP_OK && !first) {
      res = httpd_resp_send_chunk(req, ",", HTTPD_RESP_USE_STRLEN);
    }
    if (res == ESP_OK) {
      ClipFrame temp;
      temp.data = copy;
      temp.len = len;
      temp.t = captured;
      if (!send_base64_frame_chunk(req, temp)) {
        res = ESP_FAIL;
      }
    }
    free(copy);
    first = false;
    if (res != ESP_OK) {
      break;
    }
  }

  if (res == ESP_OK) {
    res = httpd_resp_send_chunk(req, "]}", HTTPD_RESP_USE_STRLEN);
  }
  if (res == ESP_OK) {
    res = httpd_resp_send_chunk(req, nullptr, 0);
  }
  return res;
}

esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

esp_err_t status_handler(httpd_req_t *req) {
  const bool wifi_ok = (WiFi.status() == WL_CONNECTED);
  const int rssi = wifi_ok ? WiFi.RSSI() : -127;
  const int wifi_quality = wifi_ok ? wifi_quality_from_rssi(rssi) : 0;
  const bool ap_mode = (WiFi.getMode() & WIFI_AP) != 0;
  const int ap_clients = ap_mode ? WiFi.softAPgetStationNum() : 0;
  const int stream_port = g_stream_httpd ? 81 : 80;
  const uint32_t uptime_sec = millis() / 1000;
  const String ip = wifi_ok ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  const float soc_temp = read_soc_temp_c();
  const bool temp_valid = !isnan(soc_temp);
  const float power_pct = read_power_bank_percent();
  float room_temp_c = -1.0f;
  bool room_temp_est = false;
  if (temp_valid) {
    room_temp_c = clampf(soc_temp - 12.0f, 10.0f, 45.0f);
    room_temp_est = true;
  }
  String voice_text;
  uint32_t voice_seq = 0;
  if (g_voice_lock && xSemaphoreTake(g_voice_lock, pdMS_TO_TICKS(20)) == pdTRUE) {
    voice_text = g_voice_text;
    voice_seq = g_voice_seq;
    xSemaphoreGive(g_voice_lock);
  }

  const uint32_t heap_free = ESP.getFreeHeap();
  const uint32_t heap_total = ESP.getHeapSize();
  const uint32_t psram_total = ESP.getPsramSize();
  const uint32_t psram_free = ESP.getFreePsram();

  const float heap_used = heap_total > 0 ? 1.0f - (float)heap_free / (float)heap_total : 0.0f;
  const float psram_used = psram_total > 0 ? 1.0f - (float)psram_free / (float)psram_total : 0.0f;
  const float uptime_phase = (uptime_sec % 3600) / 3600.0f;
  const float fps_norm = clampf((float)g_fps / 15.0f, 0.0f, 1.0f);
  const float temp_norm = temp_valid ? clampf((soc_temp - 25.0f) / 40.0f, 0.0f, 1.0f) : 0.0f;

  float speed_ms = -1.0f;
  float speed_mph = -1.0f;
  int loc_quality = 0;
  String lat_str = "";
  String lon_str = "";
  if (g_gnss_has_fix) {
    lat_str = String(g_gnss_lat, 6);
    lon_str = String(g_gnss_lon, 6);
    speed_ms = g_gnss_speed_ms >= 0 ? g_gnss_speed_ms : 0.0f;
    speed_mph = speed_ms * 2.23694f;
    loc_quality = g_gnss_quality;
  } else if (!isnan(g_gnss_lat) && !isnan(g_gnss_lon)) {
    lat_str = String(g_gnss_lat, 6);
    lon_str = String(g_gnss_lon, 6);
    loc_quality = g_gnss_quality > 0 ? std::min(g_gnss_quality, 45) : 40;
  } else if (String(HUD_LATITUDE).length() > 0 && String(HUD_LONGITUDE).length() > 0) {
    lat_str = String(HUD_LATITUDE);
    lon_str = String(HUD_LONGITUDE);
    loc_quality = 60;
  }

  String json;
  json.reserve(1024);
  json += "{";
  json += "\"time\":\"" + format_time() + "\",";
  json += "\"uptime\":\"" + format_uptime(uptime_sec) + "\",";
  json += "\"ip\":\"" + ip + "\",";
  json += "\"link\":\"" + String(wifi_ok ? "STABLE" : "AP_MODE") + "\",";
  json += "\"stream_port\":" + String(stream_port) + ",";
  json += "\"wifi_quality\":" + String(wifi_quality) + ",";
  json += "\"ap_clients\":" + String(ap_clients) + ",";
  json += "\"power_pct\":" + String(power_pct, 1) + ",";
  json += "\"lat\":\"" + lat_str + "\",";
  json += "\"lon\":\"" + lon_str + "\",";
  json += "\"cpu_mhz\":" + String(ESP.getCpuFreqMHz()) + ",";
  json += "\"fps\":" + String(g_fps) + ",";
  json += "\"heart_bpm\":" + String(read_heart_rate_bpm()) + ",";
  json += "\"mic_level\":" + String(g_mic_level, 2) + ",";
  json += "\"mic_ok\":" + String(g_mic_ok ? "true" : "false") + ",";
  json += "\"stt_ok\":" + String(g_stt_ok ? "true" : "false") + ",";
  json += "\"tts_ok\":" + String(g_tts_ok ? "true" : "false") + ",";
  json += "\"speaker_ok\":" + String(g_speaker_ok ? "true" : "false") + ",";
  json += "\"volume\":" + String(g_volume_percent) + ",";
  json += "\"mask\":{\"ok\":" + String(g_mask_ok ? "true" : "false");
  json += ",\"state\":\"" + String(g_mask_moving ? "moving" : (g_mask_open ? "open" : "closed")) + "\"";
  json += ",\"moving\":" + String(g_mask_moving ? "true" : "false") + "},";
  json += "\"voice_seq\":" + String(voice_seq) + ",";
  json += "\"voice_text\":\"" + json_escape(voice_text) + "\",";
  json += "\"nav\":{";
  json += "\"speed_mph\":" + String(speed_mph, 2) + ",";
  json += "\"speed_ms\":" + String(speed_ms, 2) + ",";
  json += "\"loc_quality\":" + String(loc_quality) + ",";
  json += "\"fix\":" + String(g_gnss_has_fix ? "true" : "false") + ",";
  json += "\"signal\":" + String(g_gnss_data_seen ? "true" : "false") + ",";
  json += "\"serial\":" + String(g_gnss_serial_seen ? "true" : "false") + ",";
  json += "\"baud\":" + String(g_gnss_active_baud) + ",";
  json += "\"baud_locked\":" + String(g_gnss_baud_locked ? "true" : "false") + ",";
  json += "\"checksums_ok\":" + String(g_gnss_passed_checksum) + ",";
  json += "\"checksums_bad\":" + String(g_gnss_failed_checksum) + ",";
  json += "\"satellites\":" + String(g_gnss_satellites) + ",";
  json += "\"hdop\":";
  if (isnan(g_gnss_hdop)) {
    json += "-1";
  } else {
    json += String(g_gnss_hdop, 1);
  }
  json += ",";
  json += "\"age_ms\":";
  if (g_gnss_fix_age_ms == 0xFFFFFFFFUL) {
    json += "-1";
  } else {
    json += String(g_gnss_fix_age_ms);
  }
  json += "},";
  json += "\"atmos\":{";
  json += "\"o2\":-1,";
  json += "\"temp_c\":" + String(temp_valid ? soc_temp : -1) + ",";
  json += "\"room_temp_c\":" + String(room_temp_c, 1) + ",";
  json += "\"room_temp_est\":" + String(room_temp_est ? "true" : "false");
  json += "},";
  json += "\"levels\":[";
  json += String(wifi_quality / 100.0f, 2) + ",";
  json += String(heap_used, 2) + ",";
  json += String(psram_used, 2) + ",";
  json += String(fps_norm, 2) + ",";
  json += String(temp_norm, 2) + ",";
  json += String(uptime_phase, 2) + ",";
  json += String(heap_free > 0 ? clampf((float)heap_free / 300000.0f, 0.05f, 1.0f) : 0.1f, 2);
  json += "],";
  json += "\"logs\":[";
  for (size_t i = 0; i < LOG_CAP; ++i) {
    size_t idx = (g_log_head + i) % LOG_CAP;
    if (g_logs[idx].length() == 0) continue;
    json += "\"" + g_logs[idx] + "\"";
    bool has_more = false;
    for (size_t j = i + 1; j < LOG_CAP; ++j) {
      size_t idx2 = (g_log_head + j) % LOG_CAP;
      if (g_logs[idx2].length() != 0) {
        has_more = true;
        break;
      }
    }
    if (has_more) {
      json += ",";
    }
  }
  json += "]";
  if (g_voice_host.length() > 0) {
    json += ",\"voice_host\":\"" + g_voice_host + "\",\"voice_port\":" + String(g_voice_port);
  }
  json += "}";

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, json.c_str(), json.length());
}

esp_err_t ai_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
#if HAS_SSCMA
  if (!g_ai_ok) {
    const char *json = "{\"ok\":false,\"error\":\"ai_disabled\"}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  }
  if (!g_ai_lock || xSemaphoreTake(g_ai_lock, pdMS_TO_TICKS(10)) != pdTRUE) {
    const char *json = "{\"ok\":false,\"error\":\"ai_busy\"}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  }

  String json;
  json.reserve(2048);
  json += "{";
  json += "\"ok\":true,";
  json += "\"ts\":" + String(g_ai_last_update_ms) + ",";
  json += "\"frame_w\":" + String(g_ai_frame_w) + ",";
  json += "\"frame_h\":" + String(g_ai_frame_h) + ",";

  json += "\"boxes\":[";
  for (size_t i = 0; i < g_ai_boxes.size(); ++i) {
    const auto &b = g_ai_boxes[i];
    json += "{\"x\":" + String(b.x, 4) +
            ",\"y\":" + String(b.y, 4) +
            ",\"w\":" + String(b.w, 4) +
            ",\"h\":" + String(b.h, 4) +
            ",\"score\":" + String(b.score, 3) +
            ",\"target\":" + String(b.target) + "}";
    if (i + 1 < g_ai_boxes.size()) json += ",";
  }
  json += "],";

  json += "\"classes\":[";
  for (size_t i = 0; i < g_ai_classes.size(); ++i) {
    const auto &c = g_ai_classes[i];
    json += "{\"target\":" + String(c.target) +
            ",\"score\":" + String(c.score, 3) + "}";
    if (i + 1 < g_ai_classes.size()) json += ",";
  }
  json += "],";

  json += "\"points\":[";
  for (size_t i = 0; i < g_ai_points.size(); ++i) {
    const auto &p = g_ai_points[i];
    json += "{\"x\":" + String(p.x, 4) +
            ",\"y\":" + String(p.y, 4) +
            ",\"z\":" + String(p.z, 4) +
            ",\"score\":" + String(p.score, 3) +
            ",\"target\":" + String(p.target) + "}";
    if (i + 1 < g_ai_points.size()) json += ",";
  }
  json += "],";

  json += "\"keypoints\":[";
  for (size_t i = 0; i < g_ai_keypoints.size(); ++i) {
    const auto &k = g_ai_keypoints[i];
    json += "{\"box\":{";
    json += "\"x\":" + String(k.box.x, 4) +
            ",\"y\":" + String(k.box.y, 4) +
            ",\"w\":" + String(k.box.w, 4) +
            ",\"h\":" + String(k.box.h, 4) +
            ",\"score\":" + String(k.box.score, 3) +
            ",\"target\":" + String(k.box.target) + "},";
    json += "\"points\":[";
    for (size_t j = 0; j < k.points.size(); ++j) {
      const auto &pt = k.points[j];
      json += "{\"x\":" + String(pt.x, 4) +
              ",\"y\":" + String(pt.y, 4) +
              ",\"z\":" + String(pt.z, 4) +
              ",\"score\":" + String(pt.score, 3) +
              ",\"target\":" + String(pt.target) + "}";
      if (j + 1 < k.points.size()) json += ",";
    }
    json += "]}";
    if (i + 1 < g_ai_keypoints.size()) json += ",";
  }
  json += "]";
  json += "}";

  xSemaphoreGive(g_ai_lock);
  return httpd_resp_send(req, json.c_str(), json.length());
#else
  const char *json = "{\"ok\":false,\"error\":\"ai_unavailable\"}";
  return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
#endif
}

esp_err_t helmet_handler(httpd_req_t *req) {
  File file = LittleFS.open("/holo.mp4", "r");
  if (!file) {
    return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "holo.mp4 missing");
  }
  httpd_resp_set_type(req, "video/mp4");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  static const size_t kChunk = 1024;
  uint8_t buffer[kChunk];
  while (file.available()) {
    size_t len = file.readBytes((char *)buffer, kChunk);
    if (len == 0) break;
    esp_err_t res = httpd_resp_send_chunk(req, (const char *)buffer, len);
    if (res != ESP_OK) {
      file.close();
      return res;
    }
  }
  file.close();
  return httpd_resp_send_chunk(req, nullptr, 0);
}

esp_err_t voice_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  if (!g_mic_ok) {
    const char *json = "{\"ok\":false,\"error\":\"mic_unavailable\"}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  }
  if (!g_stt_ok) {
    discover_voice_server();
  }
  if (!g_stt_ok) {
    const char *json = "{\"ok\":false,\"error\":\"stt_not_configured\"}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  }
  if (g_voice_busy) {
    const char *json = "{\"ok\":false,\"error\":\"voice_busy\"}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  }

  String transcript;
  float level = 0.0f;
  uint32_t samples = 0;
  g_voice_busy = true;
  bool ok = capture_and_transcribe(transcript, level, samples);
  g_voice_busy = false;
  String json;
  if (ok) {
    json = "{\"ok\":true,\"transcript\":\"";
    transcript.replace("\\", "\\\\");
    transcript.replace("\"", "\\\"");
    json += transcript;
    json += "\",\"level\":";
    json += String(level, 2);
    json += ",\"samples\":";
    json += String(samples);
    json += "}";
  } else {
    json = "{\"ok\":false,\"error\":\"stt_failed\",\"level\":";
    json += String(level, 2);
    json += ",\"samples\":";
    json += String(samples);
    json += "}";
  }
  return httpd_resp_send(req, json.c_str(), json.length());
}

esp_err_t speak_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  if (!g_tts_ok) {
    discover_voice_server();
  }
  if (!g_tts_ok) {
    const char *json = "{\"ok\":false,\"error\":\"tts_not_configured\"}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  }

  const int total = req->content_len;
  if (total <= 0) {
    const char *json = "{\"ok\":false,\"error\":\"empty\"}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  }

  String text;
  text.reserve(std::min(total, 256));
  int remaining = total;
  char buffer[128];
  while (remaining > 0) {
    const int to_read = remaining > static_cast<int>(sizeof(buffer)) ? static_cast<int>(sizeof(buffer)) : remaining;
    const int received = httpd_req_recv(req, buffer, to_read);
    if (received <= 0) {
      break;
    }
    text.concat(buffer, received);
    remaining -= received;
  }
  text.trim();
  if (text.length() == 0) {
    const char *json = "{\"ok\":false,\"error\":\"empty\"}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  }

  if (enqueue_speech_text(text)) {
    const char *json = "{\"ok\":true}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  }
  const char *json = "{\"ok\":false,\"error\":\"tts_queue_full\"}";
  return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

esp_err_t volume_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  const int total = req->content_len;
  if (total <= 0) {
    const char *json = "{\"ok\":false,\"error\":\"empty\"}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  }

  String body;
  body.reserve(std::min(total, 16));
  int remaining = total;
  char buffer[16];
  while (remaining > 0) {
    const int to_read = remaining > static_cast<int>(sizeof(buffer)) ? static_cast<int>(sizeof(buffer)) : remaining;
    const int received = httpd_req_recv(req, buffer, to_read);
    if (received <= 0) {
      break;
    }
    body.concat(buffer, received);
    remaining -= received;
  }
  body.trim();
  if (body.length() == 0) {
    const char *json = "{\"ok\":false,\"error\":\"empty\"}";
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
  }

  const int value = body.toInt();
  set_volume_percent(value);
  String json = "{\"ok\":true,\"volume\":";
  json += String(g_volume_percent);
  json += "}";
  return httpd_resp_send(req, json.c_str(), json.length());
}

esp_err_t mask_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  char query[80] = {0};
  char action_buf[24] = {0};
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
    httpd_query_key_value(query, "action", action_buf, sizeof(action_buf));
  }

  String action(action_buf);
  action.toLowerCase();
  action.trim();

  bool requested_move = false;
  bool target_open = g_mask_open;
  if (action == "open" || action == "off" || action == "up" || action == "raise" || action == "lift") {
    requested_move = true;
    target_open = true;
  } else if (action == "close" || action == "closed" || action == "on" || action == "down" || action == "lower" || action == "shut") {
    requested_move = true;
    target_open = false;
  } else if (action == "toggle" || action == "cycle") {
    requested_move = true;
    target_open = !g_mask_open;
  }

  bool ok = g_mask_ok;
  if (requested_move) {
    ok = mask_start_move(target_open);
  } else if (!ok) {
    ok = init_mask_servos();
  }

  String json = "{\"ok\":";
  json += ok ? "true" : "false";
  json += ",\"state\":\"";
  json += requested_move ? (target_open ? "open" : "closed") : (g_mask_moving ? "moving" : (g_mask_open ? "open" : "closed"));
  json += "\",\"moving\":";
  json += g_mask_moving ? "true" : "false";
  if (!ok) {
    json += ",\"error\":\"pwm_driver_not_detected\"";
  }
  json += "}";
  return httpd_resp_send(req, json.c_str(), json.length());
}

bool start_stream_server(int ctrl_port_base) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;
  config.ctrl_port = ctrl_port_base + 1;
  config.max_uri_handlers = 4;

  if (httpd_start(&g_stream_httpd, &config) != ESP_OK) {
    g_stream_httpd = nullptr;
    return false;
  }

  httpd_uri_t stream_uri = {
      .uri = "/stream",
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = nullptr};
  httpd_register_uri_handler(g_stream_httpd, &stream_uri);
  return true;
}

bool start_webserver() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 14;

  if (httpd_start(&g_httpd, &config) != ESP_OK) {
    return false;
  }

  httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
      .user_ctx = nullptr};
  httpd_register_uri_handler(g_httpd, &index_uri);

  httpd_uri_t status_uri = {
      .uri = "/status",
      .method = HTTP_GET,
      .handler = status_handler,
      .user_ctx = nullptr};
  httpd_register_uri_handler(g_httpd, &status_uri);

  httpd_uri_t snapshot_uri = {
      .uri = "/snapshot",
      .method = HTTP_GET,
      .handler = snapshot_handler,
      .user_ctx = nullptr};
  httpd_register_uri_handler(g_httpd, &snapshot_uri);

  httpd_uri_t clip_uri = {
      .uri = "/clip",
      .method = HTTP_GET,
      .handler = clip_handler,
      .user_ctx = nullptr};
  httpd_register_uri_handler(g_httpd, &clip_uri);

  httpd_uri_t ai_uri = {
      .uri = "/ai",
      .method = HTTP_GET,
      .handler = ai_handler,
      .user_ctx = nullptr};
  httpd_register_uri_handler(g_httpd, &ai_uri);

  httpd_uri_t helmet_uri = {
      .uri = "/holo.mp4",
      .method = HTTP_GET,
      .handler = helmet_handler,
      .user_ctx = nullptr};
  httpd_register_uri_handler(g_httpd, &helmet_uri);

  httpd_uri_t voice_uri = {
      .uri = "/voice",
      .method = HTTP_POST,
      .handler = voice_handler,
      .user_ctx = nullptr};
  httpd_register_uri_handler(g_httpd, &voice_uri);

  httpd_uri_t speak_uri = {
      .uri = "/speak",
      .method = HTTP_POST,
      .handler = speak_handler,
      .user_ctx = nullptr};
  httpd_register_uri_handler(g_httpd, &speak_uri);

  httpd_uri_t volume_uri = {
      .uri = "/volume",
      .method = HTTP_POST,
      .handler = volume_handler,
      .user_ctx = nullptr};
  httpd_register_uri_handler(g_httpd, &volume_uri);

  httpd_uri_t mask_uri = {
      .uri = "/mask",
      .method = HTTP_GET,
      .handler = mask_handler,
      .user_ctx = nullptr};
  httpd_register_uri_handler(g_httpd, &mask_uri);

  if (!start_stream_server(config.ctrl_port)) {
    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = nullptr};
    httpd_register_uri_handler(g_httpd, &stream_uri);
  }

  return true;
}
}  // namespace
