// Implementation fragment included by src/main.cpp inside the HUD anonymous namespace.
void write_wav_header(uint8_t *header,
                      uint32_t sample_rate,
                      uint16_t bits_per_sample,
                      uint16_t channels,
                      uint32_t data_bytes) {
  const uint32_t byte_rate = sample_rate * channels * (bits_per_sample / 8);
  const uint16_t block_align = channels * (bits_per_sample / 8);
  const uint32_t chunk_size = 36 + data_bytes;

  memcpy(header + 0, "RIFF", 4);
  header[4] = chunk_size & 0xFF;
  header[5] = (chunk_size >> 8) & 0xFF;
  header[6] = (chunk_size >> 16) & 0xFF;
  header[7] = (chunk_size >> 24) & 0xFF;
  memcpy(header + 8, "WAVE", 4);
  memcpy(header + 12, "fmt ", 4);
  header[16] = 16;
  header[17] = 0;
  header[18] = 0;
  header[19] = 0;
  header[20] = 1;
  header[21] = 0;
  header[22] = channels & 0xFF;
  header[23] = (channels >> 8) & 0xFF;
  header[24] = sample_rate & 0xFF;
  header[25] = (sample_rate >> 8) & 0xFF;
  header[26] = (sample_rate >> 16) & 0xFF;
  header[27] = (sample_rate >> 24) & 0xFF;
  header[28] = byte_rate & 0xFF;
  header[29] = (byte_rate >> 8) & 0xFF;
  header[30] = (byte_rate >> 16) & 0xFF;
  header[31] = (byte_rate >> 24) & 0xFF;
  header[32] = block_align & 0xFF;
  header[33] = (block_align >> 8) & 0xFF;
  header[34] = bits_per_sample & 0xFF;
  header[35] = (bits_per_sample >> 8) & 0xFF;
  memcpy(header + 36, "data", 4);
  header[40] = data_bytes & 0xFF;
  header[41] = (data_bytes >> 8) & 0xFF;
  header[42] = (data_bytes >> 16) & 0xFF;
  header[43] = (data_bytes >> 24) & 0xFF;
}

bool openai_transcribe_audio(const int16_t *pcm, size_t sample_count, String &out_text) {
  out_text = "";
  if (!g_stt_ok || strlen(OPENAI_API_KEY) == 0) {
    return false;
  }

  const uint32_t data_bytes = static_cast<uint32_t>(sample_count * sizeof(int16_t));
  uint8_t wav_header[44];
  write_wav_header(wav_header, MIC_SAMPLE_RATE, MIC_SAMPLE_BITS, 1, data_bytes);

  const String boundary = "----IRONMANHUD";
  const String part_model = String("--") + boundary +
                            "\r\nContent-Disposition: form-data; name=\"model\"\r\n\r\n" +
                            String(OPENAI_STT_MODEL) + "\r\n";
  const String part_format = String("--") + boundary +
                             "\r\nContent-Disposition: form-data; name=\"response_format\"\r\n\r\ntext\r\n";
  const String part_file_header = String("--") + boundary +
                                  "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n" +
                                  "Content-Type: audio/wav\r\n\r\n";
  const String part_file_footer = String("\r\n--") + boundary + "--\r\n";

  const size_t content_length = part_model.length() + part_format.length() +
                                part_file_header.length() + sizeof(wav_header) +
                                data_bytes + part_file_footer.length();

  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("api.openai.com", 443)) {
    return false;
  }

  client.print("POST /v1/audio/transcriptions HTTP/1.1\r\n");
  client.print("Host: api.openai.com\r\n");
  client.print("Authorization: Bearer ");
  client.print(OPENAI_API_KEY);
  client.print("\r\n");
  client.print("Content-Type: multipart/form-data; boundary=");
  client.print(boundary);
  client.print("\r\n");
  client.print("Content-Length: ");
  client.print(content_length);
  client.print("\r\nConnection: close\r\n\r\n");

  client.print(part_model);
  client.print(part_format);
  client.print(part_file_header);
  client.write(wav_header, sizeof(wav_header));
  client.write(reinterpret_cast<const uint8_t *>(pcm), data_bytes);
  client.print(part_file_footer);

  String response;
  uint32_t start = millis();
  while (client.connected() && (millis() - start) < 10000) {
    while (client.available()) {
      response += client.readString();
      start = millis();
    }
    delay(10);
  }
  client.stop();

  int header_end = response.indexOf("\r\n\r\n");
  if (header_end < 0) {
    return false;
  }
  String body = response.substring(header_end + 4);
  body.trim();
  if (body.length() == 0) {
    return false;
  }
  out_text = body;
  return true;
}

