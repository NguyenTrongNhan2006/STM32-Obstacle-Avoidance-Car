# FreeRTOS — task, ngắt và cơ chế đồng bộ

[◀ Về README chính](../../README.md) · [Sơ đồ phần cứng](hardware_block_diagram.md) · [Kiến trúc phần mềm](software_architecture.md) · [FSM điều khiển](fsm_control_flow.md)

Cấu hình kernel ở [`Firmware/Core/Inc/FreeRTOSConfig.h`](../../Firmware/Core/Inc/FreeRTOSConfig.h),
task layout ở [`Firmware/Config/app_config.h`](../../Firmware/Config/app_config.h).

> **Trạng thái:** 5 task đã được tạo và scheduler chạy, nhưng thân cả 5 task mới
> chỉ là `vTaskDelay()`. Đối tượng kernel (2 mailbox + 1 event group) đã tồn tại
> thật và các hàm App thao tác lên chúng đã viết xong.

---

## 1. Toàn cảnh luồng dữ liệu

```mermaid
flowchart TB
    subgraph ISR["NGẮT — mọi ISR gọi API RTOS phải có priority number 5..15"]
        direction LR
        I1["SysTick · 15<br/>ĐANG HOẠT ĐỘNG"]
        I2["Echo capture TIM2 · 5<br/>chưa bật"]
        I3["EXTI8 nút bấm · 5<br/>chưa bật"]
        I4["I2C1 · 6<br/>chưa bật"]
        I5["USART1 · 7<br/>chưa bật"]
    end

    subgraph SYNC["ĐỐI TƯỢNG KERNEL — static, đã tồn tại"]
        direction LR
        MB1["qRangeMailbox<br/>Queue len 1 · sample_t"]
        MB2["qImuMailbox<br/>Queue len 1 · imu_sample_t"]
        EGS["egSafety<br/>EventGroup · 3 bit chốt"]
    end

    subgraph TASKS["5 STATIC TASK — stack 256 word mỗi task"]
        direction TB
        T1["tSafety · prio 4"]
        T2["tSensor · prio 2"]
        T3["tDecision · prio 1"]
        T4["tLog · prio 0"]
        T5["tBuzzer · prio 0"]
    end

    HWOUT["TB6612 · STBY + PWM + IN1/IN2"]

    I1 -->|"HAL_IncTick rồi xPortSysTickHandler"| TASKS

    T2 -->|"sensor_manager_update()<br/>xQueueOverwrite bản copy"| MB1
    T2 -->|"xQueueOverwrite"| MB2

    MB1 -->|"xQueuePeek · không drain"| T1
    MB2 -->|"xQueuePeek"| T1
    MB1 -->|"xQueuePeek"| T3
    MB2 -->|"xQueuePeek"| T3

    T1 -->|"safety_update()<br/>set bit fault, clear khi rearm"| EGS
    EGS -->|"safety_is_clear_to_run()"| T3
    EGS -->|"qua con trỏ hàm is_safe"| MA

    T3 -->|"robot_car_update()<br/>đúng 1 lần gọi"| MA["motor_apply()"]
    MA --> HWOUT
    T3 -->|"status_led_set()"| LED["LED PC13"]
    T4 -->|"writer UART duy nhất"| I5
    T5 --> BUZ["Buzzer PB12"]
```

## 2. Bảng task

`configMAX_PRIORITIES = 5` → priority hợp lệ là **0..4**, số **lớn hơn = ưu tiên cao hơn**
(ngược với đánh số NVIC).

| Task | Prio | Chu kỳ skeleton | Chu kỳ đích | Stack | Chủ sở hữu | Nhiệm vụ |
| --- | --- | --- | --- | --- | --- | --- |
| `tSafety` | **4** | 100 ms | **10 ms** `[DO]` | 256 w / 1024 B | Nhân | Chốt STOP/fault, kiểm tra freshness và tilt |
| *(kernel)* `Tmr Svc` | 3 | sự kiện | — | 128 w / 512 B | — | Software timer |
| `tSensor` | 2 | 100 ms | range 60 ms, IMU 10 ms `[DO]` | 256 w / 1024 B | Hưng | Lấy mẫu, ghi 2 mailbox bản copy |
| `tDecision` | 1 | 100 ms | **20 ms** `[DO]` | 256 w / 1024 B | Nhân | FSM + `robot_car_update()` |
| `tLog` | 0 | 100 ms | — | 256 w / 1024 B | Hưng | Telemetry có giới hạn, **writer UART duy nhất** |
| `tBuzzer` | 0 | 100 ms | — | 256 w / 1024 B | Hưng | Mẫu còi theo deadline, phải block/yield ở prio 0 |
| *(kernel)* `IDLE` | 0 | — | — | 128 w / 512 B | — | — |

