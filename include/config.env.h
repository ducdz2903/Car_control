#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ================== WIFI ==================
const char* WIFI_SSID = "WIFI_SSID";
const char* WIFI_PASS = "WIFI_PASS";

// ================== WS SERVER ==================
const char* WS_HOST = "WSS_URL";
const uint16_t WS_PORT = 443;
const char* WS_PATH = "/api/ws/robot/{ROBOT_ID}";

// ================== MOTOR PINS (ESP32 + L298N) ==================
// PWM pins (ENA/ENB)
const int ENA = 5;
const int ENB = 23;

// Motor control pins
const int IN1 = 22;
const int IN2 = 21;
const int IN3 = 19;
const int IN4 = 18;
// Servo control pin
const int SERVO_PIN = 17;
// PWM (LEDC)
const int PWM_CH_A = 4;
const int PWM_CH_B = 5;
const int PWM_FREQ = 1000;   // Hz
const int PWM_BITS = 8;      // 0..255

// ================== CONTROL STATE ==================
const int SPEED_NORMAL = 120;
const int SPEED_TURN = 180;
const int DUMMY_SPEED_PWM = 120;

// *** Hiệu chỉnh theo xe của bạn (không có encoder) ***
const float MS_PER_METER = 1000.0f;   // Giả định: chạy 1 m mất 1000 ms ở speedNormal
const float MS_PER_DEGREE = 8.0f;     // Giả định: quay 1° mất 8 ms ở speedTurn

#endif // CONFIG_H