bool local_transcribe_audio(const uint8_t *wav, size_t wav_bytes, String &out_text) {
  out_text = "";
  if (g_local_stt_url.length() == 0) {
    return false;
  }
  HTTPClient http;
  if (!http.begin(g_local_stt_url)) {
    return false;
  }
  http.addHeader("Content-Type", "audio/wav");
  int code = http.POST(const_cast<uint8_t *>(wav), wav_bytes);
  if (code != 200) {
    http.end();
    return false;
  }
  out_text = http.getString();
  http.end();
  out_text.trim();
  return out_text.length() > 0;
}

bool transcribe_audio(const int16_t *pcm, size_t sample_count, String &out_text) {
  const uint32_t data_bytes = static_cast<uint32_t>(sample_count * sizeof(int16_t));
  uint8_t wav_header[44];
  write_wav_header(wav_header, MIC_SAMPLE_RATE, MIC_SAMPLE_BITS, 1, data_bytes);

  if (g_local_stt_url.length() == 0) {
    discover_voice_server();
  }
  if (g_local_stt_url.length() > 0) {
    uint8_t *wav = static_cast<uint8_t *>(malloc(sizeof(wav_header) + data_bytes));
    if (!wav) {
      return false;
    }
    memcpy(wav, wav_header, sizeof(wav_header));
    memcpy(wav + sizeof(wav_header), pcm, data_bytes);
    bool ok = local_transcribe_audio(wav, sizeof(wav_header) + data_bytes, out_text);
    free(wav);
    return ok;
  }

  return openai_transcribe_audio(pcm, sample_count, out_text);
}

bool init_speaker(uint32_t sample_rate, uint16_t bits_per_sample) {
  g_speaker_i2s.end();
  g_speaker_ok = false;
  int ok = g_speaker_i2s.begin(I2S_PHILIPS_MODE, sample_rate, bits_per_sample);
  if (ok == 0) {
    return false;
  }
  g_speaker_i2s.setBufferSize(1024);
  g_speaker_rate = sample_rate;
  g_speaker_ok = true;
  return true;
}

bool parse_wav_header(const uint8_t *wav_header,
                      uint32_t &sample_rate,
                      uint16_t &bits_per_sample,
                      uint16_t &channels) {
  if (!wav_header) {
    return false;
  }
  if (memcmp(wav_header, "RIFF", 4) != 0 || memcmp(wav_header + 8, "WAVE", 4) != 0) {
    return false;
  }
  channels = wav_header[22] | (wav_header[23] << 8);
  sample_rate = wav_header[24] |
                (wav_header[25] << 8) |
                (wav_header[26] << 16) |
                (wav_header[27] << 24);
  bits_per_sample = wav_header[34] | (wav_header[35] << 8);
  return (channels == 1 || channels == 2) &&
         bits_per_sample == 16 &&
         sample_rate >= 8000 &&
         sample_rate <= 48000;
}