> Chu kỳ 100 ms hiện tại **chỉ là sleep của skeleton**, không phải deadline an toàn
> hay điều khiển. `tSafety` và `tDecision` phải xuống 10 ms / 20 ms trước khi có motor thật.
>
> `tLog` và `tBuzzer` cùng priority 0 với Idle task nên phụ thuộc
> `configUSE_TIME_SLICING = 1`; cả hai **bắt buộc** block hoặc yield, không được spin.

## 3. SysTick wrapper — HAL và kernel dùng chung một timer

F103C8T6 không còn timer rảnh để làm HAL timebase riêng, nên HAL và FreeRTOS dùng
chung SysTick. Cách nối nằm ở [`stm32f1xx_it.c`](../../Firmware/Core/Src/stm32f1xx_it.c):

```c
void SysTick_Handler(void)
{
    HAL_IncTick();                                          /* luôn chạy */
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();                              /* chỉ sau khi scheduler chạy */
    }
}
```

```mermaid
sequenceDiagram
    autonumber
    participant HW as SysTick 1 kHz
    participant W as SysTick_Handler (wrapper)
    participant HAL as HAL uwTick
    participant K as FreeRTOS kernel

    Note over HW,K: Giai đoạn 1 — trước vTaskStartScheduler
    HW->>W: ngắt mỗi 1 ms
    W->>HAL: HAL_IncTick()
    W->>W: xTaskGetSchedulerState() == NOT_STARTED
    Note right of W: bỏ qua kernel handler,<br/>nhưng timeout của HAL vẫn chạy đúng

    Note over HW,K: Giai đoạn 2 — sau khi scheduler chạy
    HW->>W: ngắt mỗi 1 ms
    W->>HAL: HAL_IncTick()
    W->>K: xPortSysTickHandler()
    K->>K: xTaskIncrementTick, đặt PendSV nếu cần
```

Ba hệ quả:

- **`configUSE_TICK_HOOK = 0`** — không cần tick hook vì wrapper đã gọi `HAL_IncTick()` trực tiếp.
- **Timeout của HAL hoạt động cả trước lẫn sau scheduler**, nên `HAL_RCC_OscConfig()`
  vẫn hết giờ đúng hạn nếu thạch anh HSE không khởi động.
- **`SVC_Handler` và `PendSV_Handler` alias thẳng vào `port.c`** qua macro trong
  `FreeRTOSConfig.h`, **không** bọc hàm C — chúng là naked handler.
  `stm32f1xx_it.c` vì vậy tuyệt đối không được định nghĩa lại hai tên đó.

## 4. Mailbox bản copy — vì sao không dùng mutex

```mermaid
flowchart LR
    subgraph GOOD["Mailbox: producer duy nhất, consumer peek"]
        direction TB
        P["tSensor — PRODUCER DUY NHẤT<br/>xQueueOverwrite(qRangeMailbox, &amp;sample)"] --> Q["Queue len 1<br/>copy toàn bộ struct"]
        Q --> C1["tSafety<br/>xQueuePeek, timeout 0"]
        Q --> C2["tDecision<br/>xQueuePeek, timeout 0"]
    end
```

| Đặc điểm | Vì sao |
| --- | --- |
| **Copy** toàn bộ `sample_t` / `imu_sample_t` | Consumer luôn thấy một mẫu nhất quán: giá trị + đơn vị + timestamp + status cùng một lần chụp |
| **Peek**, không receive | Không drain hàng đợi — nhiều consumer đọc được cùng một mẫu |
| Timeout **0** | `tSafety` (priority cao nhất) **không bao giờ block** → không có priority inversion trên đường an toàn |
| Producer **duy nhất** là `tSensor` | Không có tranh chấp ghi → không cần mutex |
| Khởi tạo bằng `SAMPLE_NOT_READY` | Mailbox rỗng không thể bị hiểu nhầm là "đo được 0 mm" |

