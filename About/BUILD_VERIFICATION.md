# Biên bản kiểm tra skeleton — 20/09/2026

Baseline trước thay đổi: `cc834235d4426a6522ec1df64ea6d888cffde292` trên main. Kết quả dưới đây áp dụng cho source skeleton trong đợt cập nhật này, không phải kết quả chạy trên xe.

## Môi trường và phạm vi

- Windows, Arm GNU Toolchain 14.2.Rel1 / GCC 14.2.1, CMake 4.3.4, Ninja.
- Debug dùng -Og, Release dùng -Os theo CMake có sẵn.
- Đã kiểm tra 18/18 file .c của dự án có trong compile_commands.json.
- Không warning từ mã dự án với -Wall -Wextra -Wpedantic -Wshadow -Wdouble-promotion; vendor vẫn dùng -w như baseline.
- Không sửa CMakePresets.json, cmake/, linker/, ThirdParty/, stm32f1xx_hal_conf.h hoặc tools/check_constraints.sh.
- CMakeLists.txt chỉ thêm 17 dòng source, không đổi flags/toolchain/library/linker.
- Hai file IR placeholder được xóa theo phạm vi hardware đã chốt; thêm FreeRTOSConfig.h và project_types.h.

## Lệnh và kết quả

Chạy trong Firmware:

```text
cmake --preset debug
cmake --build --preset debug
PASS — obstacle_car.elf, .hex, .bin và map được tạo

bash tools/check_constraints.sh build/debug/obstacle_car.elf
== Nguon ==
OK khong co HAL_Delay()
OK khong co printf/sprintf/snprintf
OK khong co cap phat dong
== Build ==
OK elf sach: khong co pvPortMalloc/printf
OK khong co heap_*.c
Tat ca dat.

arm-none-eabi-size build/debug/obstacle_car.elf
text    data    bss     dec     hex
14100   24      8528    22652   587c

cmake --preset release
cmake --build --preset release
PASS
text    data    bss     dec     hex
12432   24      8528    20984   51f8
```

Ở lần gọi Bash đầu tiên từ PowerShell, môi trường thiếu dirname khiến script bỏ qua ELF; kết quả đó **không được dùng**. Đã chạy lại với Git Bash có /usr/bin trong PATH, xác nhận ELF tồn tại và cả mục Build được kiểm tra như trên.

## Flash và RAM

| Bản | Flash theo linker | RAM theo linker | Giới hạn |
| --- | --- | --- | --- |
| Debug | 14.124 byte (21,55%) | 8.544 byte (41,72%) | Flash 65.536; RAM 20.480 byte |
| Release | 12.456 byte (19,01%) | 8.544 byte (41,72%) | Flash 65.536; RAM 20.480 byte |

Bản Debug qua `size -A`: .data 16 byte + .bss 7.504 byte + ._user_heap_stack 1.024 byte = **8.544 byte RAM đã bố trí**. Output size dạng ngắn tính 8 byte init/fini array trong cột data dù chúng ở Flash, nên lấy data+bss thành 8.552 byte là ước lượng bảo thủ, vẫn dưới 20 KB. Cột dec là tổng section, không phải riêng RAM.

Các thành phần xác nhận qua symbol sizes: stack ứng dụng 5.120 byte; idle/timer stacks tổng 1.024 byte; 7 TCB tổng 588 byte; hai queue control 144 byte; range/IMU payload 16 + 24 byte; event group 24 byte. Timer queue và globals khác nằm trong tổng map. Còn 11.936 byte so với vùng RAM linker, nhưng phần đó không phải cam kết đủ cho thuật toán tương lai.

## Kiểm tra liên kết và tính nhất quán

- `arm-none-eabi-nm` xác nhận SVC_Handler, PendSV_Handler, SysTick_Handler và xPortSysTickHandler là symbol triển khai, không chỉ weak default.
- SVC/PendSV do port.c định nghĩa qua alias; không có duplicate C wrapper.
- Source tree không còn driver IR; đúng 6 MCAL + 6 Devices + 4 App + main/IRQ.
- `git diff --check` đạt; các đường dẫn tài liệu nội bộ được rà lại.
- Mọi phép build trên là kiểm tra biên dịch/liên kết; các stub không được gọi có thể bị --gc-sections loại khỏi ELF.

## Chưa được xác nhận

Chưa nạp board, chưa kiểm tra scheduler/tick bằng debugger, chưa đo UART/clock/pin điện, chưa chạy motor/cảm biến và chưa đo stack high-water mark hoặc STOP latency. Thuật toán vẫn TODO/NOT_READY; không dùng bản skeleton này như firmware xe hoàn chỉnh.

Lần nghiệm thu kế tiếp cần kết quả bench theo [TEST_PLAN](TEST_PLAN.md), nhất là STBY/reset, Echo 3.3 V, fault shutdown và timing khi tích hợp.
