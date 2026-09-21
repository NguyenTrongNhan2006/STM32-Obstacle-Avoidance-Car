# Đề xuất cho xe né vật cản

[Về mục lục](README.md) · [Hướng dẫn triển khai chi tiết](IMPLEMENTATION_GUIDE.md) · [Kiểm thử](TEST_PLAN.md)

Đây là phương án để tác giả cân nhắc, chưa chốt BOM, pinout hoặc thông số vận hành. Sườn driver trong `Firmware/` chỉ có `// TO DO`; chưa có firmware hoạt động. Tất cả thiết kế dưới đây là đề xuất để thảo luận, chưa phải quyết định triển khai.

Tác giả đã nêu MPU6050, cảm biến "HR-04" và buzzer. Tạm hiểu "HR-04" là HC-SR04 để đặt tên sườn driver; cần kiểm tra chữ in trên module trước khi viết driver. TB6612FNG và hai cảm biến IR vẫn là đề xuất bổ sung, chưa được xác nhận.

## Bản đầu nên làm

Xe hai động cơ tự phát hiện vật cản phía trước, dừng và xoay để tìm hướng đi. Thêm hai cảm biến IR để nhận biết vật cản trái/phải. MPU6050 gắn trên xe để nghiên cứu phát hiện nghiêng/lật và yêu cầu dừng.

Cách này kết hợp **PWM + MPU6050** của chủ đề xe robot với **đa cảm biến + EXTI + cảnh báo** của chủ đề báo động xâm nhập. MPU6050 đo chuyển động/nghiêng, không đo khoảng cách vật cản.

## Phần cứng đề xuất

| Thành phần | Phương án ban đầu | Ghi chú khi chốt |
| --- | --- | --- |
| MCU | STM32F103C8T6 / Blue Pill — **đã chốt**, dùng thư viện STM32 HAL | Kiểm tra board thực tế; pinout đã chốt trong `Firmware/Config/board_config.h` và [IMPLEMENTATION_GUIDE.md §3](IMPLEMENTATION_GUIDE.md#3-lập-bảng-phần-cứng-và-tài-nguyên) |
| Motor driver | TB6612FNG + hai động cơ DC giảm tốc | Chọn theo điện áp và dòng kẹt motor, khả năng tản nhiệt của module |
| Khoảng cách trước | HC-SR04 | Kiểm tra thông số module mua thực tế; xử lý mức điện áp Echo phù hợp GPIO STM32 |
| Vật cản hai bên | Hai module IR digital | Xác nhận mức điện áp, cực tính output; thử với nhiều bề mặt và ánh sáng |
| Chuyển động/nghiêng | MPU6050 qua I2C | Gắn cố định trên xe; hiệu chuẩn và lọc trước khi đặt ngưỡng |
| Điều khiển/trạng thái | Nút START/STOP + LED | Xe khởi động ở trạng thái dừng |
| Cảnh báo | Buzzer tùy chọn | Chọn mạch điều khiển theo dòng/điện áp buzzer |
| Nguồn và nạp | Nguồn phù hợp motor/logic + ST-Link | Chung mass; không cấp motor trực tiếp từ chân GPIO |

STM32F103C8 có timer/PWM, I2C, USART và ngắt ngoài phù hợp cách chia module này. Xem [thông tin và tài liệu chính thức ST](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html).

TB6612FNG là phương án driver motor cần đối chiếu thông số với motor thực tế. Xem [trang sản phẩm và datasheet Toshiba](https://toshiba.semicon-storage.com/ap-en/semiconductor/product/motor-driver-ics/brushed-dc-motor-driver-ics/detail.TB6612FNG.html).

MPU6050: dùng [register map của TDK InvenSense](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf) khi viết driver. Chưa chốt chân hoặc giá trị thanh ghi trong repo.

## Đề xuất hành vi

- `IDLE`: chờ nhấn START; motor tắt.
- `FORWARD`: tiến khi dữ liệu cảm biến hợp lệ và đường phía trước thoáng.
- `STOP`: dừng khi phát hiện vật cản.
- `TURN_LEFT` / `TURN_RIGHT`: chọn hướng theo dữ liệu hai bên, thử quay trong khoảng thời gian giới hạn.
- `CHECK`: đọc lại khoảng cách sau khi quay rồi mới tiến.
- `FAULT`: dừng/cảnh báo khi lỗi cảm biến, quá số lần né hoặc nghiêng quá mức; lỗi phải hết và có thao tác xác nhận trước khi trở về `IDLE`.

Nút STOP được ưu tiên ở mọi trạng thái và đưa xe về `IDLE`; nút STOP không tự nó là lỗi hệ thống. Bảng chuyển trạng thái cụ thể nằm trong [hướng dẫn triển khai, mục 7](IMPLEMENTATION_GUIDE.md#7-ghép-hành-vi-xe). Nếu còn lỗi đang hiện diện, giữ yêu cầu dừng và không cho START vượt qua lỗi.

Đây là các trạng thái đề xuất, chưa có mã triển khai. Không cho lùi tự động ở bản đầu khi chưa có cảm biến phía sau. Cần thử nghiệm phạm vi quét của thân xe khi quay; cảm biến phía trước không bảo đảm hai bên hoặc phía sau trống.

## Thứ tự triển khai đơn giản

1. GPIO + LED + nút START/STOP + timebase; log bằng UART.
2. PWM + driver motor: thử riêng từng bánh ở tốc độ thấp.
3. Cảm biến khoảng cách trước: kiểm tra giá trị, timeout và tình huống mất Echo.
4. IR hai bên + EXTI; kiểm tra chống nhiễu và sự kiện ngắn.
5. State machine né vật cản bằng vòng lặp định kỳ, chưa cần RTOS.
6. MPU6050 + hiệu chuẩn/lọc + điều kiện dừng do nghiêng.
7. Buzzer, ảnh sơ đồ nối dây và kết quả thực nghiệm.

## Để giai đoạn sau

- Servo quét cảm biến khoảng cách: chỉ thêm khi cách né cố định chưa đủ.
- Encoder/PID: chỉ thêm khi cần kiểm soát tốc độ hoặc góc quay chính xác hơn.
- Điều khiển bằng nghiêng tay: cần MPU6050 ở bộ điều khiển riêng và đường truyền có timeout mất kết nối; không phải chỉ gắn MPU6050 trên xe.

## Cần chốt khi bắt đầu viết code

Board đang có, motor/driver, cảm biến và nguồn; vai trò MPU6050. MCU (STM32F103C8T6) và lựa chọn thư viện (STM32 HAL) đã chốt; pinout động cơ và HC-SR04 đã chốt trong `board_config.h`, các tín hiệu còn lại (I2C, IR, START/STOP, buzzer) vẫn "Chưa chốt". Sau đó mới định nghĩa API còn lại và hoàn thiện build.
