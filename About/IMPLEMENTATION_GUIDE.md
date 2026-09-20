# Hướng dẫn triển khai cho Nhân và Hưng

## 1. Chuẩn bị bản làm việc

Cần Git, VS Code, CMake >= 3.22, Ninja và Arm GNU Toolchain có `arm-none-eabi-gcc`, `arm-none-eabi-size`, `arm-none-eabi-nm`. Nạp/debug cần ST-Link và công cụ phù hợp như STM32CubeProgrammer; repo chưa cung cấp launch.json/debug profile đã kiểm chứng.

```sh
git clone https://github.com/NguyenTrongNhan2006/STM32-Obstacle-Avoidance-Car.git
cd STM32-Obstacle-Avoidance-Car
code .
```

Nếu đã clone, kiểm tra `git status`, commit/cất thay đổi cá nhân trước khi chạy `git pull --ff-only`. Không force push để giải quyết phân kỳ. Mỗi người làm một feature branch; ví dụ `feature/nhan-pwm` và `feature/hung-mpu6050`.

## 2. Build và kiểm tra

Chạy trong thư mục `Firmware/`; thêm CMake, Ninja và Arm GNU Toolchain vào PATH:

```sh
cmake --preset debug
cmake --build --preset debug
arm-none-eabi-size build/debug/obstacle_car.elf
cmake --preset release
cmake --build --preset release
```

Chạy script bằng **Git Bash** trên Windows hoặc Bash trên Linux, vẫn ở `Firmware/`:

```sh
test -f build/debug/obstacle_car.elf
bash tools/check_constraints.sh build/debug/obstacle_car.elf
```

Phải có ELF và phần “Build” được kiểm tra. Script hiện có thể báo “Tat ca dat.” sau khi bỏ qua ELF không tồn tại; không nhận kết quả đó làm bằng chứng build. Nếu thiếu `dirname/grep/find`, dùng Git Bash đầy đủ (có /usr/bin trong PATH), không sửa script để bỏ kiểm tra.

Kết quả: `build/debug/obstacle_car.elf`, `.hex`, `.bin`, `.elf.map`; bản release tương tự. File .su ghi stack ước tính của từng hàm. Không commit build artifact vào repo. [Biên bản build](BUILD_VERIFICATION.md) ghi kết quả của skeleton này.

Project sources dùng `-Wall -Wextra -Wpedantic -Wshadow -Wdouble-promotion`. CMake hiện giữ `-w` cho vendor để không chỉnh mã bên thứ ba. Cần đọc lỗi đầu tiên của compiler/linker, không thêm source thư viện tùy ý để “cho qua”.

## 3. Bring-up lần đầu trên board — chưa thực hiện

1. Đối chiếu board_config với schematic và linh kiện thật. Kiểm tra HSE 8 MHz, 3.3 V, GND, Echo level shifting, pull-down STBY. Chưa cấp nguồn motor.
2. Nạp ELF/HEX bằng ST-Link. Nếu MCU khác mã C8 hoặc clock khác, dừng để sửa cấu hình có bằng chứng.
3. Breakpoint ở main và sau SystemClock_Config; kiểm tra SystemCoreClock = 72 MHz. Nếu assert, xem `g_assert_file/g_assert_line`; không tắt assert để né lỗi.
4. Quan sát STBY luôn thấp. UART 115200 8N1 dự kiến in “Skeleton: motor disabled, algorithms TODO”.
5. Xác nhận SysTick chạy trước scheduler (HAL timeout còn tiến), và sau scheduler cả HAL tick/kernel tick vẫn tăng với tần số dự kiến.
6. Kiểm tra debugger thấy năm Task ứng dụng, Idle, timer service. tSafety/tSensor/tDecision/tLog/tBuzzer hiện chỉ chờ 100 ms; không mong xe tự chạy.
7. Ghi board revision, commit SHA, công cụ, nguồn cấp và kết quả vào biên bản. Chỉ sau khi đạt mới nối từng driver thật.

Nếu HSE không khởi động, HAL timeout rồi assert là hành vi dự kiến. Không sửa vòng chờ thành vô hạn để che mất lỗi clock. Mọi peripheral IRQ ngoài SysTick đang trap; phải triển khai route và clear pending trước khi bật nguồn ngắt.

## 4. Quy trình triển khai một module

1. Owner đọc hợp đồng .h và input/output/timeout hiện tại.
2. Chốt phần cứng và đơn vị dữ liệu với người review; sửa shared header trước nếu cần.
3. Thay TODO trong .c; giữ trả lỗi tường minh. Không dùng NOT_READY để giả lập số đo hợp lệ.
4. Build cả Debug/Release, chạy constraints; thêm test có ý nghĩa cho logic mới.
5. Thử riêng trên bench, chụp waveform/log; ghi case lỗi và thời gian tối đa.
6. PR nhỏ, người còn lại review, rồi tích hợp vào đúng Task. Không cùng sửa main/config trong hai PR đang mở.

