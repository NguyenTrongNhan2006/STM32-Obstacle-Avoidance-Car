# Trạng thái hiện thực hóa — checklist

[Về mục lục](README.md) · [Vai trò firmware](FIRMWARE_GUIDE.md) · [Hướng dẫn triển khai](IMPLEMENTATION_GUIDE.md) · [Kiểm thử](TEST_PLAN.md) · [Sơ đồ Mermaid](../docs/diagrams/)

**Nguồn:** quét toàn bộ `Firmware/` (trừ `ThirdParty/`) tìm `STATUS_NOT_READY`, `IMPLEMENT`, `TO DO`.
File này mô tả **trạng thái mã nguồn**, không phải hành vi đã đo trên xe.
Cập nhật mỗi khi một mục được hiện thực hóa và kiểm chứng.

---

## 1. Phát hiện chính

**Lớp App đã xong, không phải stub.** `obstacle_avoidance.c`, `safety_monitor.c` và
`robot_car.c` đều có logic đầy đủ. Toàn bộ phần thiếu nằm ở **MCAL + Devices + thân
5 task**. Luồng tránh vật cản bị chặn bởi đúng bốn chuỗi đứt:

```text
SENSING (range)   timebase_capture_us ✗ → ultrasonic_read ✗ → sensor_manager_update ✗ → qRangeMailbox
SENSING (imu)     i2c_read ✗ → mpu6050_read ✗ → sensor_manager_update ✗ → qImuMailbox
ACTUATION         pwm_write ✗ → motor_apply ✗ → TB6612
ORCHESTRATION     task_safety / task_sensor / task_decision ✗ (chỉ vTaskDelay)
```

### Ràng buộc then chốt: không thể bỏ qua MPU6050

`safety_update()` set `SAFETY_BIT_SENSOR_FAULT` khi `!range_ok || !imu_ok`, và bit đó
chỉ được xoá lúc rearm nếu **cả hai** mẫu hợp lệ:

```c
if (range_ok && imu_ok) { clear |= SAFETY_BIT_SENSOR_FAULT; }
```

Nghĩa là **thiếu IMU thì `safety_is_clear_to_run()` vĩnh viễn `false`**, `motor_apply()`
vĩnh viễn trả `NOT_READY`, và FSM không bao giờ rời `CAR_IDLE`. I2C + MPU6050 nằm trên
đường găng, không phải việc phụ. Không có đường tắt "chạy thử chỉ với siêu âm".

---

## 2. Danh sách file thiếu logic

### 2.1 Stub hoàn toàn — mọi hàm trả `NOT_READY`

| File | Hàm | Chặn cái gì | Bước |
| --- | --- | --- | --- |
| `Drivers/MCAL/Src/pwm.c` | `pwm_init` `pwm_write` `pwm_stop` | motor | 4 |
| `Drivers/MCAL/Src/i2c.c` | `i2c_init` `i2c_read` `i2c_write` | IMU → safety | 3 |
| `Drivers/MCAL/Src/exti.c` | `exti_init` `exti_read_pending` `exti_irq_capture` | *(không chặn — button dùng polling)* | 5 |
| `Drivers/Devices/Src/ultrasonic_hcsr04.c` | `ultrasonic_init` `_request` `_read` | khoảng cách | 2 |
| `Drivers/Devices/Src/mpu6050.c` | `mpu6050_init` `_read` `_calibrate` | tilt + xoá `SENSOR_FAULT` | 3 |
| `Drivers/Devices/Src/buzzer.c` | `buzzer_init` `_set` `_update` | *(không chặn)* | 5 |

### 2.2 Stub một phần

