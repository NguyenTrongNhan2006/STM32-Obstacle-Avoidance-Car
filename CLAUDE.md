# CLAUDE.md

Hướng dẫn cho Claude Code khi làm việc trong repo này.

## Bối cảnh

Xe tránh vật cản dùng **STM32F103C8T6** (Cortex-M3, 72 MHz, 64 KB flash, **20 KB SRAM**),
đang xây dựng trên **FreeRTOS**, định hướng port sang Zephyr ở giai đoạn sau.

Repo xuất phát từ trạng thái sườn: mọi `.c/.h` chỉ chứa `// TO DO`, `CMakeLists.txt`
chỉ chứa `# TO DO`. Hiện chỉ có ba file config là có nội dung thật.

## Đọc trước khi làm gì

Toàn bộ quyết định kiến trúc và **lý do** nằm trong comment đầu ba file này —
không nằm trong `About/`:

| File | Chứa gì |
| --- | --- |
| `Firmware/Config/board_config.h` | Pinout, timer allocation, 6 cảnh báo phần cứng |
| `Firmware/Config/app_config.h` | Task layout, RAM budget, ngưỡng điều khiển và cách dẫn xuất |
| `Firmware/Core/Inc/FreeRTOSConfig.h` | Cấu hình kernel và ba quyết định nền |

`About/` chứa kế hoạch và quy trình kiểm thử của tác giả, viết từ giai đoạn
bare-metal. Khi `About/` mâu thuẫn với ba file trên, **ba file trên thắng** —
chúng phản ánh thiết kế RTOS hiện tại. Riêng `About/TEST_PLAN.md` vẫn áp dụng.

## Ràng buộc bắt buộc

Đây là các luật đã chốt, không phải gợi ý. Vi phạm là lỗi, kể cả khi code biên dịch được.

**Bộ nhớ**
- Static allocation. `configSUPPORT_DYNAMIC_ALLOCATION = 0`.
- Không `pvPortMalloc`, không `malloc`, không `heap_*.c` trong build.
- Không `printf`/`sprintf`/`snprintf` của newlib — ngốn stack và kéo theo
  malloc không thread-safe.

**Timing**
- **Cấm `HAL_Delay()`** trong toàn bộ codebase. HAL dùng chung SysTick với
  kernel qua `vApplicationTickHook()`, nên `HAL_Delay` sẽ block cả task.
  Thay bằng `vTaskDelay()`.
- Task có chu kỳ cố định dùng `vTaskDelayUntil()`, không phải `vTaskDelay()`.
- **Cấm dùng `vTaskDelay()` để hiện thực thời lượng của một state trong FSM.**
  Viết `TURN_LEFT` thành "quay rồi `vTaskDelay(400)`" sẽ khóa FSM 400 ms —
  nút STOP và cảm biến bị bỏ qua suốt khoảng đó. Dùng so sánh deadline.

**Ngắt**
- Mọi ISR gọi `...FromISR()` phải có NVIC priority number **>= 5**
  (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`). Vi phạm không crash ngay
  mà làm hỏng kernel list ngẫu nhiên. Bảng priority ở `board_config.h` (`IRQ_PRIO_*`).
- ISR chỉ capture event / chụp timestamp / set bit rồi `portYIELD_FROM_ISR`.
  Không chạy thuật toán, không chờ I2C, không in log trong ISR.
- Giữ `configASSERT` bật — chính nó kích hoạt `vPortValidateInterruptPriority()`.

**Tài nguyên**
- Mọi lệnh motor đi qua **đúng một hàm** `motor_apply()`. Hàm đó đọc `egSafety`
  và ép lệnh về 0 nếu có bit fault. Đây là điểm cưỡng chế duy nhất.
- Mỗi timer và mỗi bus có đúng một chủ sở hữu (bảng trong `board_config.h`).
  Driver không tự cấu hình lại timer đang phục vụ module khác.
- Dữ liệu cảm biến publish bằng **copy vào mailbox** (`xQueueOverwrite`, queue
  độ dài 1), không phải mutex, không phải con trỏ chia sẻ. Nhờ vậy `tSafety`
  không lấy lock nào → không có priority inversion trên đường an toàn.
- Không thêm mutex trừ khi thực sự xuất hiện resource có hai consumer.

**Dữ liệu cảm biến**
- Mỗi mẫu mang đủ: giá trị + đơn vị + timestamp + `sample_status_t`.
- Mất echo **không** được biến thành `0 mm` hay một khoảng cách rất xa rồi
  dùng như phép đo hợp lệ. Thiếu cảm biến không đồng nghĩa đường trống.

## Thứ tự làm việc

Theo mốc A→J trong `About/IMPLEMENTATION_GUIDE.md` §5. Mỗi mốc: chốt interface
trong `.h` → triển khai `.c` → thử riêng → ghi kết quả → mới ghép vào App.

Hiện đang ở **mốc A**: dựng project tối thiểu build và nạp được (CMSIS, startup,
linker script, CMakeLists, toolchain), rồi `Core` + `gpio` + `timebase` + `uart_debug`.
Chưa động đến motor và cảm biến.

## Giá trị đánh dấu `[DO]`

Trong `app_config.h`, giá trị `[DO]` là số khởi đầu an toàn, **chưa đo trên xe thật**.
Đừng coi chúng là đã chốt. Khi tác giả đo được số thật, sửa giá trị và ghi lý do
kèm số liệu vào `About/`.

Giá trị `[CO DINH]` xuất phát từ datasheet hoặc ràng buộc kiến trúc — không tự ý đổi.

## Cách làm việc

- **Đề xuất kế hoạch trước, không sinh code hàng loạt.** Tác giả muốn duyệt từng bước.
- Giải thích đánh đổi kỹ thuật khi có nhiều phương án, đừng chỉ chọn im lặng.
- Khi phát hiện ràng buộc phần cứng mâu thuẫn với thiết kế, **nói ra ngay** thay vì
  lách. Ví dụ đã gặp: Echo của HC-SR04 xuất 5V nên phải nằm trên chân 5V-tolerant,
  điều này kéo theo việc dời MPU6050 từ I2C1 sang I2C2.
- Giữ thuật ngữ kỹ thuật tiếng Anh (Task, Mutex, Semaphore, Queue, ISR,
  Context Switch, Device Tree, Kconfig, HAL).

## Phần cứng — hai điều kiện chưa xong

Ghi ở đây vì chúng chặn việc chạy thử, không phải việc viết code:

1. **Điện trở kéo xuống 10k trên chân STBY của TB6612.** Không có nó, motor có thể
   chạy ngoài ý muốn trong lúc MCU reset. Điều kiện an toàn bắt buộc.
2. **Đo `v` và `d_coast` trên xe thật.** Ngưỡng dừng 250 mm hiện dẫn xuất từ giả định
   350 mm/s. Quan hệ tuyến tính với tốc độ — xe nhanh hơn thì ngưỡng phải tăng theo.