Không đồng loạt nối tất cả TODO vào Task. Sau từng module, giữ điểm mốc build được và kiểm tra đường STOP còn hoạt động.

## 5. Các mốc kỹ thuật

| Mốc | Nhân | Hưng | Điều kiện chuyển mốc |
| --- | --- | --- | --- |
| A — nền | Xác minh clock, RTOS, GPIO, SWD | Xác minh UART và sơ đồ nguồn/sensor | Boot ổn, motor tắt, ghi build + board evidence |
| B — MCAL | PWM TIM3, EXTI8, TIM2 capture | I2C1 + lỗi/timeout, định dạng log | Chứng minh ngoại vi riêng, chưa chạy FSM |
| C — Devices | Cầu H, button, LED trên giá đỡ | HC-SR04, MPU6050, buzzer | Có mẫu hợp lệ và trạng thái lỗi phân biệt |
| D — dữ liệu | Safety event contract | Hai mailbox, timestamp, stale detection inputs | Nhiều consumer đọc không mất mẫu |
| E — ứng dụng | FSM deadline và một output boundary | Sensor scheduling + fault injection | STOP/fault thắng motion; không tự restart |
| F — xe cơ bản | Điều khiển ở tốc độ thấp | Đo range/tilt/log và false alarm | Qua checklist xe cơ bản trước 07/11 |
| G — mở rộng | Gyro-assisted turn nếu đủ điều kiện | Bias/relative angle + plot telemetry | Có số liệu so sánh với baseline |
| H — hoàn thiện | Regression, memory, release | Soak, dữ liệu, video và báo cáo | Freeze tính năng 05/12; bàn giao 20/12 |

Việc bật I2C/TIM thật cần thêm HAL module hoặc cách triển khai ngoại vi đã thống nhất; HAL config hiện chưa bật hai nhóm này. Đó là công việc của mốc B, không phải tính năng đã có trong skeleton.

## 6. Quy tắc timing và ngắt

- Với periodic work dùng delay-until để giảm trôi chu kỳ; nhiều tốc độ sensor dùng deadline riêng. Vòng chờ 100 ms hiện chỉ là placeholder.
- Thời lượng TURN là deadline trong context; không sleep toàn bộ thời gian quay, vì cần tiếp tục xét STOP/fault.
- So sánh thời gian unsigned theo elapsed để xử lý tick wrap. Timeout của giao dịch và độ mới của mẫu là hai điều kiện khác nhau.
- ISR chỉ capture/timestamp/notify/clear pending rồi yield nếu cần. Không chạy I2C, formatter, lọc IMU hoặc FSM trong ISR.
- Priority CMSIS có số càng nhỏ càng khẩn cấp. ISR gọi API RTOS FromISR phải dùng số 5..15 và grouping 4; 0..4 không được gọi RTOS.
- SVC/PendSV do port sở hữu; không thêm wrapper C. SysTick tăng HAL tick một lần rồi forward vào kernel sau khi scheduler chạy.
- Trước khi cho motor chạy phải thống nhất shutdown khi tDecision bị treo/trễ. Callback chỉ bảo vệ lúc gọi motor_apply; cân nhắc đường inhibit phần cứng/ISR tối thiểu và watchdog ở PR an toàn, đo bằng waveform. Skeleton chưa thực hiện cơ chế đó.

## 7. Quy tắc bộ nhớ và log

Chỉ dùng cấp phát static; không thêm heap implementation, libc formatting hoặc HAL busy delay. Queue dài 1 giữ mẫu mới nhất theo bản copy, không trỏ vào stack của producer. Không tự đổi sample enum/đơn vị giữa hai nhánh.

Đo high-water mark của từng Task khi test lỗi, cộng call depth ISR/MSP và log. Stack overflow hook là chốt chẩn đoán cuối cùng, không thay cho việc đo. Timer callback không được block; UART runtime chỉ có tLog ghi.

## 8. Review và tích hợp

PR ghi: thay đổi hành vi, owner file, cách thử, số đo, case lỗi và giới hạn. Người review thử ít nhất một case lỗi mà tác giả chưa dùng. Nhân quản lý main/config/IRQ/CMake; Hưng gửi yêu cầu source/pin/API trong PR, không sửa song song gây xung đột.

Mỗi tuần dành một buổi ghép xe và đo chung. Kết quả chưa đạt vẫn ghi rõ; không sửa test cho khớp số đẹp. Xem [timeline](TEAM_TIMELINE.md) để biết khi nào phải bỏ tính năng mở rộng.
