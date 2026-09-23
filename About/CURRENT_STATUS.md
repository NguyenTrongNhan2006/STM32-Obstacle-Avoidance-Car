# Trạng thái triển khai và việc tiếp theo — 23/09/2026

**Nhân + Hưng · 20/09–20/12/2026.** Tài liệu này ghi trạng thái code hiện tại; [timeline](TEAM_TIMELINE.md) vẫn là kế hoạch theo tuần. Số liệu từ build và kiểm thử trên máy chưa thay cho đo trên board.

## Đã có trong code

- 5 task FreeRTOS tĩnh: Safety 10 ms, Sensor 10 ms, Decision 20 ms, Log/Buzzer 100 ms.
- HC-SR04 qua TIM2 PWM Input; MPU6050 qua I2C1 100 kHz; TIM3 PWM và driver TB6612; FSM tránh vật cản với quay dựa trên gyro Z.
- Safety chốt STOP, lỗi cảm biến, nghiêng và lỗi heartbeat. Khi có inhibit, `tSafety` hạ STBY trực tiếp. Chỉ refresh IWDG nếu Sensor và Decision cùng check-in; lỗi heartbeat đòi hỏi nhấn rearm khi cả hai task đã khỏe lại.
- MPU6050 hiệu chuẩn gyro từng mẫu mới theo `INT_STATUS.DATA_RDY_INT`; không publish `SAMPLE_OK` trước khi đủ 64 mẫu. Mất bus hoặc khởi tạo lại đều cần hiệu chuẩn lại. Nếu phát hiện chuyển động, hủy cửa sổ mẫu và đợi xe đứng yên. Vì vậy IMU có thể cần ít nhất khoảng 0,64 s để sẵn sàng sau boot/recovery.
- Buzzer giữ sự kiện vật cản ngắn đến khi task bắt đầu phát trọn mẫu; Fault/Silent có thể ngắt mẫu. UART in yaw có dấu.
- CI build Release, kiểm tra constraints và chạy host tests. Chạy `bash Firmware/tools/test_host.sh` từ root trên Git Bash/Linux; script cần C compiler cho máy.

## Chưa được chứng minh

| Hạng mục | Trạng thái / bằng chứng cần có |
| --- | --- |
| Cầu H, motor, nguồn | TB6612 là phương án đã viết code, **chưa chốt linh kiện**. Trước khi đấu motor, xác nhận bridge, dòng motor (kể cả dòng kẹt), nguồn, cực tính và STBY pull-down ngoài. |
| HC-SR04 Echo | PA0 cần hạ mức 5 V xuống 3,3 V. Đo xung Trigger/Echo, timeout và khoảng cách với vật ở nhiều bề mặt. Main chưa có lọc median. |
| Hiệu chuẩn IMU | Xác nhận module/AD0, data-ready, thời gian ổn định sau wake, hướng trục và bias qua nhiều lần khởi động. Host test chỉ mô phỏng thanh ghi, chưa chứng minh cảm biến thật. |
| STOP/heartbeat | Đo từ khi nút/lỗi cảm biến/mất check-in đến lúc STBY thấp. Thử Decision treo trong khi PWM đang chạy trên giá đỡ, rồi thử rearm sau khi phục hồi. |
| Quay 90° | Đo góc thực và drift; timeout hiện chuyển FSM sang CHECK, chưa phân loại là lỗi quay không đạt góc. |
| RAM/RTOS | Linker báo RAM tĩnh, chưa đo stack high-water mark, MSP hay độ trễ task khi I2C/UART timeout. |

## Phân công gần nhất

| Mốc | Nhân | Hưng | Cổng chung |
| --- | --- | --- | --- |
| Đến 26/09 | Review safety latch/cắt STBY, host test treo Decision; kiểm tra cầu H và sơ đồ nguồn | Review hiệu chuẩn IMU 64 mẫu, data-ready; kiểm tra module MPU/HC-SR04 thực có | Chốt danh sách linh kiện và kịch bản thử trên giá đỡ; CI xanh |
| 27/09–03/10 | Bring-up clock/SWD, đo GPIO/PWM và reset/STBY trên board; đo độ trễ STOP | Đo I2C, Echo, telemetry số có dấu và pattern buzzer; ghi log khi lỗi bus/cảm biến | Có số đo, không chỉ bản build; cập nhật pinout, BOM và ngưỡng `[DO]` |
| 04–10/10 | Kiểm tra 5 task, stack và đường fault/rearm | Kiểm tra range/tilt/gyro khi motor gây nhiễu | Review chéo trước khi thử xe chạy |

Giữ mốc **MVP 07/11**, **feature freeze 05/12** và deadline **20/12**. Nếu board hoặc cầu H chưa chốt, giữ motor ở trạng thái tắt; phần code đã có không xác nhận pinout đúng với linh kiện sẽ mua. Hai người ghi lại kết quả đo kèm ngày, phần cứng và commit SHA trước khi đánh dấu hoàn thành.