| File | Hàm hỏng | Hàm đã chạy được | Bước |
| --- | --- | --- | --- |
| `Drivers/MCAL/Src/timebase.c` | **`timebase_capture_us`** — TIM2 cố tình chưa init | `timebase_init` ✅ · `timebase_now_ms` ✅ (`HAL_GetTick`) | 2 |
| `Drivers/Devices/Src/motor_tb6612.c` | **`motor_init`** · **`motor_apply`** — luôn `NOT_READY`, không bao giờ bật bridge | validate tham số ✅ · gọi safety predicate ✅ · `gpio_emergency_stop` ✅ | 4 |
| `App/Src/sensor_manager.c` | **`sensor_manager_update`** — mailbox vĩnh viễn `SAMPLE_NOT_READY` | `_init` ✅ · `_get_latest` ✅ | 2–3 |
| `Drivers/MCAL/Src/uart_debug.c` | `uart_debug_read` | `_init` `_write` `uart_log` `uart_log_u32` ✅ | 5 |
| `Core/Src/main.c` | **5 thân task** chỉ `vTaskDelay(100 ms)` | boot · clock 72 MHz · tạo task static ✅ | 1 |
| `Core/Src/stm32f1xx_it.c` | 49 × `UNIMPLEMENTED_IRQ` → `configASSERT(0)` | SysTick wrapper ✅ | 0 |

### 2.3 Đã có logic thật — không cần làm lại

`App/Src/obstacle_avoidance.c` · `App/Src/safety_monitor.c` · `App/Src/robot_car.c` ·
`Drivers/Devices/Src/button.c` · `Drivers/Devices/Src/status_led.c` · `Drivers/MCAL/Src/gpio.c`

---

## 3. Checklist theo bước

Nguyên tắc: **actuation làm sau cùng**; mỗi bước phải tự kiểm chứng được bằng UART
trước khi sang bước kế.

### Bước 0 — Mở đường build ✅ ĐÃ XONG

- [x] `Core/Inc/stm32f1xx_hal_conf.h`: bật `HAL_TIM_MODULE_ENABLED`, `HAL_I2C_MODULE_ENABLED`
- [x] `Firmware/CMakeLists.txt`: thêm `stm32f1xx_hal_tim.c`, `_tim_ex.c`, `_i2c.c`
- [x] `Core/Src/stm32f1xx_it.c`: định tuyến `TIM2_IRQHandler`, `I2C1_EV_IRQHandler`,
      `I2C1_ER_IRQHandler` về module chủ sở hữu thay vì `configASSERT(0)`
- [x] `timebase.h` / `i2c.h`: thêm điểm vào ISR (`timebase_irq_capture`,
      `i2c_irq_event`, `i2c_irq_error`) theo đúng mẫu `exti_irq_capture` có sẵn

> **Vì sao phải làm trước:** bật ngắt ngoại vi khi handler còn là
> `UNIMPLEMENTED_IRQ` thì ngắt đầu tiên rơi vào `configASSERT(0)` và treo máy.
> Định tuyến trước, bật NVIC sau.

### Bước 1 — Nối thân 5 task ✅ ĐÃ XONG

- [x] `task_safety` → `safety_update()`, chu kỳ **10 ms** (`SAFETY_TARGET_PERIOD_MS`)
- [x] `task_sensor` → `sensor_manager_update()`, chu kỳ **10 ms** (`SENSOR_IMU_PERIOD_MS`, deadline range 60 ms do chính module giữ)
- [x] `task_decision` → `robot_car_update()`, chu kỳ **20 ms** (`DECISION_TARGET_PERIOD_MS`)
- [x] `task_log` → in `egSafety` khi đổi + heartbeat 1 s
- [x] `task_buzzer` → `buzzer_update()`, chu kỳ 100 ms
- [x] Cả 5 task dùng `xTaskDelayUntil` thay `vTaskDelay`

#### Kết quả build sau bước 0 + 1

| Bản | Flash | RAM | So với baseline `cc83423` |
| --- | --- | --- | --- |
| Debug | 16 240 B (**24,78 %**) | 8 576 B (**41,88 %**) | Flash **+2 116 B**, RAM **+32 B** |
| Release | 14 016 B (**21,39 %**) | 8 576 B (**41,88 %**) | Flash **+1 560 B** |

