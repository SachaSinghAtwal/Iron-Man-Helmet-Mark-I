// Shared runtime state for the HUD modules. Included by src/main.cpp inside the anonymous namespace.
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 30000;
static const uint32_t WIFI_RECONNECT_INTERVAL_MS = 15000;
static const uint8_t WIFI_CONNECT_ATTEMPTS = 3;
static const uint32_t STATUS_UPDATE_MS = 1000;

static httpd_handle_t g_httpd = nullptr;
static httpd_handle_t g_stream_httpd = nullptr;
static SemaphoreHandle_t g_camera_lock = nullptr;
static SemaphoreHandle_t g_clip_lock = nullptr;
static volatile bool g_camera_recovering = false;

static volatile uint32_t g_frame_count = 0;
static volatile uint32_t g_fps = 0;
static volatile uint32_t g_last_fps_ts = 0;

static const size_t LOG_CAP = 12;
static String g_logs[LOG_CAP];
static uint8_t g_log_head = 0;
static bool g_mask_ok = false;
static bool g_mask_open = false;
static bool g_mask_moving = false;
static bool g_mask_target_open = false;
static float g_mask_top_current = MASK_TOP_CLOSED_DEG;
static float g_mask_bottom_current = MASK_BOTTOM_CLOSED_DEG;
static float g_mask_top_start = MASK_TOP_CLOSED_DEG;
static float g_mask_bottom_start = MASK_BOTTOM_CLOSED_DEG;
static float g_mask_top_target = MASK_TOP_CLOSED_DEG;
static float g_mask_bottom_target = MASK_BOTTOM_CLOSED_DEG;
static uint32_t g_mask_move_start_ms = 0;
static volatile float g_mic_level = 0.0f;
static volatile bool g_mic_ok = false;
static uint32_t g_last_mic_ms = 0;
static SemaphoreHandle_t g_mic_lock = nullptr;
static bool g_stt_ok = false;
static bool g_tts_ok = false;
static bool g_speaker_ok = false;
static uint32_t g_speaker_rate = 0;
static uint8_t g_volume_percent = 100;
static float g_volume_scale = 1.0f;
static String g_local_stt_url;
static String g_local_tts_url;
static String g_voice_host;
static uint16_t g_voice_port = LOCAL_VOICE_PORT;
static uint32_t g_last_voice_discovery_ms = 0;
static uint8_t g_voice_scan_next = 2;
static bool g_voice_busy = false;
static uint32_t g_last_wake_ms = 0;
static uint32_t g_voice_seq = 0;
static String g_voice_text;
static SemaphoreHandle_t g_voice_lock = nullptr;
static SemaphoreHandle_t g_speech_lock = nullptr;
static const size_t SPEECH_QUEUE_CAP = 24;
static String g_speech_queue[SPEECH_QUEUE_CAP];
static size_t g_speech_queue_head = 0;
static size_t g_speech_queue_tail = 0;
static size_t g_speech_queue_count = 0;
static uint32_t g_last_wifi_retry_ms = 0;
static bool g_ntp_started = false;
static uint32_t g_last_ntp_attempt_ms = 0;
// GNSS transport: use the XIAO ESP32S3 hardware UART on the GNSS daughter
// board's direct D7/D6 serial path.
static HardwareSerial GNSS(GNSS_HW_UART_PORT);
static TinyGPSPlus g_gps;
static double g_gnss_lat = NAN;
static double g_gnss_lon = NAN;
static float g_gnss_speed_ms = -1.0f;
static double g_gnss_prev_lat = NAN;
static double g_gnss_prev_lon = NAN;
static uint32_t g_gnss_prev_fix_sample_ms = 0;
static uint32_t g_gnss_last_fix_ms = 0;
static uint32_t g_gnss_last_time_ms = 0;
static uint32_t g_gnss_satellites = 0;
static uint32_t g_gnss_fix_age_ms = 0xFFFFFFFFUL;
static float g_gnss_hdop = NAN;
static int g_gnss_quality = 0;
static uint32_t g_gnss_passed_checksum = 0;
static uint32_t g_gnss_failed_checksum = 0;
static uint32_t g_gnss_last_valid_sentence_ms = 0;
static uint32_t g_gnss_baud_probe_start_ms = 0;
static uint8_t g_gnss_baud_index = 0;
static unsigned long g_gnss_active_baud = GNSS_BAUD;
static bool g_gnss_serial_seen = false;
static bool g_gnss_baud_locked = false;
static bool g_gnss_has_fix = false;
static bool g_gnss_data_seen = false;
static bool g_gnss_no_data_warned = false;
static bool g_mask_action_prev_active = false;
static uint32_t g_mask_action_last_ms = 0;
static volatile bool g_streaming = false;
static volatile bool g_camera_ok = false;
static uint8_t g_camera_fail_count = 0;
static const uint8_t CLIP_RING_CAP = 18;
static const uint32_t CLIP_STORE_INTERVAL_MS = 300;
static const uint32_t CLIP_WINDOW_MS = 5200;
static const size_t CLIP_MAX_FRAME_BYTES = 130000;
struct ClipFrame {
  uint8_t *data = nullptr;
  size_t len = 0;
  uint32_t t = 0;
};
static ClipFrame g_clip_frames[CLIP_RING_CAP];
static uint8_t g_clip_next = 0;
static uint32_t g_clip_last_store_ms = 0;
static I2SClass g_speaker_i2s(1, 0, SPEAKER_DATA_PIN, SPEAKER_BCLK_PIN, SPEAKER_LRCLK_PIN);
#if HAS_SSCMA
static SSCMAMicroCore g_ai_core;
static bool g_ai_ok = false;
static uint32_t g_ai_last_infer_ms = 0;
static uint32_t g_ai_last_update_ms = 0;
static uint16_t g_ai_frame_w = 0;
static uint16_t g_ai_frame_h = 0;
static SemaphoreHandle_t g_ai_lock = nullptr;
static std::vector<SSCMAMicroCore::Box> g_ai_boxes;
static std::vector<SSCMAMicroCore::Class> g_ai_classes;
static std::vector<SSCMAMicroCore::Point> g_ai_points;
static std::vector<SSCMAMicroCore::Keypoints> g_ai_keypoints;
static SSCMAMicroCore::Perf g_ai_perf;
#endif
static const unsigned long GNSS_BAUD_CANDIDATES[] = {
  static_cast<unsigned long>(GNSS_BAUD),
  9600UL,
  38400UL,
  57600UL,
  115200UL,
  19200UL,
  4800UL
};
static const size_t GNSS_BAUD_CANDIDATE_COUNT =
    sizeof(GNSS_BAUD_CANDIDATES) / sizeof(GNSS_BAUD_CANDIDATES[0]);