`sensor_manager_get_latest()` trả `STATUS_OK` chỉ có nghĩa là **đã lấy được bản
copy**, không có nghĩa dữ liệu hợp lệ. Hai mẫu cũng **không** phải một lần chụp
đồng thời — caller bắt buộc tự kiểm tra `status` và tuổi của **từng** mẫu.

## 5. `egSafety` — fault chốt, rearm phải có chủ đích

```mermaid
flowchart TB
    BOOT["safety_init()<br/>SET ngay SAFETY_BIT_STOP<br/>và SAFETY_BIT_SENSOR_FAULT"] --> EG
    EG["egSafety"] --> CLR{"SAFETY_INHIBIT_MASK<br/>== 0 ?"}
    CLR -->|"có"| RUN["safety_is_clear_to_run() = true"]
    CLR -->|"không"| INH["mọi lệnh motor bị chặn"]

    U["safety_update(now_ms)"] --> F1{"range_ok và imu_ok ?"}
    F1 -->|"không"| S1["SET SENSOR_FAULT"]
    U --> F2{"imu_ok và KHÔNG upright ?"}
    F2 -->|"đúng"| S2["SET TILT_FAULT"]
    S1 --> EG
    S2 --> EG

    U --> B{"cạnh lên của nút bấm ?"}
    B -->|"đang có inhibit"| RE["REARM: chỉ clear bit đã CHỨNG MINH là hết<br/>STOP luôn clear<br/>SENSOR_FAULT chỉ khi range_ok và imu_ok<br/>TILT_FAULT chỉ khi imu_ok và upright"]
    B -->|"đang sạch"| ST["SET STOP"]
    RE --> EG
    ST --> EG
```

| Bit | Đặt khi nào | Xoá khi nào |
| --- | --- | --- |
| `SAFETY_BIT_STOP` | Lúc boot; mỗi lần nhấn nút khi hệ thống đang sạch | Khi nhấn nút lúc đang có inhibit |
| `SAFETY_BIT_SENSOR_FAULT` | Lúc boot; khi range hoặc IMU thiếu/quá hạn/lỗi | Chỉ khi rearm **và** cả hai mẫu đang hợp lệ |
| `SAFETY_BIT_TILT_FAULT` | Khi IMU hợp lệ nhưng xe không thẳng đứng | Chỉ khi rearm **và** IMU hợp lệ và đang thẳng đứng |

Bốn nguyên tắc:

1. **Chốt lúc boot.** Xe không thể chạy chỉ vì vừa cấp nguồn xong.
2. **Fault chốt lại (latch).** Không dòng nào tự xoá bit; dữ liệu vắng mặt hoặc
   quá hạn là **bằng chứng của lỗi**, không bao giờ là bằng chứng của sức khoẻ.
3. **Rearm chỉ xoá bit đã chứng minh là hết.** Nhấn nút không bao giờ xoá một
   fault chưa được chứng minh là đã biến mất.
4. **Một nút, hai vai trò, theo cạnh.** Đang sạch mà nhấn → STOP; đang bị chặn mà
   nhấn → rearm. `button_was_pressed` khởi tạo `true` để trạng thái nút chưa biết
   không bị hiểu nhầm thành một lần nhả đang chờ nhấn.

## 6. Kiểm tra tilt không cần quy đổi độ

Full-scale range của accelerometer **chưa xác nhận**, nên không thể quy raw sang
độ. `imu_upright()` so sánh bình phương để thang đo tự triệt tiêu:

```
cos²(tilt) = az² / (ax² + ay² + az²)      →      az² × TILT_COS2_DEN >= tổng × TILT_COS2_NUM
```