`bash tools/check_constraints.sh build/debug/obstacle_car.elf` → **Tất cả đạt**
(không `HAL_Delay`, không `printf`, không cấp phát động, ELF sạch, không `heap_*.c`).
Không warning nào từ mã dự án với `-Wall -Wextra -Wpedantic -Wshadow -Wdouble-promotion`.

**Flash tăng chủ yếu không phải do bật HAL TIM/I2C** (chưa hàm nào gọi tới nên
`--gc-sections` vẫn cắt bỏ). Nguyên nhân là các hàm App/Devices trước đây **không ai
gọi nên bị loại khỏi ELF** — đúng như ghi chú ở `BUILD_VERIFICATION.md` §Kiểm tra
liên kết. Sau bước 1 chúng đã vào ảnh: `safety_update`, `sensor_manager_update`,
`robot_car_update`, `obstacle_avoidance_update`, `motor_apply`, `status_led_set`,
`button_read`, `buzzer_update`, `uart_log_u32`.

Xác minh định tuyến IRQ bằng `objdump`:

```text
080004ae <TIM2_IRQHandler>:    bl 8000938 <timebase_irq_capture>
080004b6 <I2C1_EV_IRQHandler>: bl 800098c <i2c_irq_event>
080004be <I2C1_ER_IRQHandler>: bl 8000994 <i2c_irq_error>
```

#### Kiểm chứng trên board — CHƯA CHẠY

Chưa nạp lên phần cứng. Kỳ vọng khi nạp:

- LED PC13 nháy theo `LED_IDLE` — sáng 60 ms mỗi chu kỳ 1 s.
- UART in `safety=3` (`SAFETY_BIT_STOP | SAFETY_BIT_SENSOR_FAULT`, chốt từ lúc boot),
  `range_st=4` và `imu_st=4` (`SAMPLE_NOT_READY`).
- Nhấn nút → rearm **thất bại** vì chưa có mẫu hợp lệ; `SENSOR_FAULT` vẫn còn,
  `safety=` đổi từ 3 xuống 2 rồi giữ nguyên.
- **Xe đứng yên. Đây là kết quả đúng, không phải lỗi** — `motor_apply()` vẫn luôn
  trả `NOT_READY` và `gpio_emergency_stop()` chạy vô điều kiện mỗi 20 ms.

### Bước 2 — Chuỗi đo khoảng cách ✅ CODE XONG, CHƯA ĐO TRÊN BOARD

- [x] `timebase.c` — TIM2 **PWM Input Mode**: TI1 (PA0) nối nội bộ tới cả IC1 (cạnh lên,
      reset bộ đếm qua slave mode) và IC2 (cạnh xuống → `CCR2` = độ rộng xung)
- [x] Xử lý tràn bộ đếm: **`URS = 1`** để chỉ tràn thật mới sinh ngắt update
- [x] `timebase_capture_us()` **non-blocking** — trả `NOT_READY` khi đang đo
- [x] `ultrasonic_hcsr04.c` — xung Trig 10 µs trên PA1, chống chồng lấn, tôn trọng
      `SENSOR_RANGE_PERIOD_MS = 60`, `ECHO_TIMEOUT_US = 30000` → `SAMPLE_TIMEOUT`
- [x] `sensor_manager.c :: sensor_manager_update()` — publish vào `qRangeMailbox`,
      timestamp là **thời điểm phát Trig**
- [x] `main.c :: task_log` — in `range_mm=` và `range_age_ms=` khi mẫu hợp lệ
- [ ] **Kiểm chứng trên board** — CHƯA CHẠY

#### Quyết định thiết kế

**PWM Input Mode thay vì EXTI hai cạnh.** Phần cứng đo và chốt độ rộng xung, nên độ trễ
ISR và jitter của scheduler **không thể** làm sai phép đo. EXTI chỉ báo có cạnh; thời
điểm đọc bộ đếm lại phụ thuộc lúc ISR chạy.

