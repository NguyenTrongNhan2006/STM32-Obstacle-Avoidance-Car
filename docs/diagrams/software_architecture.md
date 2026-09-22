# Kiến trúc phần mềm phân tầng

[◀ Về README chính](../../README.md) · [Sơ đồ phần cứng](hardware_block_diagram.md) · [FSM điều khiển](fsm_control_flow.md) · [Concurrency](freertos_concurrency.md)

Quy tắc phụ thuộc lấy từ `CLAUDE.md`: **Core → App → Devices → MCAL → Config**.

---

## 1. Năm tầng

```mermaid
flowchart TB
    subgraph L5["① CORE — boot, clock, vector table, tạo task"]
        direction LR
        MAIN["main.c<br/>SystemClock_Config, 5 static task"]
        IT["stm32f1xx_it.c<br/>SysTick wrapper, IRQ trap"]
    end

    subgraph L4["② APPLICATION — chính sách, không chạm thanh ghi"]
        direction LR
        RC["robot_car<br/>ranh giới lệnh motor duy nhất"]
        OA["obstacle_avoidance<br/>FSM 7 state"]
        SM["sensor_manager<br/>2 mailbox bản copy"]
        SF["safety_monitor<br/>egSafety, chốt fault"]
    end

    subgraph L3["③ DEVICE DRIVERS — biết linh kiện, không biết chính sách"]
        direction LR
        MOT["motor_tb6612<br/>motor_apply()"]
        USD["ultrasonic_hcsr04"]
        IMUD["mpu6050"]
        BTND["button"]
        LEDD["status_led"]
        BUZD["buzzer"]
    end

    subgraph L2["④ MCAL — biết ngoại vi STM32, không biết linh kiện"]
        direction LR
        GPIO["gpio"]
        PWM["pwm"]
        I2C["i2c"]
        EXTI["exti"]
        TB["timebase"]
        UARTD["uart_debug"]
    end

    subgraph L1["⑤ NỀN — CMSIS + STM32 HAL + FreeRTOS"]
        direction LR
        HAL["STM32F1xx_HAL_Driver<br/>8 module được bật"]
        CMSIS["CMSIS Core M3"]
        RTOS["FreeRTOS-Kernel<br/>port GCC/ARM_CM3"]
        STARTUP["startup_stm32f103xb.s"]
    end

    CFG["CONFIG — board_config.h · app_config.h · project_types.h<br/>chỉ chứa hằng số và kiểu, không có code"]

    L5 ==> L4
    L4 ==> L3
    L3 ==> L2
    L2 ==> L1
    L5 ==> L1
    L4 -.->|"chỉ đọc hằng số"| CFG
    L3 -.->|"chỉ đọc hằng số"| CFG
    L2 -.->|"chỉ đọc hằng số"| CFG
```

## 2. Dependency Rule

> **Tầng trên gọi xuống tầng dưới. Tầng dưới KHÔNG BAO GIỜ include ngược lên.**
> Cụ thể trong repo này: **không driver nào được `#include` file của App.**

| Tầng | ĐƯỢC include | **CẤM** include |
| --- | --- | --- |
| ① Core | `*.h` của App, MCAL, Config, FreeRTOS | — |
| ② App | `*.h` của Devices, Config, FreeRTOS | `stm32f1xx_hal_*.h`, thanh ghi trực tiếp |
| ③ Devices | `*.h` của MCAL, Config | **`robot_car.h`, `safety_monitor.h`, `sensor_manager.h`, `obstacle_avoidance.h`** |
| ④ MCAL | `stm32f1xx_hal.h`, `board_config.h` | `*.h` của Devices và App |
| ⑤ Nền | chỉ chính nó | mọi thứ của dự án |

### Cách giải quyết mâu thuẫn: `motor_tb6612` cần biết trạng thái an toàn

`motor_apply()` (tầng ③) phải kiểm tra `egSafety` — nhưng `egSafety` thuộc
`safety_monitor` ở tầng ②. Include ngược lên là vi phạm. Giải pháp là **tiêm
con trỏ hàm lúc init**, driver không bao giờ biết tên module cung cấp nó:

```c
/* motor_tb6612.h — tầng ③ */
typedef struct { bool (*is_safe)(void); } motor_config_t;

/* robot_car.c — tầng ② nối hai đầu lại */
const motor_config_t config = { .is_safe = safety_is_clear_to_run };
return motor_init(&config);
```

```mermaid
flowchart LR
    SF["safety_monitor (tầng ②)<br/>sở hữu egSafety"] -->|"hàm safety_is_clear_to_run"| RC
    RC["robot_car (tầng ②)<br/>nối dây lúc init"] -->|"motor_init(&amp;config)<br/>tiêm con trỏ hàm"| MOT
    MOT["motor_tb6612 (tầng ③)<br/>chỉ thấy bool (*is_safe)(void)"] -->|"gọi ngược qua con trỏ"| SF
    MOT -->|"gpio_emergency_stop()"| GP["gpio (tầng ④)"]
```

Ba cơ chế hợp lệ để tầng dưới đưa dữ liệu lên trên, không cái nào là include ngược:

