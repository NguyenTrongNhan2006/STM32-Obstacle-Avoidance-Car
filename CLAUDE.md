# Hướng dẫn làm việc trong repo

Dự án STM32F103C8T6 + FreeRTOS của Nhân và Hưng, deadline 20/12/2026. Đọc [About/README.md](About/README.md), [FIRMWARE_GUIDE](About/FIRMWARE_GUIDE.md) và [timeline](About/TEAM_TIMELINE.md) trước khi phát triển.

Trạng thái hiện tại: đã có đường sensor, motor, FSM, safety và 5 Task build được; host tests kiểm tra một phần hành vi. Chưa xác nhận xe chạy hoặc thời gian đáp ứng trên board. Xem `About/CURRENT_STATUS.md` trước khi thay đổi code hoặc tài liệu.

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
- Giữ TODO cho việc chưa được đo hoặc chưa chốt; không báo OK giả cho phép đo/actuation. TB6612 hiện đã có code nhưng linh kiện chưa được tác giả chốt.

## Build và review

Trong Firmware: build cả preset debug/release, chạy `tools/check_constraints.sh` với ELF và `tools/test_host.sh`, xem size/map và warning source dự án. HAL I2C/TIM/IWDG đã bật. Thêm kết quả kiểm thử phần cứng và cập nhật `About/CURRENT_STATUS.md` khi chốt một mốc.

Yêu cầu của tác giả cho lần áp dụng skeleton đã cho phép tạo API/plumbing và thêm source vào CMake. Những công việc sau đó làm theo yêu cầu hiện hành, không suy ra cần xin lại quyền cho thao tác đã được giao. Thay đổi hành vi phải có kiểm thử phù hợp và cập nhật About cùng config.