**`URS = 1` là bắt buộc.** Slave mode Reset cũng sinh update event mỗi lần nó reset bộ
đếm — tức mỗi cạnh lên của Echo. Không đặt `URS` thì ISR hiểu nhầm mọi cạnh lên thành
tràn bộ đếm và **mọi phép đo đều thất bại**. Lỗi này không treo máy nên rất khó thấy.

**Hai lớp deadline độc lập:** phần cứng tràn bộ đếm ở 65 536 µs; phần mềm so `HAL_GetTick`
với 30 ms. Mất cạnh xuống thì lớp cứng bắt; treo cả ISR thì lớp mềm vẫn bắt.

#### Thay đổi API — cần review chung

Thêm **`timebase_capture_arm()`** vào `timebase.h`. Không có nó,
`timebase_capture_us()` không phân biệt được kết quả mới với kết quả còn sót của lần đo
trước. Đây là API dùng chung giữa MCAL và Devices, `CLAUDE.md` yêu cầu review chung.

#### Busy-wait 10 µs — có chủ đích

Xung Trig 10 µs là **yêu cầu của datasheet**, không phải chờ sự kiện. Đo bằng chính bộ
đếm 1 µs/tick của TIM2 nên không đổi theo mức tối ưu của trình biên dịch. Gọi mỗi 60 ms
trong `tSensor` (priority 2) ⇒ **0,017 % CPU**, trễ tối đa gây cho `tSafety` là 10 µs
trên chu kỳ 10 ms. Muốn bỏ hẳn: dùng **TIM4 (đang trống)** ở One-Pulse Mode — nhưng đó
là quyết định về quyền sở hữu timer, phải chốt trong `board_config.h` trước.

#### Kết quả build sau bước 2

| Bản | Flash | RAM | So với bước 0+1 |
| --- | --- | --- | --- |
| Debug | 19 260 B (**29,39 %**) | 8 680 B (**42,38 %**) | Flash +3 020 B, RAM +104 B |
| Release | 16 416 B (**25,05 %**) | 8 672 B (**42,34 %**) | Flash +2 400 B |

`check_constraints.sh` → **Tất cả đạt**. Không warning từ mã dự án.

Stack usage đo bằng `-fstack-usage` (file `.su`) — chuỗi sâu nhất của `tSensor`:

```text
task_sensor 16 + sensor_manager_update 24 + ultrasonic_read 16 + timebase_capture_us 16 = 72 B
timebase_irq_capture = 0 B   (ISR lá, không có khung stack riêng)
timebase_init        = 80 B  (chạy trước scheduler, trên MSP)
```

Stack mỗi task là 1 024 B ⇒ biên rất rộng. Vẫn phải đo
`uxTaskGetStackHighWaterMark()` trên board, con số tĩnh không thay thế được.

#### Kiểm chứng trên board — CHƯA CHẠY

1. **Trước khi cắm Echo:** đo bằng đồng hồ rằng điện áp vào PA0 ≤ 3,3 V.
   **PA0 không phải chân 5V-tolerant.**
2. Đặt vật cản ở 100 / 300 / 1000 mm, so `range_mm=` với thước — sai số kỳ vọng ±10 mm.
3. Che kín cảm biến hoặc rút dây Echo → `range_st=2` (`SAMPLE_TIMEOUT`) trong vòng
   ~31 ms, **không treo**, `range_mm=` không được in.
4. Đưa vật cản sát < 20 mm → `range_st=0` (`SAMPLE_OK`) với `range_mm=20` — giá trị bị
   **kẹp**, không báo lỗi (xem dưới).
5. `range_age_ms=` phải dao động trong khoảng 0–70 ms; vượt `SENSOR_STALE_MS = 200`
   nghĩa là chuỗi đo đang kẹt.

#### Chính sách giá trị ngoài dải — đã chốt

