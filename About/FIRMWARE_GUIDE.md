# Vai trò từng file và hợp đồng API

## Cấu trúc và chiều phụ thuộc

```text
Firmware/
  Core/Inc, Src/           boot, exceptions, RTOS configuration
  Config/                 board_config.h, app_config.h, project_types.h
  Drivers/MCAL/Inc, Src/   gpio, pwm, i2c, exti, timebase, uart_debug
  Drivers/Devices/Inc, Src/
                          motor_tb6612, ultrasonic_hcsr04, mpu6050,
                          button, status_led, buzzer
  App/Inc, Src/           robot_car, sensor_manager,
                          obstacle_avoidance, safety_monitor
  ThirdParty/             CMSIS, HAL, FreeRTOS kernel có sẵn
  cmake/, linker/         toolchain và bố trí bộ nhớ có sẵn
  tools/                  check_constraints.sh
  CMakeLists.txt, CMakePresets.json
```

Chiều phụ thuộc: **Core → App → Devices → MCAL → Config**. Một tầng được dùng Config trực tiếp; không include ngược lên App từ driver. Config kiểu dữ liệu không phụ thuộc kernel. HAL/CMSIS và FreeRTOS là nền dùng tại những tầng cần thiết. `main.c` gọi MCAL để bring-up; `gpio_emergency_stop` là ngoại lệ đường khẩn cấp chỉ được tắt cầu H, không được chạy motor.

Các hàng driver/App dưới đây tương ứng **cả file .h trong Inc và .c trong Src**. Header là hợp đồng chia việc; implementation chưa xong phải trả NOT_READY, không trả OK giả.

## Core và Config

| File | Đã có | Nhân/Hưng cần tự làm |
| --- | --- | --- |
| `Core/Inc/FreeRTOSConfig.h` | Static allocation, 1 kHz tick, 5 mức ưu tiên, mutex/counting semaphore/timer, stack hook, assert và alias vector | Nhân đo stack, latency và kiểm tra scheduler trên board |
| `Core/Inc/main.h` | Clock prototype, biến chẩn đoán assert | Nhân chỉ mở rộng khi cần bring-up |
| `Core/Src/main.c` | HAL/clock HSE×9, GPIO/UART init, idle/timer memory callbacks, 5 Task static | Nhân nối task entry với module; Hưng cung cấp phần task sensor/log/buzzer qua PR |
| `Core/Inc/stm32f1xx_it.h` | Prototype vector của STM32F103 medium-density | Nhân giữ đồng bộ với startup |
| `Core/Src/stm32f1xx_it.c` | SysTick bridge; IRQ khác dừng ở assert | Chủ module cung cấp ISR tối thiểu, Nhân review trước khi enable NVIC |
| `Config/board_config.h` | Pinout/timer/priority dự kiến | Nhân sở hữu; Hưng đối chiếu sensor/bus thực tế |
| `Config/app_config.h` | Task layout, ngưỡng [DO], RAM budget | Nhân sở hữu; cả hai đưa số đo kèm lý do khi sửa |
| `Config/project_types.h` | status_t, sample_t, imu_sample_t | Nhân quản lý giao diện; Hưng review đơn vị/dấu/timestamp |
| `Core/Inc/stm32f1xx_hal_conf.h` | Config vendor hiện có, giữ nguyên | Chỉ bổ sung I2C/TIM trong PR triển khai ngoại vi sau này |

## MCAL

| Cặp file | API/hành vi hiện tại | Owner và bước tiếp theo |
| --- | --- | --- |
| `gpio.h/.c` | Init output an toàn, input nút; read/write thật; emergency stop kéo STBY thấp | Nhân: xác minh mức điện và pin trên board |
| `pwm.h/.c` | Config Hz, write channel/duty 0..1000, stop; stub | Nhân: TIM3, hai kênh, zero duty trước enable |
| `i2c.h/.c` | Địa chỉ 7-bit, register, buffer, timeout ms; stub | Hưng: I2C1, bounded transfer, bus recovery |
| `exti.h/.c` | Chỉ line 8; priority 5..15; read event/IRQ capture stub | Nhân: clear pending, event FromISR; không debounce trong ISR |
| `timebase.h/.c` | now_ms dùng HAL tick thật; init chỉ xác nhận config; capture_us stub | Nhân: TIM2 microsecond capture, rollover; Hưng review hợp đồng Echo |
| `uart_debug.h/.c` | USART1 115200 polling TX thật, timeout; uart_log, uart_log_u32 format riêng; RX stub | Hưng: một writer tLog, đo overhead, định dạng telemetry |

`timebase_init` trả OK cho timebase millisecond; không có nghĩa TIM2/capture đã sẵn sàng. `timebase_capture_us` hiện luôn NOT_READY. I2C read không ghi buffer khi NOT_READY. UART log chuỗi tối đa 127 ký tự; số u32 xuất thập phân kèm CRLF. Không gọi UART từ ISR hoặc nhiều Task cùng lúc.

## Devices

| Cặp file | API/hành vi hiện tại | Owner và việc cần triển khai |
| --- | --- | --- |
| `motor_tb6612.h/.c` | Enum MOTOR_*, config callback an toàn; init/apply luôn tắt STBY; NOT_READY | Nhân: truth table, PWM, đổi hướng, fault override và thử trên giá đỡ |
| `ultrasonic_hcsr04.h/.c` | init/request/read; mẫu mặc định NOT_READY, đơn vị mm | Hưng: trigger/capture, timeout, chống overlap và lọc mẫu |
| `mpu6050.h/.c` | init/read/calibrate; 3 accel + 3 gyro signed raw, temperature raw, timestamp/status | Hưng: identity, ranges, scale, bias, hướng trục, góc nghiêng |
| `button.h/.c` | **Đã có**: debounce theo thời gian bằng polling `gpio_read` + `timebase_now_ms`, wrap-safe; trả mức đã lọc | Nhân: EXTI event capture khi NVIC được bật; đo thời gian dội thật để chốt `BUTTON_DEBOUNCE_MS` |
| `status_led.h/.c` | **Đã có**: IDLE nháy chậm, RUNNING sáng, FAULT nháy nhanh; tính pha từ `now_ms`, không block, không sở hữu timer | Nhân: xác nhận cực tính active-low trên board thật |
| `buzzer.h/.c` | loại active-high, pattern, update theo now_ms; stub | Hưng: pattern không block và mạch kích phù hợp |

