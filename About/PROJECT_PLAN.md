# Phạm vi và phần cứng

## Mục tiêu phiên bản 1

Xe tốc độ thấp tự chạy trong khu thử có kiểm soát, phát hiện vật cản phía trước, dừng, thử quay và đo lại. MPU6050 gắn trên xe để nghiên cứu phát hiện nghiêng; buzzer/LED biểu thị trạng thái. START cần thao tác chủ động; STOP và lỗi phải có ưu tiên cao hơn lệnh chạy.

**Không có cảm biến trái/phải.** Khi quay, xe chưa biết bên đó có trống hay không; phải hạn chế tốc độ, thời gian/số lần thử và kiểm tra lại. Không tuyên bố tránh được mọi vật cản, mép bàn, bậc thang hoặc vật hấp thụ âm.

## BOM và các quyết định còn phải xác nhận

| Thành phần | Phương án | Người chốt và bằng chứng |
| --- | --- | --- |
| MCU | STM32F103C8T6 Blue Pill, HSE giả định 8 MHz | Nhân: đọc mã chip, crystal, schematic, thử SWD |
| Khoảng cách trước | Tạm HC-SR04 từ mô tả “HR-04” | Hưng: ảnh nhãn, điện áp và khoảng đo hợp lệ |
| IMU | MPU6050 gắn cứng trên xe | Hưng: mã module, nguồn/pull-up I2C, hướng trục và AD0 |
| Cầu H | **Chọn TB6612FNG hai kênh cho bản đầu**; chưa xác nhận module đã mua hoặc phù hợp motor thực tế | Nhân: xác nhận module, điện áp và dòng motor; Hưng: review sơ đồ đấu nối và thử STOP |
| Motor, bánh, chassis | Hai motor DC, chưa chốt thông số | Nhân đo tốc độ/dòng; Hưng hỗ trợ đo và ghi kết quả |
| Buzzer | Đề xuất active qua transistor | Hưng xác nhận active/passive, dòng/điện áp; thêm bảo vệ phù hợp tải |
| Nguồn | Nguồn motor và ổn áp logic phù hợp | Cả hai: nối chung GND, bố trí decoupling, kiểm tra sụt áp/nhiễu |
| Nút, LED, debug | START/STOP, LED board, USB-UART 3.3 V, ST-Link | Nhân bring-up; Hưng kiểm thử |

Đây là đề xuất đấu nối, chưa phải sơ đồ sản xuất. Đưa sơ đồ đã kiểm tra vào `Images/` và ghi revision/date. Không nối motor công suất trực tiếp với GPIO hay nguồn USB của board.

### Quyết định cầu H và điều kiện lắp

