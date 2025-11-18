# ESP32 Robot Car Control - WebSocket Client

Dự án điều khiển robot xe bằng ESP32 kết nối tới server qua WebSocket (WSS).

## 📋 Mục Lục

- [Yêu Cầu Hệ Thống](#requirements)
- [Chuẩn Bị Phần Cứng](#hardware)
- [Cấu Hình Dự Án](#configuration)
- [Triển Khai Ứng Dụng](#deployment)
- [Cấu Trúc Lệnh WebSocket](#ws-commands)
- [Ghi Chú Quan Trọng](#important-notes)

---

<a id="requirements"></a>
## ⚙️ Yêu Cầu Hệ Thống 

### Phần Mềm
- **PlatformIO CLI** hoặc **Visual Studio Code + PlatformIO Extension**
- **Python 3.x** (yêu cầu bởi PlatformIO)

### Cài Đặt PlatformIO
```bash
# Nếu chưa cài PlatformIO CLI
pip install platformio

# Hoặc sử dụng VSCode Extension
# Mở VSCode → Extensions → Tìm "PlatformIO IDE" → Install
```

---

<a id="hardware"></a>
## 🔧 Chuẩn Bị Phần Cứng

### Linh Kiện Cần Thiết
- **ESP32 DevKit** (hoặc tương tương)
- **Module L298N** (điều khiển động cơ DC)
- **2x Động Cơ DC** (3-6V)
- **Pin Header** + **Dây Jumper**
- **Cáp USB Micro-B** (để lập trình ESP32)
- **Pin/Ắc quy** (5V-12V tùy động cơ)

### Sơ Đồ Kết Nối

| ESP32 GPIO | L298N Pin | Chức Năng |
|-----------|-----------|----------|
| GPIO 5    | ENA       | PWM Motor A |
| GPIO 23   | ENB       | PWM Motor B |
| GPIO 22   | IN1       | Motor A Dir 1 |
| GPIO 21   | IN2       | Motor A Dir 2 |
| GPIO 19   | IN3       | Motor B Dir 1 |
| GPIO 18   | IN4       | Motor B Dir 2 |
| GND       | GND       | Ground |

---

<a id="configuration"></a>
## 📝 Cấu Hình Dự Án

### Bước 1: Clone/Copy Dự Án
```bash
cd d:/temp/code/iot/Project/car_control
```

### Bước 2: Tạo File Cấu Hình Môi Trường

**Cách A: Sử dụng `config.env.h` (Khuyến Nghị - An Toàn)**

1. Copy template từ `include/config.env.h` hoặc `include/config.example.h`:
  ```bash
  cp include/config.env.h include/config.env.h
  ```

2. Chỉnh sửa file `include/config.env.h` với thông tin thực tế:
  ```cpp
  // ================== WIFI ==================
  const char* WIFI_SSID = "YOUR_NETWORK_NAME";      // Tên WiFi của bạn
  const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";    // Mật khẩu WiFi

  // ================== WS SERVER ==================
  const char* WS_HOST = "your.server.com";         // Hostname server
  const uint16_t WS_PORT = 443;                     // Port (443=WSS/HTTPS)
  const char* WS_PATH = "/api/ws/robot/YOUR_ROBOT_ID"; // Endpoint + robot ID
  ```

3. **Thay đổi include trong `src/main.cpp`**:
  ```cpp
  // Thay từ:
  #include "config.h"
  
  // Sang:
  #include "config.env.h"
  ```

4. Đảm bảo `include/config.env.h` ở trong `.gitignore` (đã setup sẵn):
  ```bash
  cat .gitignore | grep config.env.h
  # Kết quả: include/config.env.h
  ```

**Cách B: Sửa trực tiếp `include/config.h` (Nhanh nhưng Ít An Toàn)**

Chỉnh sửa file `include/config.h` trực tiếp với thông tin thực tế.
⚠️ **Lưu Ý**: File này sẽ được commit lên Git → **Không an toàn cho credentials!**

### Bước 3: Hiệu Chỉnh Motor (Tùy Chọn)

Nếu robot không chuyển động đúng, điều chỉnh các giá trị:

```cpp
// Trong include/config.h hoặc config.env.h

// Tốc độ chạy thẳng (0-255, 0=dừng, 255=max)
const int SPEED_NORMAL = 120;

// Tốc độ rẽ (thường cao hơn để rẽ nhanh hơn)
const int SPEED_TURN = 180;

// Hiệu chỉnh khoảng cách
const float MS_PER_METER = 1000.0f;  // Thời gian (ms) để đi 1 mét
// Cách tính: Chạy robot 1m thẳng, tính thời gian, đặt vào đây

// Hiệu chỉnh góc quay
const float MS_PER_DEGREE = 8.0f;    // Thời gian (ms) để quay 1 độ
// Cách tính: Quay robot 360°, tính thời gian, chia 360 = thời gian/độ
```

---

<a id="deployment"></a>
## 🚀 Triển Khai Ứng Dụng

### Bước 1: Kiểm Tra Kết Nối USB

Kết nối ESP32 vào máy tính qua USB Micro-B. Kiểm tra port:

**Trên Windows:**
```bash
# Liệt kê các cổng COM
mode
# Hoặc kiểm tra Device Manager → Ports (COM & LPT)
```

**Trên Linux/Mac:**
```bash
ls /dev/tty.* /dev/ttyUSB*
```

### Bước 2: Build Firmware

```bash
cd d:/temp/code/iot/Project/car_control
pio run --environment esp32dev
```

**Output mong đợi:**
```
Processing esp32dev
...
Building .pio/build/esp32dev/firmware.bin
[SUCCESS] Built target firmware
```

### Bước 3: Upload Firmware vào ESP32

```bash
pio run --environment esp32dev --target upload
```

**Trong quá trình upload:**
- ESP32 tự động vào chế độ bootloader
- Firmware được ghi vào flash
- ESP32 tự động khởi động lại

**Output mong đợi:**
```
Uploading .pio/build/esp32dev/firmware.bin
...
[SUCCESS] Uploaded firmware
```

### Bước 4: Theo Dõi Serial Output

Mở Serial Monitor để xem log từ robot:

```bash
pio device monitor --environment esp32dev
```

**Log mong đợi:**
```
[BOOT] ESP32 Robot (WS Client)
[PIN] motor pins set OUTPUT
[PWM] 1000Hz, 8bit, init=120
[WiFi] Connecting to YOUR_NETWORK_NAME.....
[WiFi] Connected. IP=192.168.x.x
[WS] Connected to wss://your.server.com/api/ws/robot/YOUR_ID
[STATE] idle ready
```

### Bước 5: Kiểm Tra Kết Nối WebSocket

Gửi lệnh test từ server:

```json
{
  "intent": "tien",
  "action_id": "test_001",
  "params": {
   "distance": 0.5,
   "unit": "m"
  }
}
```

Robot sẽ in log:
```
[INTENT] tien distance=0.50 m, action_id=test_001
[MOTOR] FORWARD @PWM=120
[SCHED] stop in 500 ms for intent=tien, action_id=test_001
[SCHED] action time reached for intent=tien, action_id=test_001
[MOTOR] STOP
[WS->] {"action_id":"test_001","success":true,"message":"OK"}
```

---

<a id="ws-commands"></a>
## 📡 Cấu Trúc Lệnh WebSocket

### Định Dạng Chung

```json
{
  "intent": "tien|lui|re_phai|re_trai|stop|set_speed",
  "action_id": "unique_id_string",
  "params": {
   // Các tham số tùy intent
  }
}
```

### Các Intent Hỗ Trợ

#### 1. **tien** - Đi Thẳng Phía Trước

```json
{
  "intent": "tien",
  "action_id": "forward_01",
  "params": {
   "distance": 1.5,
   "unit": "m"
  }
}
```

**Response:**
```json
{
  "action_id": "forward_01",
  "success": true,
  "message": "OK"
}
```

#### 2. **lui** - Lùi Lại

```json
{
  "intent": "lui",
  "action_id": "backward_01",
  "params": {
   "distance": 1.0,
   "unit": "m"
  }
}
```

#### 3. **re_phai** - Rẽ Phải

```json
{
  "intent": "re_phai",
  "action_id": "turn_right_01",
  "params": {
   "angle": 90,
   "unit": "deg"
  }
}
```

#### 4. **re_trai** - Rẽ Trái

```json
{
  "intent": "re_trai",
  "action_id": "turn_left_01",
  "params": {
   "angle": 90,
   "unit": "deg"
  }
}
```

#### 5. **stop** - Dừng Ngay

```json
{
  "intent": "stop",
  "action_id": "stop_01",
  "params": {}
}
```

#### 6. **set_speed** - Đặt Tốc Độ

```json
{
  "intent": "set_speed",
  "action_id": "speed_01",
  "params": {
   "pwm": 150
  }
}
```

**Ghi Chú:**
- `pwm`: 0-255 (0=dừng, 255=max)
- Tốc độ rẽ sẽ tự động tăng (~60 điểm so với `pwm` mới)

---

<a id="important-notes"></a>
## 🔍 Ghi Chú Quan Trọng

### ⚠️ Bảo Mật Credentials

**ĐỪNG** commit file chứa credentials lên Git:

```bash
# Đúng: Sử dụng config.env.h (gitignored)
include/config.env.h  ← .gitignore đã thêm rule này

# Sai: Commit config.h với credentials thực tế
# → Credentials sẽ bị leak trên Git
```

### 🔌 Pinout ESP32

Nếu muốn đổi GPIO pins, sửa trong `include/config.h`:

```cpp
const int ENA = 5;    // Thay đổi GPIO pins ở đây
const int ENB = 23;
const int IN1 = 22;
const int IN2 = 21;
const int IN3 = 19;
const int IN4 = 18;
```

### 📊 Hiệu Chỉnh Motion

Robot sử dụng **time-based positioning** (không có encoder):

1. **Hiệu chỉnh MS_PER_METER:**
  ```
  Chạy robot đi thẳng 1m → Tính thời gian (ms) → Đặt vào MS_PER_METER
  Ví dụ: Đi 1m mất 1200ms → MS_PER_METER = 1200.0f
  ```

2. **Hiệu chỉnh MS_PER_DEGREE:**
  ```
  Quay robot 360° → Tính thời gian (ms) → Chia 360
  Ví dụ: Quay 360° mất 2880ms → MS_PER_DEGREE = 2880.0/360 = 8.0f
  ```

### 🔗 Kết Nối WebSocket

- **Cổng mặc định**: 443 (WSS - Secure WebSocket)
- **Cơ chế reconnect**: 2 giây (nếu kết nối bị mất)
- **Heartbeat**: PING/PONG mỗi 15 giây

### 📱 Serial Monitor Output

Các loại log chính:

| Prefix | Ý Nghĩa |
|--------|---------|
| `[BOOT]` | Khởi động |
| `[PIN]` | Cấu hình GPIO |
| `[PWM]` | Cấu hình PWM |
| `[WiFi]` | Kết nối WiFi |
| `[WS]` | WebSocket event |
| `[MOTOR]` | Lệnh motor |
| `[INTENT]` | Lệnh từ server |
| `[SCHED]` | Lập lịch dừng |
| `[JSON]` | Lỗi parse JSON |


## 📚 Nguồn tham khảo

- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [WebSocketsClient Library](https://github.com/Links2004/arduinoWebSockets)
- [ArduinoJson Library](https://arduinojson.org/)