bool write_pcm_to_speaker(const uint8_t *data, size_t bytes, uint16_t bits_per_sample, uint16_t channels) {
  if (!data || bytes == 0 || bits_per_sample != 16 || (channels != 1 && channels != 2)) {
    return false;
  }

  const float volume = g_volume_scale;
  if (channels == 1) {
    int16_t stereo_buf[512];
    const int16_t *mono = reinterpret_cast<const int16_t *>(data);
    size_t mono_samples = bytes / sizeof(int16_t);
    size_t offset = 0;
    while (offset < mono_samples) {
      const size_t chunk = std::min<size_t>(mono_samples - offset, 256);
      for (size_t i = 0; i < chunk; ++i) {
        int32_t sample = mono[offset + i];
        if (volume < 0.999f) {
          sample = clampi(static_cast<int32_t>(sample * volume), -32768, 32767);
        }
        stereo_buf[i * 2] = static_cast<int16_t>(sample);
        stereo_buf[i * 2 + 1] = static_cast<int16_t>(sample);
      }
      g_speaker_i2s.write(reinterpret_cast<const uint8_t *>(stereo_buf), chunk * sizeof(int16_t) * 2);
      offset += chunk;
    }
    return true;
  }

  int16_t stereo_buf[512];
  const int16_t *pcm = reinterpret_cast<const int16_t *>(data);
  size_t samples = bytes / sizeof(int16_t);
  size_t offset = 0;
  while (offset < samples) {
    const size_t chunk = std::min<size_t>(samples - offset, 512);
    for (size_t i = 0; i < chunk; ++i) {
      int32_t sample = pcm[offset + i];
      if (volume < 0.999f) {
        sample = clampi(static_cast<int32_t>(sample * volume), -32768, 32767);
      }
      stereo_buf[i] = static_cast<int16_t>(sample);
    }
    g_speaker_i2s.write(reinterpret_cast<const uint8_t *>(stereo_buf), chunk * sizeof(int16_t));
    offset += chunk;
  }
  return true;
}

void write_speaker_silence() {
  int16_t silence[128] = {0};
  g_speaker_i2s.write(reinterpret_cast<const uint8_t *>(silence), sizeof(silence));
}

bool play_wav_buffer(const uint8_t *wav, size_t wav_bytes) {
  if (!wav || wav_bytes < 44) {
    return false;
  }
  uint32_t sample_rate = 0;
  uint16_t bits = 0;
  uint16_t channels = 0;
  if (!parse_wav_header(wav, sample_rate, bits, channels)) {
    return false;
  }
  if (!init_speaker(sample_rate, bits)) {
    return false;
  }
  const size_t frame_bytes = static_cast<size_t>(bits / 8) * channels;
  if (frame_bytes == 0) {
    return false;
  }
  const size_t pcm_bytes = ((wav_bytes - 44) / frame_bytes) * frame_bytes;
  const bool ok = write_pcm_to_speaker(wav + 44, pcm_bytes, bits, channels);
  write_speaker_silence();
  return ok;
}

#if HAS_SSCMA
bool init_ai() {
#if AI_ENABLE
  auto ret = g_ai_core.begin(SSCMAMicroCore::Config::DefaultConfig);
  if (!ret.success) {
    return false;
  }
  if (!g_ai_lock) {
    g_ai_lock = xSemaphoreCreateMutex();
  }
  g_ai_ok = true;
  return true;
#else
  return false;
#endif
}

void update_ai_results() {
  if (!g_ai_lock) return;
  if (xSemaphoreTake(g_ai_lock, pdMS_TO_TICKS(10)) != pdTRUE) return;
  g_ai_boxes.clear();
  for (const auto &box : g_ai_core.getBoxes()) {
    if (box.score >= AI_SCORE_THRESHOLD) {
      g_ai_boxes.push_back(box);
    }
  }
  g_ai_classes.clear();
  int class_count = 0;
  for (const auto &cls : g_ai_core.getClasses()) {
    if (cls.score >= AI_SCORE_THRESHOLD) {
      g_ai_classes.push_back(cls);
      if (++class_count >= AI_MAX_CLASSES) break;
    }
  }
  g_ai_points.clear();
  for (const auto &pt : g_ai_core.getPoints()) {
    if (pt.score >= AI_SCORE_THRESHOLD) {
      g_ai_points.push_back(pt);
    }
  }
  g_ai_keypoints.clear();
  for (const auto &kp : g_ai_core.getKeypoints()) {
    if (kp.box.score >= AI_SCORE_THRESHOLD) {
      g_ai_keypoints.push_back(kp);
    }
  }
  g_ai_perf = g_ai_core.getPerf();
  g_ai_last_update_ms = millis();
  xSemaphoreGive(g_ai_lock);
}
#endif

