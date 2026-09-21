# Hướng dẫn tự triển khai từng bước

[Về mục lục](README.md) · [Vai trò các file](FIRMWARE_GUIDE.md) · [Kế hoạch dự án](PROJECT_PLAN.md) · [Kiểm thử](TEST_PLAN.md)

Tài liệu này mô tả công việc để Nguyễn Trọng Nhân tự thiết kế và viết firmware. Toàn bộ file `.c/.h` hiện chỉ có `// TO DO`; không có API hoặc cấu hình đã triển khai. Các mốc dưới đây là kế hoạch, chưa có mốc nào được xác nhận chạy trên phần cứng.

## 1. Chốt phạm vi trước khi viết

| Hạng mục | Thông tin hiện có | Việc cần tự xác nhận |
| --- | --- | --- |
| Vi điều khiển | Dự án dự kiến STM32F103 | Mã chip, board, bộ nhớ và nguồn clock thực tế |
| MPU6050 | Tác giả đã nêu | Module cụ thể, hướng lắp và dùng để phát hiện nghiêng hay điều khiển từ xa |
| Cảm biến khoảng cách | Tác giả ghi “HR-04” | Kiểm tra nhãn; tên `ultrasonic_hcsr04` đang tạm theo HC-SR04 |
| Buzzer | Tác giả đã nêu | Loại active/passive, điện áp, dòng và mạch kích |
| Motor/cầu H | Chưa chốt | Mã motor, điện áp, dòng kẹt; TB6612FNG chỉ là phương án |
| IR trái/phải | Chưa xác nhận có | Có dùng hay bỏ qua ở bản đầu |
| Công cụ | Tự lựa chọn | HAL, LL hoặc thanh ghi; compiler, công cụ nạp/debug |

Nếu chỉ có cảm biến phía trước, bản đầu có thể dừng rồi thử quay theo một quy tắc cố định, có giới hạn thời gian/số lần và đo lại trước khi đi tiếp. Khi đó **chưa có thông tin xác nhận bên trái/phải trống**. Không dùng dữ liệu IR giả để làm như hai cảm biến đã tồn tại.

Đề xuất MPU6050 gắn trên xe để phát hiện nghiêng là một hướng nghiên cứu. Điều khiển bằng nghiêng tay là phạm vi khác: cần bộ điều khiển và đường truyền riêng. Chưa triển khai cả hai cùng lúc.

## 2. Chuẩn bị bản làm việc

Trong terminal VS Code, nếu chưa có repo:

```sh
git clone https://github.com/NguyenTrongNhan2006/STM32-Obstacle-Avoidance-Car.git
cd STM32-Obstacle-Avoidance-Car
code .
```

Nếu đã clone, chạy `git status` trước. Khi còn thay đổi chưa lưu vào Git, commit hoặc cất chúng trước khi cập nhật. Khi thư mục làm việc sạch:

```sh
git pull --ff-only
git switch -c feature/board-bringup
```

Tên branch là ví dụ cho giai đoạn khởi động board. Nếu `pull --ff-only` báo hai nhánh đã phân kỳ, kiểm tra lịch sử và xử lý merge/rebase có chủ đích; không dùng force push để chữa lỗi này.

VS Code chỉ là nơi soạn thảo. Repo hiện chưa có startup, linker script, toolchain hoàn chỉnh hay cấu hình debug; mở được thư mục không đồng nghĩa build được.

## 3. Lập bảng phần cứng và tài nguyên

Sao chép bảng này vào ghi chú thiết kế trong `About/`, rồi tự điền sau khi đối chiếu board và datasheet. “Chưa chốt” không phải giá trị để đưa vào code.

**MCU đã chốt:** STM32F103C8T6, dùng thư viện STM32 HAL. Nhóm hiện có 2 người: **Nhân** phụ trách `motor_driver` (PWM/GPIO động cơ); **Hưng** phụ trách `ultrasonic_hcsr04` và `obstacle_avoidance`. Giá trị dưới đây khớp với `Firmware/Config/board_config.h` — coi file đó là nguồn chốt cuối khi viết code, bảng này chỉ để tra cứu nhanh.