Driver không đoán sensor đã hợp lệ. Return code nói giao dịch/API có thành công hay không; sample.status nói phép đo có dùng được hay không. Với IMU, raw chưa phải độ/s hay m/s²; chỉ convert sau khi xác nhận range/scale.

## App

| Cặp file | Đã có | Owner và việc tiếp theo |
| --- | --- | --- |
| `sensor_manager.h/.c` | Hai mailbox static dài 1, khởi tạo NOT_READY, get_latest dùng peek | Hưng: tSensor là producer duy nhất; overwrite bản copy với timestamp lúc lấy mẫu |
| `safety_monitor.h/.c` | **Đã có**: fault latch, freshness theo `SENSOR_STALE_MS`, tilt scale-independent, rearm có chủ đích qua cạnh lên của nút; chỉ clear bit đã chứng minh là hết | Nhân: đo độ trễ STOP; Hưng review ngưỡng tilt và hướng trục |
| `obstacle_avoidance.h/.c` | **Đã có**: FSM 7 trạng thái, hysteresis `D_STOP_MM`/`D_CLEAR_MM`, deadline `T_TURN_MS`, retry budget, CHECK chỉ nhận mẫu lấy sau khi vào trạng thái; FAULT là terminal | Nhân: chốt ngưỡng bằng số đo; Hưng review các case sensor lỗi |
| `robot_car.h/.c` | **Đã có**: rearm theo cạnh, snapshot → FSM → đúng **một** lần `motor_apply` trên mọi nhánh, cập nhật LED | Nhân: nối vào tDecision và chứng minh thời gian shutdown trước khi có motor thật |

Handles mailbox/event group công khai để học và debug, nhưng ownership vẫn bắt buộc: ngoài sensor_manager chỉ đọc mailbox; ngoài safety_monitor không clear/set bit trực tiếp. ISR nên notify Task an toàn để Task cập nhật state; không nhầm event-group FromISR được xử lý ngay như một GPIO kill.

Năm module ghi **Đã có** ở trên biên dịch sạch với `-Wall -Wextra -Wpedantic -Wshadow -Wdouble-promotion` nhưng **chưa được Task nào gọi**: `task_safety` và `task_decision` trong `main.c` vẫn là vòng chờ placeholder, nên `--gc-sections` loại `safety_update`, `robot_car_update`, `obstacle_avoidance_update`, `button_read` và `status_led_set` khỏi ELF. Build sạch ở đây chứng minh mã hợp lệ, **không** chứng minh mã đã chạy. Việc nối vào Task là bước tích hợp riêng, làm cùng lúc với đo độ trễ STOP.

`safety_update` trả `STATUS_OK` nghĩa là chính sách đã chạy, không phải xe an toàn; điều kiện chạy chỉ đọc qua `safety_is_clear_to_run()`.

`sensor_manager_get_latest` trả OK khi **copy được** hai mailbox. Người dùng phải kiểm tra status và tuổi của từng mẫu; hai mẫu không được coi là lấy cùng thời điểm. Consumer dùng peek để tSafety và tDecision đều đọc được; receive sẽ làm mất mẫu cho consumer còn lại.

## Task và bộ nhớ

| Task | Priority | Stack words | Hiện tại | Mục tiêu sau khi triển khai |
| --- | --- | --- | --- | --- |
| tSafety | 4 | 256 | delay 100 ms | Nhân: bắt lỗi/STOP; mục tiêu 10 ms cần đo |
| tSensor | 2 | 256 | delay 100 ms | Hưng: IMU ~10 ms, range ~60 ms bằng deadline riêng |
| tDecision | 1 | 256 | delay 100 ms | Nhân: FSM ~20 ms, không ngủ hết thời gian TURN |
| tLog | 0 | 256 | delay 100 ms | Hưng: telemetry có giới hạn, nhường CPU |
| tBuzzer | 0 | 256 | delay 100 ms | Hưng: pattern theo deadline, nhường CPU |

Timer service P3 và Idle P0 dùng stack 128 words mỗi Task. Một word trên port này là 4 byte. Tổng stack cấp tĩnh là 6144 byte; còn TCB, queue, event group, globals và MSP. App không dùng timer service để chạy thuật toán nặng.

Xem phép tính trong app_config, static assert trong main và [số đo ELF](BUILD_VERIFICATION.md). Khi có logic thật, stack 256 words chỉ là điểm bắt đầu: đo high-water mark dưới tải và kiểm tra MSP; compile thành công không chứng minh không tràn stack.

## CMake và vendor

CMake đã thêm toàn bộ 18 file .c do dự án sở hữu vào target (main, IRQ, 6 MCAL, 6 Devices, 4 App). Bản build cũ chỉ có main/startup. Preset, linker, toolchain, ThirdParty và HAL config không đổi. Tên/comment mốc A2 cũ còn trong CMake để giữ phạm vi ngoại lệ tối thiểu; file FreeRTOSConfig hiện tồn tại nên nhánh build kernel được kích hoạt.