Chỉ **mất xung Echo** hoặc **quá `ECHO_TIMEOUT_US` (30 ms)** mới là thất bại của phép đo.
Một kết quả nằm ngoài dải datasheet vẫn là **bằng chứng có thật** về khoảng cách, chỉ là
không chính xác — nên nó bị **kẹp** chứ không bị báo lỗi:

| Đo được | Báo ra | Hướng lệch | Hệ quả ở FSM |
| --- | --- | --- | --- |
| `< 20 mm` | `20 mm`, `SAMPLE_OK` | — | dưới `D_STOP_MM` → `CAR_STOP` → quay né |
| `20..4000 mm` | giữ nguyên, `SAMPLE_OK` | — | bình thường |
| `> 4000 mm` | `4000 mm`, `SAMPLE_OK` | **gần hơn** thực tế | không bao giờ tạo ra "đường trống" giả |
| mất echo / > 30 ms | `0 mm`, `SAMPLE_TIMEOUT` | — | `range_valid = false` → `CAR_FAULT` |

Kẹp cận dưới thay vì báo `SAMPLE_ERROR` vì báo lỗi sẽ đẩy xe vào `CAR_FAULT` — dừng hẳn
và phải rearm bằng tay — quá nặng cho tình huống "vật cản rất sát" mà đáng lẽ chỉ cần né.

> ⚠️ `SAMPLE_TIMEOUT` **tuyệt đối không** được quy về `0 mm` hay một khoảng cách rất xa
> **rồi dùng như phép đo hợp lệ**. Hiện tại `value = 0` đi kèm `status = SAMPLE_TIMEOUT`;
> consumer bắt buộc đọc `status` trước, và `safety_monitor` coi mọi status khác
> `SAMPLE_OK` là fault.
>
> ⚠️ Xe vẫn **không chạy** sau bước này: thiếu IMU nên `SAFETY_BIT_SENSOR_FAULT` chưa
> xoá được. Đúng thiết kế — xem §1.

### Bước 3 — IMU ✅ CODE XONG, CHƯA ĐO TRÊN BOARD

- [x] `i2c.c` — I2C1 400 kHz (`IMU_I2C_SPEED_HZ` mới trong `board_config.h`),
      transaction có timeout `I2C_TIMEOUT_MS = 10`
- [x] **Stuck-bus recovery** — phát tối đa 9 xung SCL khi SDA kẹt thấp, rồi phát STOP
- [x] `mpu6050.c` — `WHO_AM_I == 0x68`, đánh thức + DLPF 44 Hz + 125 Hz + ±250 °/s + ±2 g,
      burst read 14 byte, timestamp lúc bắt đầu giao dịch
- [x] `mpu6050_calibrate()` — ước lượng bias gyro, **từ chối nếu phát hiện chuyển động**
- [x] `sensor_manager.c` — nhánh IMU ở nhịp `SENSOR_IMU_PERIOD_MS = 10`, tự khởi tạo lại
      mỗi 500 ms khi driver báo chưa sẵn sàng
- [ ] **Kiểm chứng trên board** — CHƯA CHẠY

#### Tilt nằm ở đâu — và vì sao không đặt trong `mpu6050.c`

`mpu6050.c` **chỉ trả raw signed + timestamp + status**. Quyết định "nghiêng bao nhiêu
thì dừng" thuộc `safety_monitor.c :: imu_upright()`, nơi sở hữu các bit inhibit.
`IMPLEMENTATION_GUIDE.md` §5 đã chốt điều này: *"Việc quyết định nghiêng quá mức thì
dừng thuộc `safety_monitor`, không thuộc lớp I2C."*

Thêm một hàm tilt vào `mpu6050.c` sẽ **nhân đôi** logic đã có và đặt chính sách vào lớp
thiết bị. Chuỗi hiện tại đã đầy đủ: `mpu6050_read` → `qImuMailbox` →
`safety_update` → `imu_upright` → `SAFETY_BIT_TILT_FAULT`.

Phép kiểm tra dùng **tỉ số bình phương** `az² / (ax²+ay²+az²) ≥ 3/4`, nên thang đo
±2 g cấu hình ở bước này **không ảnh hưởng** kết quả — đổi full-scale không làm sai.

