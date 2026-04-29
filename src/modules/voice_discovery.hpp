// Implementation fragment included by src/main.cpp inside the HUD anonymous namespace.
bool probe_voice_server(const IPAddress &ip) {
  WiFiClient client;
  client.setTimeout(300);
  if (!client.connect(ip, LOCAL_VOICE_PORT)) {
    return false;
  }
  client.print("GET /health HTTP/1.1\r\nHost: ");
  client.print(ip.toString());
  client.print("\r\nConnection: close\r\n\r\n");

  String response;
  uint32_t start = millis();
  while (client.connected() && (millis() - start) < 800) {
    while (client.available()) {
      char c = static_cast<char>(client.read());
      response += c;
      if (response.length() > 160) {
        break;
      }
    }
    if (response.length() > 160) {
      break;
    }
    delay(5);
  }
  client.stop();

  return response.indexOf("200") >= 0 || response.indexOf("stt=") >= 0;
}

bool parse_voice_origin(const String &url, String &host_out, uint16_t &port_out) {
  String work = url;
  work.trim();
  if (work.length() == 0) return false;
  if (work.startsWith("http://")) {
    work = work.substring(7);
  } else if (work.startsWith("https://")) {
    work = work.substring(8);
  }
  int slash = work.indexOf('/');
  if (slash >= 0) {
    work = work.substring(0, slash);
  }
  if (work.length() == 0) return false;
  int colon = work.lastIndexOf(':');
  String host = work;
  uint16_t port = LOCAL_VOICE_PORT;
  if (colon >= 0) {
    host = work.substring(0, colon);
    const String port_str = work.substring(colon + 1);
    int parsed = port_str.toInt();
    if (parsed > 0) {
      port = static_cast<uint16_t>(parsed);
    }
  }
  host.trim();
  if (host.length() == 0) return false;
  host_out = host;
  port_out = port;
  return true;
}

void set_voice_origin_from_url(const String &url) {
  String host;
  uint16_t port = LOCAL_VOICE_PORT;
  if (parse_voice_origin(url, host, port)) {
    g_voice_host = host;
    g_voice_port = port;
  }
}

void set_local_voice_base(const IPAddress &ip) {
  const String base = String("http://") + ip.toString() + ":" + String(LOCAL_VOICE_PORT);
  g_local_stt_url = base + "/stt";
  g_local_tts_url = base + "/tts";
  g_voice_host = ip.toString();
  g_voice_port = LOCAL_VOICE_PORT;
  log_event("VOICE_SERVER " + ip.toString());
  update_voice_config();
}

bool discover_voice_server() {
  if (strlen(LOCAL_STT_URL) > 0 || strlen(LOCAL_TTS_URL) > 0) {
    return g_local_stt_url.length() > 0 || g_local_tts_url.length() > 0;
  }
  const uint32_t now = millis();
  if (now - g_last_voice_discovery_ms < VOICE_DISCOVERY_INTERVAL_MS) {
    return false;
  }
  g_last_voice_discovery_ms = now;

  const wifi_mode_t mode = WiFi.getMode();
  const bool ap_mode = (mode & WIFI_AP) != 0;
  if (!ap_mode && WiFi.status() != WL_CONNECTED) {
    return false;
  }
  if (ap_mode && WiFi.softAPgetStationNum() == 0) {
    return false;
  }

  IPAddress base = ap_mode ? WiFi.softAPIP() : WiFi.localIP();
  if (base[0] == 0) {
    return false;
  }

  if (ap_mode) {
    for (uint8_t host = 2; host <= VOICE_DISCOVERY_AP_MAX; ++host) {
      if (host == base[3]) continue;
      IPAddress target(base[0], base[1], base[2], host);
      if (probe_voice_server(target)) {
        set_local_voice_base(target);
        return true;
      }
    }
    return false;
  }

  for (uint8_t i = 0; i < VOICE_DISCOVERY_BATCH; ++i) {
    uint8_t host = g_voice_scan_next++;
    if (g_voice_scan_next > 254) {
      g_voice_scan_next = 2;
    }
    if (host == base[3] || host == 0 || host == 1) {
      continue;
    }
    IPAddress target(base[0], base[1], base[2], host);
    if (probe_voice_server(target)) {
      set_local_voice_base(target);
      return true;
    }
  }
  return false;
}

void update_voice_config() {
  g_stt_ok = strlen(OPENAI_API_KEY) > 0 || g_local_stt_url.length() > 0;
  g_tts_ok = strlen(OPENAI_API_KEY) > 0 || g_local_tts_url.length() > 0;
}
