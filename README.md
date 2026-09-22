# STM32 Obstacle Avoidance Car

Xe tự tránh vật cản dùng **STM32F103C8T6 + FreeRTOS**, do **Nhân và Hưng** cùng triển khai trong 3 tháng: **20/09–20/12/2026**.

**Trạng thái: skeleton build được, chưa phải firmware điều khiển xe hoàn chỉnh.** Có boot, clock, kernel static allocation, UART cơ bản và API/stub. Năm Task chỉ chờ; thuật toán, PWM, I2C và đọc cảm biến vẫn để `TO DO / IMPLEMENT`. Motor luôn bị vô hiệu hóa trong skeleton.

```text
Firmware/   Core, Config, MCAL, Devices, App, CMake và thư viện nền
Images/     Sơ đồ nguyên lý, đấu nối, ảnh lắp ráp và minh họa kiểm thử
About/      Hướng dẫn, quyết định thiết kế, phân công và bằng chứng kiểm thử
```

Bắt đầu tại [mục lục About](About/README.md), [vai trò từng file](About/FIRMWARE_GUIDE.md) và [timeline Nhân–Hưng](About/TEAM_TIMELINE.md). [Đánh giá ý tưởng skeleton](About/DESIGN_REVIEW.md) giải thích những điểm đã điều chỉnh trước khi áp dụng.

## Sơ đồ hệ thống

Sơ đồ Mermaid dựng từ mã nguồn hiện tại, xem trực tiếp trên GitHub. Đây là bản mô tả **thiết kế đang có trong repo**, không phải hành vi đã đo trên xe; hồ sơ sơ đồ vẽ tay và ảnh kiểm thử vẫn nằm ở [About/SYSTEM_DIAGRAMS.md](About/SYSTEM_DIAGRAMS.md).

| Sơ đồ | Nội dung |
| --- | --- |
| [Sơ đồ khối phần cứng](docs/diagrams/hardware_block_diagram.md) | Kết nối linh kiện, bảng chân kèm chế độ GPIO, cây nguồn, quyền sở hữu timer/bus, bảng NVIC priority, cảnh báo phần cứng |
| [Kiến trúc phần mềm](docs/diagrams/software_architecture.md) | Năm tầng Core → App → Devices → MCAL → Config, Dependency Rule, cách `motor_apply` nhận safety predicate qua callback |
| [FSM điều khiển](docs/diagrams/fsm_control_flow.md) | Bảy state của `obstacle_avoidance`, bảng guard, cách chọn hướng quay khi không có cảm biến hai bên, xử lý mẫu quá hạn |
| [FreeRTOS concurrency](docs/diagrams/freertos_concurrency.md) | Năm task, SysTick wrapper dùng chung HAL/kernel, mailbox bản copy, `egSafety` chốt fault và rearm, ngân sách RAM |

Phần cứng hiện dự kiến: một cảm biến khoảng cách phía trước (tạm gọi HC-SR04; cần xác nhận nhãn “HR-04”), MPU6050 trên xe, buzzer, nút START/STOP và hai motor qua TB6612FNG **chưa chốt theo dòng motor**. Không có cảm biến IR trái/phải trong phiên bản này.

Copyright (c) 2026 Nguyễn Trọng Nhân. All rights reserved. Xem [LICENSE](LICENSE); thư viện bên thứ ba giữ giấy phép riêng.