#### `i2c_read`/`i2c_write` là bounded-blocking, không phải non-blocking

Chữ ký hàm buộc phải vậy: chúng nhận buffer ra và trả kết quả ngay trong một lần gọi,
nên không có chỗ để báo "đang chạy". Đổi sang non-blocking thật cần tách arm/poll như
`timebase_capture_arm()`.

Chi phí thực tế: burst 14 byte @400 kHz ≈ **0,4 ms**; worst case là `I2C_TIMEOUT_MS = 10 ms`
khi bus hỏng. Người gọi là `tSensor` (priority 2):

- `tSafety` (priority 4) vẫn preempt được ngay ⇒ **đường an toàn không bị trễ**
- `tDecision` (priority 1) có thể bị trễ tới 10 ms trên chu kỳ 20 ms của nó

Nếu đo được trễ thật và thấy không chấp nhận được: chuyển sang `HAL_I2C_Mem_Read_IT`
+ semaphore từ `i2c_irq_event()` — ISR đã được định tuyến sẵn từ bước 0. Hiện **ngắt
I2C1 chưa bật trong NVIC**, hai ISR chỉ đóng vai lưới an toàn chống interrupt storm.

#### Hai chính sách đáng chú ý

**Bus lỗi ⇒ thiết bị bị buộc về trạng thái chưa khởi tạo.** Một MPU6050 vừa sụt nguồn
sẽ quay lại mặc định (đang SLEEP, thang đo khác). Đọc tiếp mà không cấu hình lại sẽ cho
số liệu vô nghĩa nhưng **trông như thật**. Nên `mpu6050_read()` tự đặt
`initialized = false` khi giao dịch hỏng, và `sensor_manager` gọi lại `mpu6050_init()`
(kèm kiểm tra `WHO_AM_I`) tối đa mỗi 500 ms.

**IMU vắng mặt không được làm chết boot.** `sensor_manager_init()` bỏ qua kết quả của
`mpu6050_init()`. Bắt boot fail ở đó sẽ làm board không khởi động được chỉ vì một sợi
dây lỏng. `safety_monitor` đã chốt `SAFETY_BIT_SENSOR_FAULT` từ lúc boot nên xe vẫn
không thể chạy.

#### Kết quả build sau bước 3

| Bản | Flash | RAM | So với bước 2 |
| --- | --- | --- | --- |
| Debug | 23 668 B (**36,11 %**) | 8 792 B (**42,93 %**) | Flash +4 388 B, RAM +112 B |
| Release | 20 048 B (**30,59 %**) | 8 784 B (**42,89 %**) | Flash +3 632 B |

Flash tăng chủ yếu là `stm32f1xx_hal_i2c.c` (~4 KB) — giá của việc dùng HAL blocking
thay vì tự viết. Đổi lại: xử lý đúng các errata I2C của F1 mà ta không phải tự dò.

`check_constraints.sh` → **Tất cả đạt**. Không warning từ mã dự án.

Stack chain sâu nhất của `tSensor` (từ file `.su`):

```text
task_sensor 16 + sensor_manager_update 24 + update_imu 40 + mpu6050_read 40
  + i2c_read 24 + HAL_I2C_Mem_Read 64 + I2C_RequestMemoryRead 48
  + I2C_WaitOnFlagUntilTimeout 24  =  280 B  /  1024 B
```

#### Kiểm chứng trên board — CHƯA CHẠY

1. **Trước khi cấp nguồn:** xác nhận MPU6050 chạy ở 3,3 V và có điện trở kéo lên trên
   SDA/SCL. Firmware **không** bật pull-up nội — pull-up yếu của MCU làm sườn tín hiệu
   xấu đi chứ không tốt lên.
2. Cắm IMU → `imu_st=0` (`SAMPLE_OK`) trong log; `safety=` giảm từ `3` xuống `1`
   (chỉ còn `SAFETY_BIT_STOP`).
