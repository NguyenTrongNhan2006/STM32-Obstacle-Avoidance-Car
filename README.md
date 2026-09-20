# STM32 Obstacle Avoidance Car

Xe tự tránh vật cản dùng **STM32F103C8T6 + FreeRTOS**, do **Nhân và Hưng** cùng triển khai trong 3 tháng: **20/09–20/12/2026**.

**Trạng thái: skeleton build được, chưa phải firmware điều khiển xe hoàn chỉnh.** Có boot, clock, kernel static allocation, UART cơ bản và API/stub. Năm Task chỉ chờ; thuật toán, PWM, I2C và đọc cảm biến vẫn để `TO DO / IMPLEMENT`. Motor luôn bị vô hiệu hóa trong skeleton.

```text
Firmware/   Core, Config, MCAL, Devices, App, CMake và thư viện nền
Images/     Sơ đồ nguyên lý, đấu nối, ảnh lắp ráp và minh họa kiểm thử
About/      Hướng dẫn, quyết định thiết kế, phân công và bằng chứng kiểm thử
```

Bắt đầu tại [mục lục About](About/README.md), [vai trò từng file](About/FIRMWARE_GUIDE.md) và [timeline Nhân–Hưng](About/TEAM_TIMELINE.md). [Đánh giá ý tưởng skeleton](About/DESIGN_REVIEW.md) giải thích những điểm đã điều chỉnh trước khi áp dụng.

Phần cứng hiện dự kiến: một cảm biến khoảng cách phía trước (tạm gọi HC-SR04; cần xác nhận nhãn “HR-04”), MPU6050 trên xe, buzzer, nút START/STOP và hai motor qua TB6612FNG **chưa chốt theo dòng motor**. Không có cảm biến IR trái/phải trong phiên bản này.

Copyright (c) 2026 Nguyễn Trọng Nhân. All rights reserved. Xem [LICENSE](LICENSE); thư viện bên thứ ba giữ giấy phép riêng.
