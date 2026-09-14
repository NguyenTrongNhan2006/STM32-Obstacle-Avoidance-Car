# Firmware — sườn để tự triển khai

[Về mục lục](README.md) · [Các bước tự triển khai](IMPLEMENTATION_GUIDE.md) · [Kế hoạch kiểm thử](TEST_PLAN.md)

Mỗi file `.c/.h` trong `Firmware/` chỉ chứa `// TO DO`. Tên module thể hiện cách chia công việc dự kiến, không có nghĩa đã hỗ trợ phần cứng hoặc có API chạy được. `CMakeLists.txt` chỉ chứa `# TO DO`; chưa thể build/nạp. Mọi hướng dẫn nằm trong `About/` để phần firmware dành hoàn toàn cho tác giả tự triển khai.

## Cấu trúc

```text
Firmware/
├── Core/
│   ├── Inc/       main.h, stm32f1xx_it.h
│   └── Src/       main.c, stm32f1xx_it.c
├── Drivers/
│   ├── MCAL/      Lớp ngoại vi STM32: Inc/*.h và Src/*.c
│   └── Devices/   Lớp linh kiện: Inc/*.h và Src/*.c
├── App/
│   ├── Inc/       Giao diện ứng dụng
│   └── Src/       Xử lý cảm biến, né vật cản, điều kiện dừng
├── Config/        board_config.h, app_config.h
└── CMakeLists.txt
```

Mỗi module bên dưới có một cặp `.c` / `.h` chỉ chứa dòng TODO.

Ví dụ đường dẫn đầy đủ: `Drivers/MCAL/Inc/gpio.h` và `Drivers/MCAL/Src/gpio.c` tính từ `Firmware/`. Quy tắc tương tự áp dụng cho tất cả module MCAL/Devices; các module ứng dụng nằm ở `App/Inc/` và `App/Src/`. Có 6 cặp MCAL + 7 cặp Devices + 4 cặp App + 2 cặp Core + 2 header Config = **40 file `.c/.h`**. Số file chỉ mô tả sườn, không phải số tính năng đã hoạt động.

## Drivers/MCAL — ngoại vi STM32

MCAL là lớp thao tác ngoại vi MCU. Sau này có thể viết thanh ghi trực tiếp hoặc bọc HAL/LL theo cách bạn chọn.

| Module | Nhiệm vụ dự kiến |
| --- | --- |
| `gpio` | Cấu hình chân, đọc input, ghi output |
| `pwm` | PWM phần cứng từ timer để điều khiển tốc độ động cơ |
| `i2c` | Giao tiếp MPU6050, có timeout và xử lý lỗi bus |
| `exti` | Cấu hình ngắt ngoài, nhận sự kiện cảm biến/nút |
| `timebase` | Tick hệ thống, đo thời gian và timeout; hạn chế delay chặn |
| `uart_debug` | Xuất log trạng thái/cảm biến để thử nghiệm |

## Drivers/Devices — linh kiện đề xuất

| Module | Nhiệm vụ dự kiến | Mức ưu tiên |
| --- | --- | --- |
| `motor_tb6612` | Driver cầu H TB6612FNG: hai motor, tốc độ/hướng, stop/standby | Cốt lõi nếu chọn TB6612 |
| `ultrasonic_hcsr04` | Phát trigger, đo echo, đổi sang khoảng cách, đánh dấu dữ liệu lỗi/quá hạn | Cốt lõi nếu chọn HC-SR04 |
| `ir_obstacle` | Đọc cảm biến IR trái/phải, xử lý cực tính tín hiệu và lọc nhiễu | Sau cảm biến trước |
| `mpu6050` | Khởi tạo/đọc gia tốc, gyro và hiệu chuẩn; cung cấp dữ liệu cho ứng dụng | Bước kết hợp MPU6050 |
| `button` | Nút START/STOP, chống dội và phát sự kiện nhấn | Cốt lõi |
| `status_led` | Báo trạng thái dừng, chạy, né, lỗi | Cốt lõi |
| `buzzer` | Còi cảnh báo khi mắc kẹt/lỗi hoặc trạng thái đặc biệt | Tùy chọn |

Tên driver phần cứng là đề xuất, chưa phải danh sách linh kiện đã xác nhận. Có thể thay `motor_tb6612` bằng driver tương ứng nếu dùng module khác; không cần viết nhiều driver motor khi chỉ dùng một loại.

## App — logic dự án