| Tín hiệu | Chân MCU | Ngoại vi/kênh | Mức điện áp/cực tính | Module sở hữu |
| --- | --- | --- | --- | --- |
| PWM motor trái/phải | PA0 (trái), PA1 (phải) | TIM2_CH1 / TIM2_CH2, PWM1, 1 kHz (PSC=71, ARR=999) | 3.3V logic; tần số/clock timer giả định 72 MHz, cần đối chiếu `SystemClock_Config()` thực tế | `motor_driver` |
| Hướng motor (IN1/IN2) | PB0/PB1 (trái), PB10/PB11 (phải) | GPIO output | 3.3V logic; **ánh xạ trái/phải là giả định, chưa đối chiếu sơ đồ đấu dây thật** | `motor_driver` |
| Trigger / Echo | PB8 (Trig), PA8 (Echo) | GPIO output / TIM1_CH1 input capture | Trig 3.3V logic; mức Echo của module thực tế chưa kiểm tra | `ultrasonic_hcsr04` |
| SDA / SCL | Chưa chốt | I2C | Pull-up và mức logic chưa chốt | `i2c` |
| IR trái/phải, nếu có | Chưa chốt | GPIO/EXTI | Chưa chốt | `ir_obstacle` |
| START/STOP | Chưa chốt | GPIO/EXTI | Chưa chốt | `button` |
| LED debug/trạng thái | PC13 | GPIO output | 3.3V logic | `status_led` |
| Buzzer | Chưa chốt | GPIO/PWM tùy linh kiện | Chưa chốt | `buzzer` |
| UART debug | PA9 (TX), PA10 (RX) | USART1, 115200 baud | 3.3V logic (TTL) | `uart_debug` |
| Nạp/debug | Theo board thực tế | SWD | Theo board thực tế | Công cụ nạp |

Ghi thêm nguồn clock, tần số bộ đếm và đơn vị thời gian cho mỗi timer. Tránh để driver tự cấu hình lại một timer đang phục vụ module khác. Nếu dùng chung, phải thiết kế rõ phần cấu hình chung và quyền thay đổi của từng module.

