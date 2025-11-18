#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "config.h"

WebSocketsClient ws;

// ================== CONTROL STATE ==================
// speedNormal: dùng cho đi thẳng / lùi
// speedTurn  : dùng cho rẽ trái / phải
int speedNormal = SPEED_NORMAL;
int speedTurn   = SPEED_TURN;

String lastCmd = "idle";
unsigned long actionUntil = 0;          // thời điểm dừng auto (ms). 0 = không hẹn

// Lưu intent + action_id của action hiện tại (để gửi result)
String currentIntent = "";
String currentActionId = "";

// ================== HELPERS ==================
void pwmWriteBoth(int val) {
  val = constrain(val, 0, 255);
  ledcWrite(PWM_CH_A, val);
  ledcWrite(PWM_CH_B, val);
}

void idle() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  pwmWriteBoth(0);
  lastCmd = "idle";
  Serial.println("[MOTOR] IDLE");
}

void forward() {
  pwmWriteBoth(speedNormal);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  lastCmd = "forward";
  Serial.printf("[MOTOR] FORWARD @PWM=%d\n", speedNormal);
}

void backward() {
  pwmWriteBoth(speedNormal);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  lastCmd = "backward";
  Serial.printf("[MOTOR] BACKWARD @PWM=%d\n", speedNormal);
}

void turnLeft() {
  pwmWriteBoth(speedTurn);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  lastCmd = "left";
  Serial.printf("[MOTOR] LEFT @PWM=%d\n", speedTurn);
}

void turnRight() {
  pwmWriteBoth(speedTurn);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  lastCmd = "right";
  Serial.printf("[MOTOR] RIGHT @PWM=%d\n", speedTurn);
}

void stopMotor() {
  pwmWriteBoth(0);
  lastCmd = "stop";
  Serial.println("[MOTOR] STOP");
}

// Gửi kết quả lên server sau khi HOÀN THÀNH lệnh
void sendActionResult(const String& actionId, bool success, const char* message) {
  if (actionId.length() == 0) {
    Serial.println("[WS] No action_id -> skip sendActionResult");
    return;
  }

  StaticJsonDocument<192> doc;

  // Payload cuối cùng bạn yêu cầu
  doc["action_id"] = actionId;
  doc["success"]   = success;
  doc["message"]   = message;

  String out;
  serializeJson(doc, out);
  ws.sendTXT(out);
  Serial.printf("[WS->] %s\n", out.c_str());
}

// Tạo lịch dừng sau ms, lưu intent + action_id
void scheduleStop(unsigned long ms, const String& intent, const String& actionId) {
  if (ms == 0) {
    // Không auto stop, coi như xử lý tức thì
    actionUntil     = 0;
    currentIntent   = "";
    currentActionId = "";
    Serial.printf("[SCHED] no auto stop for intent=%s (duration=0)\n", intent.c_str());
    // Trường hợp distance/angle = 0, có thể coi là hoàn thành luôn
    sendActionResult(actionId, true, "OK");
    return;
  }

  actionUntil     = millis() + ms;
  currentIntent   = intent;
  currentActionId = actionId;
  Serial.printf("[SCHED] stop in %lu ms for intent=%s, action_id=%s\n",
                ms, intent.c_str(), actionId.c_str());
}

// ================== PARSE & EXECUTE INTENT ==================
void handleIntent(const String& intent, JsonObject params, const String& actionId) {
  // "tien" (đi thẳng), "lui", "re_phai", "re_trai", "stop", "set_speed"
  if (intent == "tien") {
    float distance = params["distance"] | 0;
    const char* unit = params["unit"] | "m";
    Serial.printf("[INTENT] tien distance=%.2f %s, action_id=%s\n",
                  distance, unit, actionId.c_str());

    // Nếu distance <= 0 thì coi như xong luôn, không chạy
    if (distance <= 0) {
      stopMotor();
      sendActionResult(actionId, true, "OK");
      return;
    }

    forward();
    unsigned long dur = (unit[0] == 'm')
                        ? (unsigned long)(distance * MS_PER_METER)
                        : (unsigned long)distance; // fallback ms
    scheduleStop(dur, intent, actionId);
  }
  else if (intent == "lui") {
    float distance = params["distance"] | 0;
    const char* unit = params["unit"] | "m";
    Serial.printf("[INTENT] lui distance=%.2f %s, action_id=%s\n",
                  distance, unit, actionId.c_str());

    if (distance <= 0) {
      stopMotor();
      sendActionResult(actionId, true, "OK");
      return;
    }

    backward();
    unsigned long dur = (unit[0] == 'm')
                        ? (unsigned long)(distance * MS_PER_METER)
                        : (unsigned long)distance;
    scheduleStop(dur, intent, actionId);
  }
  else if (intent == "re_phai") {
    float angle = params["angle"] | 0;
    const char* unit = params["unit"] | "deg";
    Serial.printf("[INTENT] re_phai angle=%.2f %s, action_id=%s\n",
                  angle, unit, actionId.c_str());

    if (angle <= 0) {
      stopMotor();
      sendActionResult(actionId, true, "OK");
      return;
    }

    turnRight();
    unsigned long dur = (unit[0] == 'd')
                        ? (unsigned long)(angle * MS_PER_DEGREE)
                        : (unsigned long)angle;
    scheduleStop(dur, intent, actionId);
  }
  else if (intent == "re_trai") {
    float angle = params["angle"] | 0;
    const char* unit = params["unit"] | "deg";
    Serial.printf("[INTENT] re_trai angle=%.2f %s, action_id=%s\n",
                  angle, unit, actionId.c_str());

    if (angle <= 0) {
      stopMotor();
      sendActionResult(actionId, true, "OK");
      return;
    }

    turnLeft();
    unsigned long dur = (unit[0] == 'd')
                        ? (unsigned long)(angle * MS_PER_DEGREE)
                        : (unsigned long)angle;
    scheduleStop(dur, intent, actionId);
  }
  else if (intent == "stop") {
    Serial.printf("[INTENT] stop, action_id=%s\n", actionId.c_str());
    stopMotor();
    actionUntil     = 0;
    currentIntent   = "";
    currentActionId = "";
    // stop là lệnh tức thì → gửi OK luôn
    sendActionResult(actionId, true, "OK");
  }
  else if (intent == "set_speed") {
    int v = params["pwm"] | speedNormal;
    v = constrain(v, 0, 255);

    // cập nhật speedNormal + speedTurn dựa trên v
    speedNormal = v;
    speedTurn   = min(255, v + 60);   // rẽ nhanh hơn đi thẳng

    if (lastCmd == "forward" || lastCmd == "backward") {
      pwmWriteBoth(speedNormal);
    } else if (lastCmd == "left" || lastCmd == "right") {
      pwmWriteBoth(speedTurn);
    }

    Serial.printf("[INTENT] set_speed normal=%d, turn=%d, action_id=%s\n",
                  speedNormal, speedTurn, actionId.c_str());
    // Hoàn thành ngay
    sendActionResult(actionId, true, "OK");
  }
  else {
    Serial.printf("[INTENT] unknown: %s, action_id=%s\n",
                  intent.c_str(), actionId.c_str());
    // Lệnh không biết → báo lỗi
    sendActionResult(actionId, false, "unknown_intent");
  }
}