| Module | Nhiệm vụ dự kiến |
| --- | --- |
| `robot_car` | Điều phối ứng dụng: khởi tạo, cập nhật định kỳ, chế độ hoạt động |
| `sensor_manager` | Gom dữ liệu khoảng cách, IR, MPU6050; theo dõi thời điểm và độ hợp lệ |
| `obstacle_avoidance` | Quyết định dừng/chuyển hướng theo dữ liệu cảm biến, giới hạn số lần né |
| `safety_monitor` | Xử lý STOP, cảm biến lỗi, nghiêng/lật và điều kiện cho phép chạy lại |

`Core/Src/stm32f1xx_it.c` dành cho ISR. Nên giữ ISR ngắn: nhận/xóa cờ ngắt, chụp thời gian hoặc báo sự kiện; để thuật toán xe chạy ở `App/`.

## Config

- `board_config.h`: pinout, kênh timer, địa chỉ I2C, cực tính cảm biến; bổ sung sau khi chốt mạch.
- `app_config.h`: tốc độ PWM, khoảng cách dừng, hysteresis, timeout, ngưỡng nghiêng; chưa điền giá trị.

Hướng phụ thuộc dự kiến: `Core → App → Devices → MCAL`. Cấu hình được chia sẻ qua `Config/`. Tránh đưa thuật toán né vào driver cảm biến.

Đây là hướng phân lớp chính; `Core` vẫn phải khởi tạo nền tảng và chuyển sự kiện ngắt đến module sở hữu ngoại vi. Không nên làm một lớp bọc chỉ để tuân theo mũi tên nếu không giúp phân chia trách nhiệm.

| Khi cần thay đổi | Nơi dự kiến chỉnh | Nơi không nên chứa quyết định này |
| --- | --- | --- |
| Đổi dây, chân timer, mức kích hoạt | `board_config.h` và phần cấu hình ngoại vi tương ứng | Thuật toán né |
| Đổi ngưỡng dừng/thời gian quay | `app_config.h` và ghi lý do thử nghiệm trong About | Driver I2C/GPIO |
| Đổi loại cầu H | Driver Devices cho loại thực tế; giữ hợp đồng motor với App nếu phù hợp | Driver cảm biến |
| Lỗi giao dịch I2C | `i2c` trả trạng thái; `mpu6050` truyền lỗi lên | MCAL tự quyết định xe rẽ |
| Một cảm biến quá hạn | `sensor_manager` xác định tuổi mẫu; `safety_monitor` áp dụng chính sách | Tự sửa mẫu thành số đo giả |
| Chọn trái/phải | `obstacle_avoidance` dựa trên dữ liệu đã kiểm tra | ISR hoặc GPIO |

## Việc tiếp theo

1. Chốt phần cứng và lựa chọn HAL/LL hoặc thanh ghi trực tiếp.
2. Thêm CMSIS/startup/linker phù hợp STM32F103; giữ giấy phép gốc của thư viện.
3. Điền CMake và cấu hình toolchain.
4. Triển khai từng cặp driver, bắt đầu từ GPIO/timebase/UART rồi đến motor/cảm biến.

Xem [đề xuất dự án](PROJECT_PLAN.md). Chưa có khai báo hàm mẫu để bạn tự thiết kế API khi triển khai.

## Vai trò file nguồn và header khi bạn bắt đầu làm

- `*.h`: sau này khai báo interface công khai, kiểu dữ liệu, trạng thái lỗi và hợp đồng sử dụng module.
- `*.c`: sau này triển khai interface; giữ dữ liệu nội bộ và hàm hỗ trợ ở phạm vi file.
- `main.c`: điểm vào, gọi khởi tạo và lịch xử lý ứng dụng; hạn chế nhét toàn bộ driver vào đây.
- `main.h`: khai báo dùng chung ở lớp khởi động nếu cần; tránh biến thành nơi chứa mọi cấu hình.
- `stm32f1xx_it.c/.h`: handler ngắt và khai báo tương ứng. ISR ghi nhận sự kiện; thuật toán ở App.
- `board_config.h`: quyết định phần cứng sau khi xác nhận board và sơ đồ nối dây.
- `app_config.h`: tham số hành vi sau khi thử nghiệm, tách khỏi cấu hình chân.

## Hợp đồng module cần tự chốt trước khi viết

Đơn vị dữ liệu (mm, ms, duty cycle), giá trị hợp lệ, timeout, mã lỗi, quyền sở hữu ngoại vi và cách gọi từ ISR/main. Với sensor, phân biệt giá trị bằng 0 với dữ liệu không hợp lệ hoặc quá hạn. Với motor, định nghĩa rõ stop/coast/brake và trạng thái mặc định sau reset. Đây là các câu hỏi thiết kế, không phải API đã được ấn định.