Với Echo, có thể cân nhắc timer input capture hoặc EXTI kết hợp đọc bộ đếm thời gian. EXTI chỉ báo cạnh, không tự cung cấp độ rộng xung. Input capture và PWM là các chế độ timer cần đối chiếu trong [RM0008 của ST](https://www.st.com/en/microcontrollers-microprocessors/stm32f103/documentation.html). Sườn hiện chưa có driver capture riêng; nếu chọn cách này, tự xác định interface đo xung trước khi mở rộng MCAL.

## 4. Tạo project tối thiểu có thể build/nạp

1. Xác nhận chính xác mã MCU rồi chọn device headers/CMSIS, startup và linker script tương ứng.
2. Chọn một hướng HAL, LL hoặc thanh ghi để bắt đầu. MCAL trong repo là lớp do bạn tự viết; không phải thư viện STM32 đã được cung cấp.
3. Nếu sinh project bằng công cụ của ST, tạo ở thư mục thử nghiệm riêng trước; kiểm tra cách ghép với sườn hiện tại. Tránh có hai `main.c`, hai vector table hoặc hai handler cùng tên trong danh sách build.
4. Hoàn thiện `Firmware/CMakeLists.txt` và các file build cần thiết: compiler cho MCU, danh sách nguồn, include paths, linker và đầu ra. File CMake hiện chỉ chứa `# TO DO`, nên chưa có lệnh build dùng ngay.
5. Cấu hình nạp/debug và kiểm tra breakpoint tại điểm vào chương trình. Giữ motor ở trạng thái dừng trong lúc kiểm tra board.
6. Ghi tên/phiên bản công cụ và các bước đã chạy thành công vào `About/`. Giữ thông báo bản quyền của thư viện bên thứ ba khi thêm vào repo.

**Hoàn thành mốc này khi:** tạo được đầu ra firmware, nạp được, debug được và xác nhận chương trình thực sự chạy trên board. Chưa cần đọc cảm biến hoặc chạy xe.

## 5. Viết từng lớp theo thứ tự

Với mỗi module: chốt đầu vào/đầu ra → viết interface trong `.h` → triển khai trong `.c` → thử riêng → ghi kết quả → mới ghép vào App. Không cần điền tất cả file cùng lúc.

| Mốc | File/module cần làm | Việc tự triển khai | Điều kiện chuyển bước |
| --- | --- | --- | --- |
| A. Nền tảng | `Core`, `gpio`, `timebase`, `uart_debug` | Khởi động, đọc/ghi chân, lịch thời gian và log | LED hoạt động; log có mốc thời gian; vòng lặp không bị chờ vô hạn |
| B. Điều khiển cơ bản | `button`, `status_led` | Chống dội; tách mức chân và sự kiện nhấn; báo trạng thái | Mỗi lần nhấn tạo đúng sự kiện mong muốn; STOP được ưu tiên |
| C. Motor | `pwm`, `motor_tb6612` hoặc loại thực tế | Ánh xạ bánh trái/phải, hướng, giới hạn lệnh, dừng | Thử từng bánh khi kê xe; reset/STOP đưa motor về trạng thái đã định nghĩa |
| D. Khoảng cách | `ultrasonic_hcsr04` | Trigger, nhận Echo, đo xung, timeout, đơn vị khoảng cách | So sánh với thước; mất Echo trả lỗi hữu hạn, không treo chương trình |
| E. IR nếu có | `exti`, `ir_obstacle` | Cực tính, sự kiện hai bên, lọc nhiễu | Xác định đúng trái/phải; ngắt không làm vòng lặp bị đói thời gian |
| F. MPU6050 | `i2c`, `mpu6050` | Kiểm tra kết nối, cấu hình chế độ đo, đọc dữ liệu, hiệu chuẩn | Dữ liệu có ý nghĩa khi đặt yên/nghiêng; ngắt kết nối được báo lỗi |
| G. Gom cảm biến | `sensor_manager` | Gói dữ liệu gồm giá trị, đơn vị, thời điểm và trạng thái hợp lệ | Nhận biết dữ liệu mới, quá hạn và lỗi; không coi thiếu cảm biến là đường trống |
| H. Điều kiện chạy | `safety_monitor` | Quyền cho phép chạy, STOP, lỗi và điều kiện phục hồi | Lệnh tiến không vượt qua được yêu cầu dừng |
| I. Né vật cản | `obstacle_avoidance`, `robot_car` | Trạng thái, quyết định hướng, timeout quay, số lần thử | Mỗi trạng thái có điều kiện thoát; mọi lỗi đưa xe về dừng theo thiết kế |
| J. Cảnh báo | `buzzer` | Mẫu còi theo trạng thái | Còi không giữ CPU trong một vòng chờ làm trễ STOP/cảm biến |

Trong `mpu6050`, đối chiếu thanh ghi nhận dạng, cấu hình thang đo và dữ liệu với [register map TDK InvenSense](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map.pdf). Không sao chép nguyên giá trị thanh ghi từ một module khác khi chưa hiểu ý nghĩa. Việc quyết định “nghiêng quá mức thì dừng” thuộc `safety_monitor`, không thuộc lớp I2C.

## 6. Chốt hợp đồng module bằng lời trước khi viết API

Với mỗi cặp `.c/.h`, tự trả lời các câu sau trong ghi chú thiết kế:

- Ai gọi module? Có được gọi từ ISR không, hay chỉ từ vòng lặp chính?
- Đầu vào dùng đơn vị gì và phạm vi nào? Dữ liệu ngoài phạm vi bị từ chối hay giới hạn?
- Lời gọi hoàn thành ngay, chờ có giới hạn, hay cần cập nhật nhiều lần?
- Kết quả phân biệt thành công, đang chờ, timeout và lỗi thế nào?
- Ai sở hữu timer, bus và dữ liệu dùng chung? Dữ liệu ISR/main trao đổi theo cơ chế nào?
- Khởi tạo thất bại thì ứng dụng có được chạy motor không? Cần thao tác gì để phục hồi?

Ví dụ thiết kế bằng lời: một mẫu khoảng cách cần có **giá trị + đơn vị + thời điểm lấy mẫu + trạng thái**. Mất Echo không nên biến thành “0 mm” hoặc một khoảng cách rất xa rồi được dùng như phép đo hợp lệ. Tên kiểu dữ liệu và hàm cụ thể do bạn tự thiết kế.

ISR nên chỉ ghi nhận sự kiện/chụp thời gian và xử lý cờ theo ngoại vi. Tránh chạy thuật toán né, in log dài hoặc chờ I2C trong ISR. Khi chia sẻ một nhóm dữ liệu giữa ISR và main, cần bảo đảm main không đọc trúng bản đang cập nhật dở; chỉ thêm `volatile` chưa giải quyết tính nhất quán của cả nhóm dữ liệu.

## 7. Ghép hành vi xe

Đề xuất ưu tiên: **STOP/lỗi bắt buộc dừng → dữ liệu cảm biến hợp lệ → quyết định né → lệnh motor**. Chọn một nơi cuối cùng áp dụng lệnh motor, ví dụ `robot_car`, để tránh hai module ra lệnh trái ngược.

| Trạng thái đề xuất | Công việc | Điều kiện thoát |
| --- | --- | --- |
| `IDLE` | Motor dừng; chờ START | START và các điều kiện chạy đều đạt |
| `FORWARD` | Đi theo tốc độ đã thử nghiệm | Vật cản → `STOP`; nút STOP → `IDLE`; lỗi → `FAULT` |
| `STOP` | Dừng do gặp vật cản; chọn phương án né | Có phương án hợp lệ → trạng thái quay; hết khả năng thử → `FAULT` |
| `TURN_LEFT/RIGHT` | Quay theo giới hạn đã chốt | Hết thời gian quay → `CHECK`; STOP/lỗi được xử lý ngay theo ưu tiên |
| `CHECK` | Giữ dừng, đợi phép đo mới | Đường thoáng → `FORWARD`; còn vật cản → thử lại có giới hạn; dữ liệu lỗi/quá hạn → `FAULT` |
| `FAULT` | Dừng và báo lý do | Lỗi đã hết và tác giả quy định thao tác xác nhận → `IDLE` |

Nút STOP ở đây là dừng theo yêu cầu người dùng, không đồng nghĩa linh kiện bị lỗi. Chỉ có một cảm biến trước thì hướng quay là phương án thử, không phải hướng đã được đo là an toàn. Không suy ra góc quay chính xác từ một khoảng thời gian quay khi chưa có phản hồi phù hợp.

Tự chọn ngưỡng và thời gian bằng thử nghiệm: khoảng cách dừng phải xét tốc độ, thời gian lấy mẫu/xử lý và quãng đường xe còn trôi. Hysteresis là dùng điều kiện vào/ra khác nhau để tránh trạng thái bật tắt liên tục gần một ngưỡng. Giá trị cụ thể sau này đặt trong `app_config.h`, kèm lý do đo được trong tài liệu.

## 8. Kết thúc mỗi mốc

Chạy các ca liên quan trong [TEST_PLAN.md](TEST_PLAN.md), lưu log/ảnh và cập nhật trạng thái thực tế. Sau đó kiểm tra thay đổi trước khi commit:

```sh
git status
git diff
git add Firmware About Images
git diff --cached
git commit -m "Bring up GPIO and document hardware checks"
git push -u origin feature/board-bringup
```

Lệnh trên áp dụng cho branch ví dụ đã tạo ở bước 2; đổi tên branch và commit theo công việc thực tế. Các file toolchain hoặc cấu hình mới nằm ngoài ba thư mục trên cần được thêm có chủ đích. Không commit file build tạm hoặc thông tin đăng nhập. Có thể tạo Pull Request vào `main` để xem lại từng mốc trước khi hợp nhất.
