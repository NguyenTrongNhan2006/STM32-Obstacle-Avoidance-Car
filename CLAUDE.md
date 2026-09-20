# Hướng dẫn làm việc trong repo

Dự án STM32F103C8T6 + FreeRTOS của Nhân và Hưng, deadline 20/12/2026. Đọc [About/README.md](About/README.md), [FIRMWARE_GUIDE](About/FIRMWARE_GUIDE.md) và [timeline](About/TEAM_TIMELINE.md) trước khi phát triển.

Trạng thái hiện tại là skeleton build được: boot/clock/RTOS static, GPIO an toàn, UART cơ bản và mailbox/event group có mã nền. Thuật toán/driver cảm biến/PWM chưa triển khai. Không coi mọi file là rỗng, cũng không coi xe đã chạy.

## Ràng buộc

- Static allocation; không thêm malloc/free/pvPortMalloc hoặc heap_*.c.
- Không printf/sprintf/snprintf/vsnprintf, không HAL_Delay trong source dự án. UART có formatter giới hạn riêng, chỉ một writer.
- SysTick wrapper tăng HAL tick trước/sau scheduler; tick hook tắt. SVC/PendSV alias trực tiếp vào handler của port, không bọc C.
- Priority CMSIS dùng số 5..15 cho ISR gọi API RTOS FromISR; FreeRTOS raw priority dịch lên 4 bit.
- Core → App → Devices → MCAL → Config; không driver include App. Motor lấy safety predicate qua callback.
- robot_car là caller bình thường duy nhất của motor_apply. Emergency path chỉ kéo STBY thấp; phải chứng minh thời gian shutdown trước khi có motor thật.
- tSensor là producer duy nhất cho mailbox bản copy; consumer peek, không drain. Mẫu luôn có timestamp/status; IMU raw signed có kiểu riêng.
- Mỗi timer/bus có một owner. Nhân giữ Core/config/CMake; Hưng giữ sensor/I2C/UART/buzzer. Shared API thay đổi qua review chung.
- Không có IR trái/phải, Zephyr hoặc điều khiển từ xa trong scope ba tháng.
- [DO] là giả định cần đo; [CO DINH] là ràng buộc đã có nguồn. Không đổi giả định thành số đo trong tài liệu.
- Giữ TODO/IMPLEMENT cho phần chưa được yêu cầu triển khai; stub trả NOT_READY, không báo OK giả cho phép đo/actuation.

## Build và review

Trong Firmware: configure/build cả preset debug/release, chạy tools/check_constraints.sh với ELF tồn tại, xem size/map và warning của source dự án. Chi tiết ở About/IMPLEMENTATION_GUIDE.md. Khi thêm driver HAL I2C/TIM thật, cập nhật HAL config/source trong thay đổi có chủ đích; bản skeleton hiện giữ nguyên vendor/toolchain/linker/preset.

Yêu cầu của tác giả cho lần áp dụng skeleton đã cho phép tạo API/plumbing và thêm source vào CMake. Những công việc sau đó làm theo yêu cầu hiện hành, không suy ra cần xin lại quyền cho thao tác đã được giao. Thay đổi hành vi phải có kiểm thử phù hợp và cập nhật About cùng config.
