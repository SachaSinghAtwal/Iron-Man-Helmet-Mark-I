#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "time.h"
#include <sys/time.h>
#include <I2S.h>
#include <math.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <cstring>
#include <algorithm>
#include <vector>
#include "mbedtls/base64.h"
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

#if __has_include(<SSCMA_Micro_Core.h>)
#include <SSCMA_Micro_Core.h>
#define HAS_SSCMA 1
#else
#define HAS_SSCMA 0
#endif

#include "camera_pins.h"
#include <FS.h>
#include <LittleFS.h>

#if __has_include("secrets.h")
#include "secrets.h"
#endif

#include "hud_config_defaults.h"

namespace {
#include "modules/app_state.hpp"

#include "modules/core_utils.hpp"
#include "modules/mask_pwm.hpp"
#include "modules/hud_status.hpp"
#include "modules/voice_discovery.hpp"
#include "modules/mic_gnss.hpp"
#include "modules/speech_speaker_ai.hpp"
#include "modules/camera_wifi.hpp"
#include "modules/web_page.hpp"
#include "modules/http_handlers.hpp"

void setup() {
  Serial.begin(115200);
  delay(200);
  log_event("BOOT_OK");
  if (!g_camera_lock) {
    g_camera_lock = xSemaphoreCreateMutex();
  }
  if (!g_clip_lock) {
    g_clip_lock = xSemaphoreCreateMutex();
  }

  setenv("TZ", TIMEZONE, 1);
  tzset();

  if (!init_camera()) {
    log_event("CAM_INIT_FAIL");
  } else {
    log_event("CAM_INIT_OK");
  }

#if HAS_SSCMA
  if (AI_ENABLE && init_ai()) {
    log_event("AI_OK");
  } else if (AI_ENABLE) {
    log_event("AI_FAIL");
  }
#endif

  if (!LittleFS.begin(true)) {
    log_event("FS_FAIL");
  } else {
    log_event("FS_OK");
  }

  set_volume_percent(g_volume_percent);
  g_speech_lock = xSemaphoreCreateMutex();
  g_local_stt_url = String(LOCAL_STT_URL);
  g_local_tts_url = String(LOCAL_TTS_URL);
  if (g_local_stt_url.length() > 0) {
    set_voice_origin_from_url(g_local_stt_url);
  } else if (g_local_tts_url.length() > 0) {
    set_voice_origin_from_url(g_local_tts_url);
  }
  update_voice_config();
  init_mask_action_input();
  if (!init_mask_servos()) {
    log_event("MASK_PWM_FAIL");
  } else {
    log_event("MASK_PWM_OK");
  }
  if (!init_mic()) {
    log_event("MIC_FAIL");
  } else {
    log_event("MIC_OK");
  }

  if (!init_gnss()) {
    log_event("GNSS_FAIL");
  } else {
    log_event("GNSS_OK");
  }

  if (!init_speaker(SPEAKER_DEFAULT_RATE, 16)) {
    log_event("SPK_FAIL");
  } else {
    log_event("SPK_OK");
  }

  const bool wifi_ok = connect_wifi();
  if (wifi_ok) {
    sync_time_if_wifi();
  }

  if (!start_webserver()) {
    log_event("HTTP_FAIL");
  } else {
    log_event("HTTP_OK");
  }
}

void loop() {
  static uint32_t last_status = 0;
  maintain_wifi();
  update_speech_output();
  update_mask_action_input();
  update_mask();
  update_mic_level();
  update_gnss();
#if HAS_SSCMA
  if (AI_ENABLE && g_ai_ok) {
    const uint32_t now = millis();
    const uint32_t interval = g_streaming ? std::max<uint32_t>(AI_INFER_INTERVAL_MS, 1000) : AI_INFER_INTERVAL_MS;
    if (now - g_ai_last_infer_ms >= interval) {
      g_ai_last_infer_ms = now;
      bool lock_taken = false;
      if (g_camera_lock) {
        if (xSemaphoreTake(g_camera_lock, pdMS_TO_TICKS(10)) == pdTRUE) {
          lock_taken = true;
        }
      }
      if (lock_taken || !g_camera_lock) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
          auto frame = SSCMAMicroCore::Frame::fromCameraFrame(fb);
          g_ai_frame_w = frame.width;
          g_ai_frame_h = frame.height;
          auto ret = g_ai_core.invoke(frame);
          if (ret.success) {
            update_ai_results();
          }
          esp_camera_fb_return(fb);
        } else {
          g_camera_fail_count++;
          if (g_camera_fail_count >= 3) {
            g_camera_fail_count = 0;
            recover_camera();
          }
        }
        if (lock_taken) {
          xSemaphoreGive(g_camera_lock);
        }
      }
    }
  }
#endif
  if (VOICE_ALWAYS_LISTEN && g_mic_ok && !g_voice_busy) {
    if (g_local_stt_url.length() == 0 && g_local_tts_url.length() == 0) {
      discover_voice_server();
    }
  }
  if (VOICE_ALWAYS_LISTEN && g_mic_ok && g_stt_ok && !g_voice_busy) {
    const uint32_t now = millis();
    if (g_mic_level > MIC_WAKE_THRESHOLD && (now - g_last_wake_ms) > MIC_WAKE_COOLDOWN_MS) {
      g_voice_busy = true;
      String transcript;
      float level = 0.0f;
      uint32_t samples = 0;
      const bool ok = capture_and_transcribe(transcript, level, samples);
      g_last_wake_ms = now;
      g_voice_busy = false;
      if (ok) {
        transcript.trim();
        if (transcript.length() > 0) {
          set_voice_text(transcript);
        }
      }
    }
  }
  if (millis() - last_status > STATUS_UPDATE_MS) {
    last_status = millis();
    // Placeholder for future sensor polling.
  }
  delay(10);
}





