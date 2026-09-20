# Timeline Nhân và Hưng — 3 tháng

**Kế hoạch: 20/09/2026 đến 20/12/2026.** Gồm 12 tuần phát triển đến 12/12, sau đó 8 ngày dự phòng/bàn giao. Ngày bắt đầu được lấy theo lần cập nhật này; nếu lịch thực tế khác, đổi cả bảng và deadline cùng nhau.

Giả định lập kế hoạch: **8–10 giờ/người/tuần**, cả hai cùng mức cam kết; đây chưa phải số giờ hai bạn đã xác nhận. Khoảng 60% thời gian làm phần mình, 20% review/test phần bạn, 20% tích hợp và tài liệu. Cân bằng bằng thời gian và độ khó, không đếm số file hay dòng code.

## Ownership để làm song song

| Nhân | Hưng |
| --- | --- |
| Core, RTOS và cấu hình chung; GPIO/PWM/EXTI/TIM2 | I2C, UART telemetry và xử lý dữ liệu sensor |
| Motor, button, LED; FSM và safety_monitor | HC-SR04, MPU6050, buzzer và sensor_manager |
| CMake/pinout/API chung sau khi cùng review | BOM sensor, hiệu chuẩn, fault injection, đồ thị đo |
| Review lỗi sensor và dữ liệu Hưng gửi | Review STOP/timing/FSM và đo kiểm motor Nhân làm |

Cả hai đều viết code, đo phần cứng, review và viết tài liệu. Hưng không chỉ viết báo cáo; Nhân không ôm cả integration lẫn mọi driver. TIM2 do Nhân viết theo hợp đồng Echo Hưng chốt từ tuần 1; main/config/CMake có một owner để tránh conflict.

## Công việc theo tuần

| Tuần / ngày | Nhân — đầu ra cụ thể | Hưng — đầu ra cụ thể | Cổng tích hợp chung |
| --- | --- | --- | --- |
| 1 · 20–26/09 | Chốt board, nguồn, motor/driver, pin/timer và STBY | Chốt HC-SR04/MPU/buzzer; đơn vị/status/timestamp, BOM | Review API skeleton, schematic rev A, task list ước lượng giờ |
| 2 · 27/09–03/10 | Bring-up clock/SWD, GPIO, kiểm tra reset tắt motor | UART bench, xác minh mức logic sensor, chuẩn bị test cases | Build + nạp thật; clock/tick và nguồn ổn |
| 3 · 04–10/10 | Kiểm tra 5 Task, static RAM/stack, bắt đầu PWM zero-duty | I2C1, identity MPU6050, log signed raw | Waveform PWM trên bench, bus không treo khi lỗi |
| 4 · 11–17/10 | TIM2 capture, EXTI/button và debounce | HC-SR04 request/read/timeout, thử khoảng cách chuẩn | Echo có timeout/rollover; STOP event không mất |
| 5 · 18–24/10 | Driver motor trên giá đỡ, FSM bằng dữ liệu giả, LED | MPU bias/hướng trục/tilt, mailbox và sensor_manager | Contract test: NOT_READY/TIMEOUT/STALE không thành mẫu tốt |
| 6 · 25–31/10 | Nối tDecision/tSafety; fault latch và rearm | Nối tSensor/tLog/tBuzzer, nhiều tốc độ sampling | Ghép bench; chứng minh đường inhibit trước khi xe chạy |
| 7 · 01–07/11 | Xe né cơ bản tốc độ thấp, đo STOP/quãng dừng | Kiểm tra range/tilt/nhiễu motor, fault injection và log | **MVP gate**: đủ chức năng cơ bản, không lỗi STOP nghiêm trọng |
| 8 · 08–14/11 | Nếu MVP đạt: TURN có feedback góc tương đối | Hiệu chuẩn gyro, angle estimate ngắn, CSV/plot UART | So sánh góc mục tiêu/thực tế; có timeout và fallback STOP |
| 9 · 15–21/11 | Đo độ lặp lại lượt quay, tuning ngưỡng bằng dữ liệu | Đo drift/false alarm/overhead log, hoàn thiện đồ thị | **Go/no-go**: giữ cải thiện có bằng chứng; bỏ phần chưa ổn |
| 10 · 22–28/11 | Regression FSM/STOP/rearm, thử bế tắc và retry budget | Mất Echo/I2C lỗi, rung/tilt, kiểm tra status/stale | Chạy ma trận lỗi, không tự chạy lại ngoài ý muốn |
| 11 · 29/11–05/12 | Review stack/MSP/Flash, tải xấu nhất, release candidate | Soak và bảng số đo; hình schematic/ảnh lắp ráp | **Feature freeze 05/12**; chỉ sửa lỗi sau mốc này |
| 12 · 06–12/12 | Regression bản chốt, hướng dẫn build/nạp/tái lập | Video demo, biểu đồ kết quả, báo cáo giới hạn | Demo thử, cross-review tài liệu và repo sạch |
| Dự phòng · 13–20/12 | Xử lý lỗi còn lại, release/tag khi cả hai chấp thuận | Chạy lại test bị ảnh hưởng, chuẩn bị bàn giao | **Deadline 20/12**, không thêm hardware/tính năng |

## Điều chỉnh tải và bảo vệ deadline

Mỗi đầu tuần hai bạn ước lượng giờ cho task; cuối tuần ghi giờ thực tế, blocker và phần chuyển giao. Nếu một người vượt tải khoảng 20%, chuyển một việc độc lập (test harness, đo bench, documentation hoặc driver phụ) sang người kia, giữ owner module đang tích hợp để không gây conflict.

Nếu chưa đạt MVP ngày 07/11, dùng tuần 8–9 để ổn định xe; bỏ gyro-assisted turn hoặc biểu đồ nâng cao. Nếu đến 21/11 tính năng mở rộng chưa có kết quả ổn định, loại khỏi demo chính. Buffer dành cho phần cứng/thi cử/sửa lỗi, không coi là thời gian thêm tính năng.

## Bộ bàn giao hai người cùng chịu trách nhiệm

- Source build được và SHA/tag bản chốt; hướng dẫn tái lập.
- Schematic/pinout/BOM đã xác nhận, ảnh xe và các thay đổi phần cứng.
- Bảng ngưỡng đo được, log kiểm thử, kết quả STOP, sensor timeout, tilt và stack.
- Video demo thể hiện bình thường + một tình huống lỗi, không chỉ một lần chạy đẹp.
- Bảng đóng góp Nhân/Hưng theo PR và kết quả đo; giới hạn kỹ thuật trình bày trung thực.
