// Implementation fragment included by src/main.cpp inside the HUD anonymous namespace.
void set_volume_percent(int percent) {
  const int clamped = clampi(percent, 0, 100);
  g_volume_percent = static_cast<uint8_t>(clamped);
  g_volume_scale = static_cast<float>(clamped) / 100.0f;
}

static const uint8_t PCA9685_MODE1 = 0x00;
static const uint8_t PCA9685_PRESCALE = 0xFE;
static const uint8_t PCA9685_LED0_ON_L = 0x06;

bool pca9685_write8(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MASK_PWM_I2C_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

uint8_t pca9685_read8(uint8_t reg, bool &ok) {
  Wire.beginTransmission(MASK_PWM_I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    ok = false;
    return 0;
  }
  const uint8_t read = Wire.requestFrom(static_cast<uint8_t>(MASK_PWM_I2C_ADDR), static_cast<uint8_t>(1));
  if (read != 1 || !Wire.available()) {
    ok = false;
    return 0;
  }
  ok = true;
  return Wire.read();
}

bool pca9685_set_pwm_freq(float freq_hz) {
  bool ok = false;
  const uint8_t oldmode = pca9685_read8(PCA9685_MODE1, ok);
  if (!ok) return false;
  const uint8_t sleep_mode = (oldmode & 0x7F) | 0x10;
  const float prescale_value = 25000000.0f / (4096.0f * freq_hz) - 1.0f;
  const uint8_t prescale = static_cast<uint8_t>(roundf(prescale_value));
  if (!pca9685_write8(PCA9685_MODE1, sleep_mode)) return false;
  if (!pca9685_write8(PCA9685_PRESCALE, prescale)) return false;
  if (!pca9685_write8(PCA9685_MODE1, oldmode)) return false;
  delay(5);
  return pca9685_write8(PCA9685_MODE1, oldmode | 0xA1);  // restart + auto-increment
}

bool pca9685_set_pwm(uint8_t channel, uint16_t on, uint16_t off) {
  if (channel > 15) return false;
  const uint8_t reg = PCA9685_LED0_ON_L + 4 * channel;
  Wire.beginTransmission(MASK_PWM_I2C_ADDR);
  Wire.write(reg);
  Wire.write(on & 0xFF);
  Wire.write((on >> 8) & 0x0F);
  Wire.write(off & 0xFF);
  Wire.write((off >> 8) & 0x0F);
  return Wire.endTransmission() == 0;
}

uint16_t mask_angle_to_ticks(float angle_deg) {
  const float angle = clampf(angle_deg, 0.0f, 180.0f);
  const float pulse_us = MASK_SERVO_MIN_US + (MASK_SERVO_MAX_US - MASK_SERVO_MIN_US) * (angle / 180.0f);
  const float period_us = 1000000.0f / static_cast<float>(MASK_SERVO_FREQ);
  return static_cast<uint16_t>(clampf(roundf((pulse_us * 4096.0f) / period_us), 0.0f, 4095.0f));
}

bool mask_write_servo(uint8_t channel, float angle_deg) {
  return pca9685_set_pwm(channel, 0, mask_angle_to_ticks(angle_deg));
}

float ease_cubic_in_out(float t) {
  const float clamped = clampf(t, 0.0f, 1.0f);
  if (clamped < 0.5f) {
    return 4.0f * clamped * clamped * clamped;
  }
  const float f = -2.0f * clamped + 2.0f;
  return 1.0f - (f * f * f) / 2.0f;
}

bool init_mask_servos() {
#if MASK_SERVO_ENABLE
  if (MASK_PWM_OE_PIN >= 0) {
    pinMode(MASK_PWM_OE_PIN, OUTPUT);
    digitalWrite(MASK_PWM_OE_PIN, LOW);  // OE is active-low on PCA9685 boards.
  }
  if (MASK_PWM_SDA_PIN >= 0 && MASK_PWM_SCL_PIN >= 0) {
    Wire.begin(MASK_PWM_SDA_PIN, MASK_PWM_SCL_PIN);
  } else {
    Wire.begin();
  }
  Wire.setClock(400000);

  bool ok = false;
  (void)pca9685_read8(PCA9685_MODE1, ok);
  if (!ok || !pca9685_set_pwm_freq(static_cast<float>(MASK_SERVO_FREQ))) {
    g_mask_ok = false;
    return false;
  }

  g_mask_ok = true;
  g_mask_open = false;
  g_mask_moving = false;
  g_mask_top_current = MASK_TOP_CLOSED_DEG;
  g_mask_bottom_current = MASK_BOTTOM_CLOSED_DEG;
  const bool top_ok = mask_write_servo(MASK_TOP_CHANNEL, g_mask_top_current);
  const bool bottom_ok = mask_write_servo(MASK_BOTTOM_CHANNEL, g_mask_bottom_current);
  if (!top_ok || !bottom_ok) {
    g_mask_ok = false;
    return false;
  }
  return true;
#else
  return false;
#endif
}

bool mask_start_move(bool open) {
  if (!g_mask_ok) {
    if (!init_mask_servos()) {
      return false;
    }
  }

  g_mask_target_open = open;
  g_mask_top_start = g_mask_top_current;
  g_mask_bottom_start = g_mask_bottom_current;
  g_mask_top_target = open ? MASK_TOP_OPEN_DEG : MASK_TOP_CLOSED_DEG;
  g_mask_bottom_target = open ? MASK_BOTTOM_OPEN_DEG : MASK_BOTTOM_CLOSED_DEG;
  g_mask_move_start_ms = millis();
  g_mask_moving = true;
  log_event(open ? "MASK_OPENING" : "MASK_CLOSING");
  return true;
}

void update_mask() {
  if (!g_mask_ok || !g_mask_moving) {
    return;
  }

  const uint32_t elapsed = millis() - g_mask_move_start_ms;
  const float raw = MASK_MOVE_MS > 0 ? static_cast<float>(elapsed) / static_cast<float>(MASK_MOVE_MS) : 1.0f;
  const float eased = ease_cubic_in_out(raw);
  g_mask_top_current = g_mask_top_start + (g_mask_top_target - g_mask_top_start) * eased;
  g_mask_bottom_current = g_mask_bottom_start + (g_mask_bottom_target - g_mask_bottom_start) * eased;

  const bool top_ok = mask_write_servo(MASK_TOP_CHANNEL, g_mask_top_current);
  const bool bottom_ok = mask_write_servo(MASK_BOTTOM_CHANNEL, g_mask_bottom_current);
  if (!top_ok || !bottom_ok) {
    g_mask_ok = false;
    g_mask_moving = false;
    log_event("MASK_PWM_FAIL");
    return;
  }

  if (raw >= 1.0f) {
    g_mask_top_current = g_mask_top_target;
    g_mask_bottom_current = g_mask_bottom_target;
    mask_write_servo(MASK_TOP_CHANNEL, g_mask_top_current);
    mask_write_servo(MASK_BOTTOM_CHANNEL, g_mask_bottom_current);
    g_mask_open = g_mask_target_open;
    g_mask_moving = false;
    log_event(g_mask_open ? "MASK_OPEN" : "MASK_CLOSED");
  }
}

void init_mask_action_input() {
#if MASK_ACTION_PIN >= 0
  if (MASK_ACTION_ACTIVE_LOW) {
    pinMode(MASK_ACTION_PIN, INPUT_PULLUP);
  } else {
    pinMode(MASK_ACTION_PIN, INPUT);
  }
  g_mask_action_prev_active = false;
  g_mask_action_last_ms = 0;
#endif
}

void update_mask_action_input() {
#if MASK_ACTION_PIN >= 0
  const bool active = MASK_ACTION_ACTIVE_LOW ? (digitalRead(MASK_ACTION_PIN) == LOW)
                                             : (digitalRead(MASK_ACTION_PIN) == HIGH);
  const uint32_t now = millis();
  if (active &&
      !g_mask_action_prev_active &&
      !g_mask_moving &&
      (now - g_mask_action_last_ms) >= MASK_ACTION_DEBOUNCE_MS) {
    if (mask_start_move(!g_mask_open)) {
      g_mask_action_last_ms = now;
    }
  }
  g_mask_action_prev_active = active;
#endif
}
