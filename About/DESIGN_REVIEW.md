# Đánh giá và áp dụng ý tưởng skeleton

## Kết luận

Ý tưởng **phù hợp cho nhóm hai người trong ba tháng**: API có kiểu dữ liệu, ownership rõ và build được giúp Nhân, Hưng làm riêng rồi tích hợp sớm. Chỉ giữ file rỗng sẽ không phát hiện được trùng enum, thiếu source trong CMake hay lỗi liên kết RTOS.

Đã áp dụng phần nền cần thiết; chưa viết thuật toán né vật cản, ước lượng góc, debounce, PWM, I2C hoặc driver đo khoảng cách. Hai người vẫn tự cấu hình và phát triển các phần này. Các Task hiện chỉ gọi delay 100 ms; đây không phải chu kỳ điều khiển cuối cùng.

## Những điểm phải điều chỉnh

| Ý tưởng ban đầu | Cách áp dụng và lý do |
| --- | --- |
| Priority kernel = 15, syscall = 5 trực tiếp | Giữ số CMSIS 15/5, nhưng macro ghi thanh ghi FreeRTOS phải là `15 << 4` và `5 << 4`. F103 có 4 bit priority. |
| C wrapper gọi SVC/PendSV của port | Alias tên handler trực tiếp trong `FreeRTOSConfig.h`; các hàm của port dùng naked assembly, không được bọc bằng lời gọi C thông thường. |
| Tick hook tắt nhưng HAL tick lại dùng hook | `SysTick_Handler` tăng HAL tick đúng một lần; chỉ gọi kernel tick sau khi scheduler đã khởi động. Tick hook vẫn tắt. Giữ tick 1 kHz. |
| Một mẫu unsigned dùng cho mọi cảm biến | Khoảng cách dùng `sample_t`; IMU dùng `imu_sample_t` với gia tốc/gyro signed raw, timestamp và status. |
| Enum cùng có STOP/FORWARD ở nhiều header | Prefix `MOTOR_*`, `CAR_*`, `STATUS_*` để tránh xung đột tên C. |
| Driver motor trực tiếp include App event group | Inject callback `is_safe` vào driver. App giữ event group; Devices không include App. Callback đọc snapshot an toàn, không đồng nghĩa có cơ chế ngắt motor độc lập đã hoàn thiện. |
| IR trái/phải | Bỏ hai file driver và bỏ IR khỏi pinout/timeline theo phạm vi đã thống nhất. |
| Echo phải FT, kéo theo đổi I2C | Đề xuất PA0/TIM2CH1 với mạch giảm mức Echo xuống 3.3 V. Giữ I2C1 PB6/PB7; không giả định PA0 chịu 5 V. |
| Giữ CMake hoàn toàn | Nhân đã đồng ý ngoại lệ: chỉ thêm danh sách source skeleton. Không đổi toolchain, linker, preset, HAL config hoặc thư viện. |
| Hơn 40 file nhưng dễ trùng typedef | Thêm `Config/project_types.h` làm nơi duy nhất chứa kiểu dùng chung. |

Quy tắc priority dựa trên [hướng dẫn chính thức FreeRTOS](https://www.freertos.org/FreeRTOS_Support_Forum_Archive/April_2017/freertos_configMAX_SYSCALL_INTERRUPT_PRIORITY_4845a599j.html). Giới hạn 72 MHz, Flash 64 KB và SRAM 20 KB đối chiếu [STM32F103C8 của ST](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8); chi tiết chân cần đối chiếu [datasheet ST](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf). Tên handler được kiểm tra trực tiếp trong port GCC/ARM_CM3 có sẵn trong repo.

## Đánh đổi có chủ đích

- Giữ 5 Task theo ý tưởng để học cách phân trách nhiệm. Buzzer và log ở P0 phải block/yield; nếu đo thấy lãng phí stack, có thể gộp sau bằng một thay đổi được cả nhóm review.
- UART polling chỉ là công cụ debug ban đầu, có timeout và một writer. Chưa phải telemetry hiệu năng cao. Nếu nó chiếm CPU quá lâu, giảm lưu lượng trước khi cân nhắc DMA.
- `motor_apply` hiện luôn kéo STBY thấp và trả NOT_READY. Callback an toàn không giải quyết được mọi race/stall khi motor đã chạy; phải chốt đường ngắt motor khẩn cấp và đo latency trước mốc tích hợp.
- Bật timer service theo đề xuất dù chưa có software timer của ứng dụng. Chi phí bộ nhớ đã nằm trong ELF.
- Compile toàn bộ source không có nghĩa toàn bộ stub xuất hiện trong ELF: linker có thể bỏ hàm chưa được gọi.

## Điểm “wow” vừa sức

Sau khi xe cơ bản đạt kiểm thử: **quay tương đối có hỗ trợ gyro** + **log UART minh họa state, khoảng cách, góc quay và fault**. Hai tính năng này có số liệu để trình bày, tận dụng linh kiện hiện có, không mở thêm một hệ thống điều khiển từ xa. MPU6050 sáu trục không cung cấp hướng tuyệt đối chống drift; chỉ đánh giá góc tương đối trong lượt quay ngắn.

FreeRTOS là nền duy nhất trong deadline này. Zephyr, SLAM, app điện thoại, encoder, servo quét và điều khiển nghiêng tay để sau phiên bản 1.