3. Nhấn nút khi cả hai cảm biến hợp lệ → `safety=0`, `safety_is_clear_to_run()` lần đầu
   trả `true`. **Xe vẫn không chạy** vì `motor_apply()` còn `NOT_READY` (bước 4).
4. **Nghiêng xe > 30°** → `safety=4` (`SAFETY_BIT_TILT_FAULT`). Đặt lại phẳng rồi nhấn
   nút → xoá được.
5. **Rút dây SDA hoặc SCL** → `imu_st=1` hoặc `2`, `safety=2` (`SENSOR_FAULT`) trong
   vòng ~200 ms. Cắm lại → tự phục hồi trong ≤ 500 ms, **không cần reset**.
6. **Nối tắt SDA xuống GND rồi reset MCU** → stuck-bus recovery phải gỡ được bus; nếu
   không, `i2c_init()` vẫn trả OK nhưng mọi giao dịch sẽ `STATUS_TIMEOUT`.
7. In raw 3 trục accel để **xác nhận `accel_raw[2]` là trục thẳng đứng và dương khi xe
   nằm phẳng** — giả định `[DO]` của `imu_upright()` phụ thuộc hoàn toàn vào hướng lắp.

> ⚠️ `mpu6050_calibrate()` **chưa được gọi ở đâu**. Nó phải được gọi có chủ đích khi đã
> biết chắc xe đứng yên, không phải tự động lúc khởi động.

### Bước 4 — Actuation ⬜ *(sau cùng)*

- [ ] `pwm.c` — sở hữu TIM3, hai kênh khởi tạo duty 0, `pwm_write(channel, duty_per_mille)`
- [ ] `motor_tb6612.c` — chốt **bảng chân lý brake/coast** trước, rồi arbitration
      fault-dominant và đổi chiều an toàn (qua brake, không đảo trực tiếp)
- [ ] **Kiểm chứng:** kê bánh khỏi mặt đất, thử từng bánh; **đo thời gian từ lúc nhấn
      STOP đến khi PWM về 0** — `CLAUDE.md` yêu cầu chứng minh con số này trước khi có
      motor thật
- [ ] **Điều kiện phần cứng:** pull-down 10k trên STBY (PB5) phải có trước khi cấp nguồn động lực

### Bước 5 — Phụ trợ, không chặn luồng chính ⬜

- [ ] `buzzer.c` — xác nhận loại active/passive và tầng transistor trước
- [ ] `exti.c` — chỉ cần nếu muốn phản ứng nút nhanh hơn polling 10 ms
- [ ] `uart_debug_read` — chỉ khi thực sự cần giao thức lệnh

---

## 4. Hai việc nên chốt trước khi viết code

1. **Hướng lắp và full-scale range của MPU6050.** `TILT_COS2_NUM/DEN = 3/4` giả định
   `accel_raw[2]` là trục thẳng đứng và đọc dương khi xe nằm phẳng. Sai hướng lắp thì
   tilt check vô nghĩa. Xác nhận bằng cách in raw ba trục ở bước 3 **trước khi** tin vào
   `imu_upright()`.
2. **Bảng chân lý brake/coast của TB6612.** `motor_tb6612.c` ghi rõ
   `TO DO: resolve brake/coast truth table before enabling STBY`. Đây là việc đọc
   datasheet, làm xong trước bước 4.

---

## 5. Ghi chú kỹ thuật phát sinh

- **`HAL_UART_Transmit` là busy-wait**, không phải block của RTOS. `tLog` chạy ở
  priority 0 nên không làm trễ task cao hơn, nhưng nó **đốt CPU** suốt thời gian
  truyền. Khi khối lượng log tăng, chuyển sang DMA + semaphore từ ISR.
- `tLog` và `tBuzzer` cùng priority 0 với Idle task, phụ thuộc
  `configUSE_TIME_SLICING = 1`. Cả hai **bắt buộc** block hoặc yield, không được spin.