bool play_wav_stream(WiFiClient &client) {
  int first = -1;
  uint32_t wait_start = millis();
  while (client.connected() && first < 0 && (millis() - wait_start) < 2000) {
    if (client.available()) {
      first = client.peek();
      break;
    }
    delay(5);
  }
  if (first < 0) {
    return false;
  }
  if (first == 'H') {
    String line;
    while (client.connected()) {
      line = client.readStringUntil('\n');
      if (line == "\r" || line.length() == 0) {
        break;
      }
    }
  }

  uint8_t wav_header[44];
  size_t got = client.readBytes(wav_header, sizeof(wav_header));
  if (got < sizeof(wav_header)) {
    return false;
  }
  uint32_t sample_rate = 0;
  uint16_t bits = 0;
  uint16_t channels = 0;
  if (!parse_wav_header(wav_header, sample_rate, bits, channels)) {
    return false;
  }
  if (!init_speaker(sample_rate, bits)) {
    return false;
  }

  const size_t frame_bytes = static_cast<size_t>(bits / 8) * channels;
  uint8_t buffer[1028];
  size_t buffered = 0;
  while (client.connected() || client.available()) {
    int available = client.available();
    if (available > 0) {
      const int max_read = static_cast<int>(sizeof(buffer) - buffered);
      int read = client.read(buffer + buffered, std::min(available, max_read));
      if (read > 0) {
        buffered += static_cast<size_t>(read);
        const size_t consumable = frame_bytes > 0 ? (buffered / frame_bytes) * frame_bytes : 0;
        if (consumable > 0) {
          write_pcm_to_speaker(buffer, consumable, bits, channels);
          buffered -= consumable;
          if (buffered > 0) {
            memmove(buffer, buffer + consumable, buffered);
          }
        }
      }
    } else {
      delay(5);
    }
  }
  if (buffered >= frame_bytes) {
    const size_t consumable = (buffered / frame_bytes) * frame_bytes;
    if (consumable > 0) {
      write_pcm_to_speaker(buffer, consumable, bits, channels);
    }
  }
  write_speaker_silence();
  return true;
}

bool openai_tts_speak(const String &text) {
  if (!g_tts_ok || strlen(OPENAI_API_KEY) == 0) {
    return false;
  }
  if (!g_speaker_ok && !init_speaker(SPEAKER_DEFAULT_RATE, 16)) {
    return false;
  }
  const String body = String("{\"model\":\"") + OPENAI_TTS_MODEL +
                      "\",\"input\":\"" + text +
                      "\",\"voice\":\"" + OPENAI_TTS_VOICE +
                      "\",\"response_format\":\"wav\"}";

  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("api.openai.com", 443)) {
    return false;
  }
  client.print("POST /v1/audio/speech HTTP/1.1\r\n");
  client.print("Host: api.openai.com\r\n");
  client.print("Authorization: Bearer ");
  client.print(OPENAI_API_KEY);
  client.print("\r\n");
  client.print("Content-Type: application/json\r\n");
  client.print("Content-Length: ");
  client.print(body.length());
  client.print("\r\nConnection: close\r\n\r\n");
  client.print(body);

  bool ok = play_wav_stream(client);
  client.stop();
  return ok;
}

bool local_tts_speak(const String &text) {
  if (g_local_tts_url.length() == 0) {
    return false;
  }
  if (!g_speaker_ok && !init_speaker(SPEAKER_DEFAULT_RATE, 16)) {
    return false;
  }
  HTTPClient http;
  if (!http.begin(g_local_tts_url)) {
    return false;
  }
  http.addHeader("Content-Type", "text/plain");
  int code = http.POST(text);
  if (code != 200) {
    http.end();
    return false;
  }
  WiFiClient *stream = http.getStreamPtr();
  const int total = http.getSize();
  bool ok = false;
  if (stream && total > 44 && total < 512 * 1024) {
    std::vector<uint8_t> wav(static_cast<size_t>(total));
    size_t offset = 0;
    uint32_t last_read_ms = millis();
    while (offset < wav.size() && (http.connected() || stream->available())) {
      int read = stream->read(wav.data() + offset, wav.size() - offset);
      if (read > 0) {
        offset += static_cast<size_t>(read);
        last_read_ms = millis();
      } else if (millis() - last_read_ms > 2000) {
        break;
      } else {
        delay(2);
      }
    }
    if (offset > 0) {
      ok = play_wav_buffer(wav.data(), offset);
    }
  }
  if (!ok && stream) {
    ok = play_wav_stream(*stream);
  }
  http.end();
  return ok;
}