// ================== WEBSOCKET EVENTS ==================
void wsEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED: {
      Serial.printf("[WS] Connected to wss://%s%s\n", WS_HOST, WS_PATH);
      // Không gửi gì lên server ở đây để tránh format không đúng mong đợi
      break;
    }
    case WStype_DISCONNECTED:
      Serial.println("[WS] Disconnected");
      break;

    case WStype_TEXT: {
      if (length == 0) return;

      String msg((char*)payload, length);
      Serial.printf("[WS<-] %s\n", msg.c_str());

      // Nếu không bắt đầu bằng '{' hoặc '[' thì KHÔNG phải JSON → bỏ qua
      char c0 = (char)payload[0];
      if (c0 != '{' && c0 != '[') {
        Serial.println("[WS] Non-JSON text from server -> ignore");
        return;
      }

      StaticJsonDocument<512> doc;
      DeserializationError err = deserializeJson(doc, msg);
      if (err) {
        Serial.printf("[JSON] parse error on incoming: %s\n", err.c_str());
        // KHÔNG gửi error lại, tránh vòng lặp
        return;
      }

      const char* intent   = doc["intent"]    | "";
      const char* actionId = doc["action_id"] | "";
      JsonObject params    = doc["params"].isNull() ? JsonObject() : doc["params"].as<JsonObject>();

      handleIntent(String(intent), params, String(actionId));
      break;
    }

    case WStype_PING:
      Serial.println("[WS] PING");
      break;
    case WStype_PONG:
      Serial.println("[WS] PONG");
      break;
    case WStype_ERROR:
      Serial.println("[WS] ERROR");
      break;
    default:
      break;
  }
}

// ================== SETUP / LOOP ==================
void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println();
  Serial.println("[BOOT] ESP32 Robot (WS Client)");

  // Pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  Serial.println("[PIN] motor pins set OUTPUT");

  // PWM
  ledcSetup(PWM_CH_A, PWM_FREQ, PWM_BITS);
  ledcSetup(PWM_CH_B, PWM_FREQ, PWM_BITS);
  ledcAttachPin(ENA, PWM_CH_A);
  ledcAttachPin(ENB, PWM_CH_B);
  pwmWriteBoth(DUMMY_SPEED_PWM);
  Serial.printf("[PWM] %dHz, %dbit, init=%d\n", PWM_FREQ, PWM_BITS, DUMMY_SPEED_PWM);

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.printf("\n[WiFi] Connected. IP=%s\n", WiFi.localIP().toString().c_str());

  // WebSocket Client (WSS) — GIỮ NGUYÊN PHẦN CONNECT
  ws.beginSSL(WS_HOST, WS_PORT, WS_PATH); // wss
  ws.onEvent([](WStype_t type, uint8_t * payload, size_t length){ wsEvent(type, payload, length); });
  ws.setReconnectInterval(2000);          // tự reconnect
  ws.enableHeartbeat(15000, 3000, 2);     // ping/pong

  idle();
  Serial.println("[STATE] idle ready");
}

void loop() {
  ws.loop();

  // Kiểm tra lịch dừng
  if (actionUntil != 0 && millis() >= actionUntil) {
    Serial.printf("[SCHED] action time reached for intent=%s, action_id=%s\n",
                  currentIntent.c_str(), currentActionId.c_str());
    stopMotor();

    // Gửi kết quả hoàn thành cho lệnh đó
    sendActionResult(currentActionId, true, "OK");

    actionUntil     = 0;
    currentIntent   = "";
    currentActionId = "";
  }
}