| Hằng số | Giá trị | Ghi chú |
| --- | --- | --- |
| `TILT_LIMIT_DEG` | 30 | `[DO]` |
| `TILT_COS2_NUM` / `TILT_COS2_DEN` | 3 / 4 | `cos²(30°) = 3/4`. **Không** dẫn xuất tự động từ `TILT_LIMIT_DEG` lúc biên dịch — đổi thì phải đổi cả hai |

`az <= 0` (xe lật ngửa) và `total == 0` (dữ liệu vô nghĩa) đều trả `false`.
`[DO]` — giả định `accel_raw[2]` là trục thẳng đứng và đọc dương khi xe nằm phẳng;
phải xác nhận hướng lắp trước khi tin vào phép kiểm tra này.

## 7. Ranh giới NVIC priority

```mermaid
flowchart TB
    HI["Số 0..4 — CẤM gọi mọi API FreeRTOS<br/>hiện đang TRỐNG"]
    BND["configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5<br/>RANH GIỚI"]
    LO["Số 5..15 — được gọi ...FromISR()<br/>5 Echo và nút bấm · 6 I2C · 7 UART · 15 SysTick"]
    HI --> BND --> LO
```

- CMSIS nhận **số chưa dịch bit**; FreeRTOS dịch lên 4 bit cao qua
  `configMAX_SYSCALL_INTERRUPT_PRIORITY`.
- `main()` gọi `HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4)` — toàn bộ 4 bit
  là preempt priority, **không có subpriority**.
- `_Static_assert(configPRIO_BITS == __NVIC_PRIO_BITS, ...)` bắt lỗi lệch ngay lúc biên dịch.
- Vi phạm ranh giới **không crash ngay** — nó làm hỏng kernel list ngẫu nhiên.
  `configASSERT` phải luôn bật vì chính nó kích hoạt `vPortValidateInterruptPriority()`.

## 8. Ngân sách RAM — kiểm tra ngay lúc biên dịch

`main.c` không chỉ ghi ngân sách vào comment mà **ép linker/compiler kiểm tra**:

```c
_Static_assert(
    sizeof(task_stacks) + sizeof(idle_stack) + sizeof(timer_stack) +
    sizeof(task_controls) + sizeof(idle_control) + sizeof(timer_control) +
    2U * sizeof(StaticQueue_t) + sizeof(sample_t) + sizeof(imu_sample_t) +
    sizeof(StaticEventGroup_t) + APP_KERNEL_DATA_RESERVE_BYTES + APP_MSP_RESERVE_BYTES
    < APP_SRAM_BYTES, "Static RAM planning budget exceeded");
```

| Hạng mục | Kích thước |
| --- | --- |
| 5 task stack × 256 word | 5 120 B |
| Idle + Timer stack, 2 × 128 word | 1 024 B |
| 7 × `StaticTask_t` | — `sizeof` kiểm tra trong `main.c` |
| 2 mailbox + `sample_t` + `imu_sample_t` | — |
| `StaticEventGroup_t` | — |
| Dự phòng kernel/HAL/data/alignment | 4 096 B |
| Dự phòng MSP (ISR stack) | 1 024 B |
| **Trần** | **20 480 B** |

Dự phòng là **khoản trù tính, không thay thế việc đo**. `CMakeLists.txt` bật
`-fstack-usage` để sinh file `.su` đối chiếu, và `-Wl,--print-memory-usage` in
%FLASH/%RAM sau mỗi lần link. Đo stack thật bằng `uxTaskGetStackHighWaterMark()`.

## 9. Quy tắc viết ISR

| Phải | Không được |
| --- | --- |
| Capture event / chụp timestamp / set bit, rồi `portYIELD_FROM_ISR()` | Chạy thuật toán né vật cản |
| Priority number 5..15 nếu có gọi API kernel | Gọi API kernel bản **không** `FromISR` |
| Giữ thời gian chạy ở mức µs | Chờ I2C, chờ UART, in log |

Skeleton hiện dùng macro `UNIMPLEMENTED_IRQ(name)` để mọi IRQ chưa có chủ sở hữu
rơi vào `configASSERT(0)`. Đây là **bẫy chẩn đoán lúc bring-up**, không phải cách
xử lý fault cuối cùng — phải thay bằng handler thật trước khi bật ngoại vi tương ứng.