bool speak_text(const String &text) {
  const String trimmed = text.substring(0, 200);
  if (g_local_tts_url.length() == 0) {
    discover_voice_server();
  }
  if (g_local_tts_url.length() > 0) {
    return local_tts_speak(trimmed);
  }
  return openai_tts_speak(trimmed);
}

bool enqueue_speech_text(const String &text) {
  if (!g_speech_lock || text.length() == 0) {
    return false;
  }
  const String trimmed = text.substring(0, 200);
  if (xSemaphoreTake(g_speech_lock, pdMS_TO_TICKS(20)) != pdTRUE) {
    return false;
  }
  bool ok = false;
  if (g_speech_queue_count < SPEECH_QUEUE_CAP) {
    g_speech_queue[g_speech_queue_tail] = trimmed;
    g_speech_queue_tail = (g_speech_queue_tail + 1) % SPEECH_QUEUE_CAP;
    g_speech_queue_count++;
    ok = true;
  }
  xSemaphoreGive(g_speech_lock);
  return ok;
}

bool dequeue_speech_text(String &text) {
  text = "";
  if (!g_speech_lock) {
    return false;
  }
  if (xSemaphoreTake(g_speech_lock, pdMS_TO_TICKS(20)) != pdTRUE) {
    return false;
  }
  bool ok = false;
  if (g_speech_queue_count > 0) {
    text = g_speech_queue[g_speech_queue_head];
    g_speech_queue[g_speech_queue_head] = "";
    g_speech_queue_head = (g_speech_queue_head + 1) % SPEECH_QUEUE_CAP;
    g_speech_queue_count--;
    ok = true;
  }
  xSemaphoreGive(g_speech_lock);
  return ok;
}

void update_speech_output() {
  if (g_voice_busy || !g_tts_ok) {
    return;
  }
  String next;
  if (!dequeue_speech_text(next)) {
    return;
  }
  g_voice_busy = true;
  const bool ok = speak_text(next);
  g_voice_busy = false;
  if (!ok) {
    log_event("TTS_FAIL");
  }
}

bool capture_and_transcribe(String &transcript, float &level_out, uint32_t &samples_out) {
  transcript = "";
  level_out = 0.0f;
  samples_out = 0;
  if (!g_mic_ok || !g_stt_ok) {
    return false;
  }
  if (!g_mic_lock || xSemaphoreTake(g_mic_lock, pdMS_TO_TICKS(200)) != pdTRUE) {
    return false;
  }

  const size_t sample_count = static_cast<size_t>(MIC_SAMPLE_RATE) * MIC_CAPTURE_MS / 1000;
  const size_t bytes_needed = sample_count * sizeof(int16_t);
  int16_t *buffer = static_cast<int16_t *>(malloc(bytes_needed));
  if (!buffer) {
    xSemaphoreGive(g_mic_lock);
    return false;
  }

  size_t offset = 0;
  const uint32_t start = millis();
  while (offset < bytes_needed && (millis() - start) < (MIC_CAPTURE_MS + 400)) {
    const int read = I2S.read(reinterpret_cast<uint8_t *>(buffer) + offset, bytes_needed - offset);
    if (read > 0) {
      offset += static_cast<size_t>(read);
    }
  }
  xSemaphoreGive(g_mic_lock);

  uint64_t sum = 0;
  size_t count = offset / sizeof(int16_t);
  const float gain = MIC_INPUT_GAIN;
  for (size_t i = 0; i < count; ++i) {
    int32_t v = buffer[i];
    if (gain > 1.01f || gain < 0.99f) {
      v = static_cast<int32_t>(v * gain);
      v = clampi(v, -32768, 32767);
      buffer[i] = static_cast<int16_t>(v);
    }
    sum += static_cast<uint64_t>(v * v);
  }
  const float rms = count > 0 ? sqrtf(static_cast<float>(sum) / static_cast<float>(count)) / 32768.0f : 0.0f;
  level_out = clampf(rms * 4.0f, 0.0f, 1.0f);
  samples_out = static_cast<uint32_t>(count);

  bool ok = transcribe_audio(buffer, count, transcript);
  free(buffer);
  return ok;
}
