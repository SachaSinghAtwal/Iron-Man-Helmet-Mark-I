// Fallback build-time configuration. Values in include/secrets.h override these defaults.
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif
#ifndef WIFI_PASS
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#endif
#ifndef AP_SSID
#define AP_SSID "IRONMAN_HUD"
#endif
#ifndef AP_PASS
#define AP_PASS "repulsor42"
#endif
#ifndef TIMEZONE
#define TIMEZONE "UTC0"
#endif
#ifndef HUD_LATITUDE
#define HUD_LATITUDE ""
#endif
#ifndef HUD_LONGITUDE
#define HUD_LONGITUDE ""
#endif
#ifndef HEART_RATE_SENSOR_PIN
#define HEART_RATE_SENSOR_PIN -1
#endif
#ifndef BATTERY_SENSE_PIN
#define BATTERY_SENSE_PIN -1
#endif
#ifndef BATTERY_SENSE_MIN_V
#define BATTERY_SENSE_MIN_V 3.3f
#endif
#ifndef BATTERY_SENSE_MAX_V
#define BATTERY_SENSE_MAX_V 4.2f
#endif
#ifndef BATTERY_SENSE_DIVIDER
#define BATTERY_SENSE_DIVIDER 2.0f
#endif
#ifndef MIC_DATA_PIN
#define MIC_DATA_PIN 41
#endif
#ifndef MIC_CLK_PIN
#define MIC_CLK_PIN 42
#endif
#ifndef MIC_SAMPLE_RATE
#define MIC_SAMPLE_RATE 16000
#endif
#ifndef MIC_SAMPLE_BITS
#define MIC_SAMPLE_BITS 16
#endif
#ifndef MIC_INPUT_GAIN
#define MIC_INPUT_GAIN 2.0f
#endif
#ifndef MIC_LEVEL_INTERVAL_MS
#define MIC_LEVEL_INTERVAL_MS 120
#endif
#ifndef MIC_CAPTURE_MS
#define MIC_CAPTURE_MS 1500
#endif
#ifndef OPENAI_API_KEY
#define OPENAI_API_KEY ""
#endif
#ifndef OPENAI_STT_MODEL
#define OPENAI_STT_MODEL "gpt-4o-mini-transcribe"
#endif
#ifndef OPENAI_TTS_MODEL
#define OPENAI_TTS_MODEL "gpt-4o-mini-tts"
#endif
#ifndef OPENAI_TTS_VOICE
#define OPENAI_TTS_VOICE "onyx"
#endif
#ifndef LOCAL_STT_URL
#define LOCAL_STT_URL ""
#endif
#ifndef LOCAL_TTS_URL
#define LOCAL_TTS_URL ""
#endif
#ifndef LOCAL_VOICE_PORT
#define LOCAL_VOICE_PORT 8000
#endif
#ifndef VOICE_DISCOVERY_INTERVAL_MS
#define VOICE_DISCOVERY_INTERVAL_MS 5000
#endif
#ifndef VOICE_DISCOVERY_BATCH
#define VOICE_DISCOVERY_BATCH 12
#endif
#ifndef VOICE_DISCOVERY_AP_MAX
#define VOICE_DISCOVERY_AP_MAX 12
#endif
#ifndef VOICE_ALWAYS_LISTEN
#define VOICE_ALWAYS_LISTEN 1
#endif
#ifndef MIC_WAKE_THRESHOLD
#define MIC_WAKE_THRESHOLD 0.18f
#endif
#ifndef MIC_WAKE_COOLDOWN_MS
#define MIC_WAKE_COOLDOWN_MS 6000
#endif
#ifndef GNSS_RX_PIN
#define GNSS_RX_PIN D7
#endif
#ifndef GNSS_TX_PIN
#define GNSS_TX_PIN D6
#endif
#ifndef GNSS_BAUD
#define GNSS_BAUD 9600
#endif
#ifndef GNSS_UART_PORT
#define GNSS_UART_PORT 1
#endif
#ifndef GNSS_HW_UART_PORT
#define GNSS_HW_UART_PORT 1
#endif
#ifndef GNSS_USE_XIAO_DEFAULT_UART_PINS
#define GNSS_USE_XIAO_DEFAULT_UART_PINS 0
#endif
#ifndef GNSS_FIX_TIMEOUT_MS
#define GNSS_FIX_TIMEOUT_MS 5000
#endif
#ifndef GNSS_TIME_SYNC_INTERVAL_MS
#define GNSS_TIME_SYNC_INTERVAL_MS 30000
#endif
#ifndef GNSS_TIME_SYNC_ENABLE
#define GNSS_TIME_SYNC_ENABLE 0
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
#ifndef NTP_SYNC_TIMEOUT_MS
#define NTP_SYNC_TIMEOUT_MS 15000
#endif
#ifndef NTP_RETRY_INTERVAL_MS
#define NTP_RETRY_INTERVAL_MS 30000
#endif
#ifndef SPEAKER_BCLK_PIN
#define SPEAKER_BCLK_PIN D8
#endif
#ifndef SPEAKER_LRCLK_PIN
#define SPEAKER_LRCLK_PIN D9
#endif
#ifndef SPEAKER_DATA_PIN
#define SPEAKER_DATA_PIN D10
#endif
#ifndef SPEAKER_DEFAULT_RATE
#define SPEAKER_DEFAULT_RATE 24000
#endif
#ifndef MASK_SERVO_ENABLE
#define MASK_SERVO_ENABLE 1
#endif
#ifndef MASK_PWM_I2C_ADDR
#define MASK_PWM_I2C_ADDR 0x40
#endif
#ifndef MASK_PWM_SDA_PIN
#define MASK_PWM_SDA_PIN D4
#endif
#ifndef MASK_PWM_SCL_PIN
#define MASK_PWM_SCL_PIN D5
#endif
#ifndef MASK_PWM_OE_PIN
#define MASK_PWM_OE_PIN -1
#endif
#ifndef MASK_TOP_CHANNEL
#define MASK_TOP_CHANNEL 0
#endif
#ifndef MASK_BOTTOM_CHANNEL
#define MASK_BOTTOM_CHANNEL 1
#endif
#ifndef MASK_TOP_OPEN_DEG
#define MASK_TOP_OPEN_DEG 20
#endif
#ifndef MASK_BOTTOM_OPEN_DEG
#define MASK_BOTTOM_OPEN_DEG 20
#endif
#ifndef MASK_TOP_CLOSED_DEG
#define MASK_TOP_CLOSED_DEG 167
#endif
#ifndef MASK_BOTTOM_CLOSED_DEG
#define MASK_BOTTOM_CLOSED_DEG 107
#endif
#ifndef MASK_SERVO_MIN_US
#define MASK_SERVO_MIN_US 500
#endif
#ifndef MASK_SERVO_MAX_US
#define MASK_SERVO_MAX_US 2500
#endif
#ifndef MASK_SERVO_FREQ
#define MASK_SERVO_FREQ 50
#endif
#ifndef MASK_MOVE_MS
#define MASK_MOVE_MS 600
#endif
#ifndef MASK_ACTION_PIN
#define MASK_ACTION_PIN -1
#endif
#ifndef MASK_ACTION_ACTIVE_LOW
#define MASK_ACTION_ACTIVE_LOW 1
#endif
#ifndef MASK_ACTION_DEBOUNCE_MS
#define MASK_ACTION_DEBOUNCE_MS 600
#endif
#ifndef AI_ENABLE
#define AI_ENABLE 1
#endif
#ifndef AI_INFER_INTERVAL_MS
#define AI_INFER_INTERVAL_MS 450
#endif
#ifndef AI_SCORE_THRESHOLD
#define AI_SCORE_THRESHOLD 0.45f
#endif
#ifndef AI_MAX_CLASSES
#define AI_MAX_CLASSES 4
#endif