Chọn module TB6612FNG có hai kênh và chân STBY để khớp driver/pinout hiện tại, giảm việc đổi kiến trúc trong deadline ba tháng. Theo [datasheet Toshiba](https://toshiba.semicon-storage.com/info/datasheet_en_20141001.pdf?did=10660), VCC logic làm việc trong 2,7–5,5 V (dùng 3,3 V), VM trong 2,5–13,5 V. Dòng 1,2 A/kênh là **giới hạn tuyệt đối**; 3,2 A chỉ là xung đơn tối đa 10 ms, không phải khả năng chạy/kẹt liên tục. Datasheet nêu dòng làm việc tối đa 1,0 A/kênh khi VM ≥ 4,5 V (hoặc 0,4 A khi VM thấp hơn, không PWM). Khả năng nhiệt còn tùy module, hai motor chạy cùng lúc và điều kiện lắp thực tế.

Trước khi nối motor: Nhân ghi mã motor, điện áp danh định, dòng không tải, dòng dưới tải và dòng kẹt; ghi điện áp pin khi đầy và khi tải. Nếu dữ liệu không chứng minh được motor nằm trong giới hạn dòng/nhiệt của TB6612FNG, **đổi cầu H và cập nhật driver/pinout trước khi chạy**, không dựa vào mức 3,2 A như dòng liên tục. Hưng kiểm tra sơ đồ nguồn chung GND, tụ gần module theo datasheet, STBY pull-down ngoài 10 kΩ, cực tính và phép thử STOP trên giá đỡ. Đây là lựa chọn thiết kế, chưa xác nhận tính tương thích với một motor hoặc pack pin cụ thể.

Hai động cơ DC trong bản đầu **không đọc encoder**: PWM là điều khiển tốc độ hở vòng, còn quay theo góc lấy phản hồi gyro Z từ MPU6050. Nếu mua motor có encoder, để ngõ encoder chưa nối tới STM32 trong MVP và ghi rõ loại tín hiệu/điện áp cho phiên bản sau; firmware hiện chưa có chân, timer hay driver quadrature dành cho encoder.

## Pinout dự kiến — đồng bộ board_config.h

| Tín hiệu | Chân | Ngoại vi/owner | Lưu ý |
| --- | --- | --- | --- |
| Motor trái/phải PWM | PA6 / PA7 | TIM3 CH1/CH2, `pwm` | Đề xuất 20 kHz, duty ban đầu 0 |
| AIN1 / AIN2 | PB0 / PB1 | `motor_tb6612` qua GPIO | Xác nhận chiều từng bánh |
| BIN1 / BIN2 | PB10 / PB11 | `motor_tb6612` qua GPIO | Không dùng I2C2 cùng lúc |
| STBY | PB5 | Motor; đường khẩn cấp chỉ kéo thấp | Pull-down ngoài 10 kΩ; reset vẫn phải tắt cầu H |
| TRIG / ECHO | PA1 / PA0 | GPIO / TIM2 CH1, `timebase` | Echo giảm mức ngoài xuống 3.3 V; không đưa 5 V trực tiếp vào PA0 |
| SCL / SDA | PB6 / PB7 | I2C1, `i2c` | Pull-up logic 3.3 V; kiểm tra module cụ thể |
| START/STOP | PA8 | EXTI8, `exti` | Active-low pull-up; debounce ở Task |
| LED | PC13 | `status_led` | Đề xuất active-low, kiểm tra board |
| Buzzer | PB12 | `buzzer` | Skeleton giữ thấp; xác nhận mạch kích |
| UART TX / RX | PA9 / PA10 | USART1, `uart_debug` | 115200, 8N1, mức 3.3 V |
| SWD | PA13 / PA14 | Debug | Giữ nguyên |

SysTick dành cho kernel và HAL tick wrapper; TIM4 còn trống. Mỗi timer/bus có một module sở hữu; task không tự reconfigure ngoại vi. I2C/TIM chưa được bật trong HAL config và source vendor hiện tại: khi triển khai thật, thêm hỗ trợ trong PR riêng, không dùng header stub như bằng chứng ngoại vi đã hoạt động.

## Tính năng và điều kiện nhận

| Mức | Nội dung | Điều kiện |
| --- | --- | --- |
| Bắt buộc | Boot RTOS, STOP/START, hai motor, khoảng cách, FSM né cơ bản, báo lỗi | Mỗi module có đo kiểm; lỗi cảm biến dẫn đến trạng thái dừng |
| Bắt buộc | Phát hiện nghiêng với MPU6050, mailbox có status/timestamp | Chốt hướng trục, hiệu chuẩn, đo false alarm khi rung |
| Nâng cao 1 | Quay ngắn dùng gyro hỗ trợ | Chỉ làm sau mốc xe cơ bản 07/11; kiểm tra drift và timeout |
| Nâng cao 2 | UART telemetry và biểu đồ kết quả | Log không làm trễ STOP; ưu tiên CSV/plot đơn giản trên PC |
| Sau deadline | Zephyr, wireless, app, encoder, servo, bản đồ | Không đưa vào tiêu chí nghiệm thu 20/12 |

Ngưỡng 250/350 mm, góc 30°, timeout và chu kỳ trong config đều là `[DO]`. Cần đo tốc độ, độ trễ sensor/scheduler/actuation và quãng đường dừng trước khi dùng chúng làm ngưỡng thật. Chọn khoảng dừng theo `v_max × tổng độ trễ + quãng đường dừng đo được + biên dự phòng`; vùng clear cần hysteresis đủ lớn.

Không tự động lùi ở bản đầu: phía sau không có cảm biến. Enum BACKWARD chỉ là chỗ giữ API cho thử motor trên giá đỡ hoặc mở rộng sau này.