| Cơ chế | Ví dụ trong repo |
| --- | --- |
| Giá trị trả về / con trỏ ra | `button_read(&pressed)`, `sensor_manager_get_latest(&range, &imu)` |
| Callback tiêm lúc init | `motor_config_t.is_safe` |
| Đối tượng kernel | `qRangeMailbox`, `qImuMailbox`, `egSafety` |

## 3. Ranh giới lệnh motor

```mermaid
flowchart TB
    OA2["obstacle_avoidance_update()<br/>trả về motor_cmd_t request"] --> RC2
    RC2["robot_car_update()<br/>ĐÚNG MỘT lần gọi motor_apply<br/>trên MỌI nhánh"] --> MA
    MA["motor_apply(command, speed)"] --> STBY["gpio_emergency_stop()<br/>hạ STBY ngay đầu hàm"]
    STBY --> V{"tham số hợp lệ ?<br/>command &lt;= TURN_RIGHT<br/>speed &lt;= 100"}
    V -->|"không"| ERR["STATUS_ERROR<br/>bridge vẫn tắt"]
    V -->|"có"| S{"safety_check != NULL<br/>và safety_check() == true ?"}
    S -->|"không"| NR["STATUS_NOT_READY<br/>bridge vẫn tắt"]
    S -->|"có"| TODO["IMPLEMENT: arbitration + đổi chiều an toàn<br/>skeleton vẫn trả NOT_READY"]
```

Ba lớp bảo vệ, độc lập nhau:

1. **`gpio_emergency_stop()` chạy vô điều kiện** ở dòng đầu `motor_apply()` và
   `motor_init()` — kể cả khi tham số sai hoặc predicate chưa được tiêm.
2. **Predicate an toàn** được kiểm tra lại **bên trong** driver, không tin vào
   việc App đã kiểm tra trước đó.
3. **`robot_car_update()` gọi `motor_apply()` đúng một lần trên mọi nhánh** —
   kể cả khi `sensor_manager_get_latest()` thất bại, khi đó `request` giữ
   nguyên `MOTOR_STOP` chứ không tái sử dụng lệnh cũ.

> **Trạng thái skeleton:** `motor_apply()` **luôn** trả `STATUS_NOT_READY` và
> không bao giờ bật bridge, kể cả khi predicate trả `true`. Bảng chân lý
> brake/coast phải chốt trước khi cho phép STBY lên mức cao.

## 4. Ánh xạ tầng ↔ thư mục

| Tầng | Thư mục | Trạng thái |
| --- | --- | --- |
| ① Core | `Firmware/Core/{Inc,Src}/` | ✅ boot, clock 72 MHz, 5 task, SysTick wrapper |
| ② App | `Firmware/App/{Inc,Src}/` | ⚠️ `obstacle_avoidance` + `safety_monitor` + `robot_car` đã có logic; `sensor_manager_update()` còn `NOT_READY` |
| ③ Devices | `Firmware/Drivers/Devices/{Inc,Src}/` | ⚠️ API đã chốt, phần lớn thân hàm còn `IMPLEMENT` |
| ④ MCAL | `Firmware/Drivers/MCAL/{Inc,Src}/` | ⚠️ `gpio` + `timebase` + `uart_debug` chạy; `pwm`/`i2c`/`exti` còn stub |
| ⑤ Nền | `Firmware/ThirdParty/` | ✅ CMSIS, HAL, FreeRTOS-Kernel |
| Config | `Firmware/Config/`, `Firmware/Core/Inc/` | ✅ `board_config.h`, `app_config.h`, `project_types.h`, `FreeRTOSConfig.h`, `stm32f1xx_hal_conf.h` |

## 5. Ràng buộc xuyên tầng

1. **Static allocation.** `configSUPPORT_DYNAMIC_ALLOCATION = 0`; danh sách nguồn
   trong `CMakeLists.txt` **không có** `portable/MemMang/heap_*.c`. Gọi
   `pvPortMalloc` sẽ là lỗi link, không phải lỗi runtime.
2. **Không `--specs=nosys.specs`.** Chủ đích: thiếu libnosys thì `printf`, `malloc`
   và file-IO của newlib **lỗi link ngay**, nên luật "cấm printf" được linker
   cưỡng chế chứ không phụ thuộc vào review.
3. **Cấm `HAL_Delay()`** trong source dự án — dùng `vTaskDelay()` / `xTaskDelayUntil()`.
4. **Stub phải trả `NOT_READY`**, không báo `OK` giả cho phép đo hay actuation.
5. Giá trị `[DO]` là giả định cần đo; `[CO DINH]` là ràng buộc đã có nguồn.
   **Không biến giả định thành số đo trong tài liệu.**

## 6. HAL module đang bật

`CORTEX` · `DMA` · `EXTI` · `FLASH` · `GPIO` · `PWR` · `RCC` · `UART`

`TIM` bật ở **mốc C** (pwm/motor), `I2C` bật ở **mốc F** (mpu6050). Module không
bật thì file `.c` tương ứng không nằm trong `CMakeLists.txt` — tiết kiệm trong
64 KiB flash và không kéo theo code chưa được review.
