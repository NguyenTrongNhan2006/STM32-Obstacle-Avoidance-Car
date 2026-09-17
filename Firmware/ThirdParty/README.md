# ThirdParty

Code của bên thứ ba, vendor thẳng vào repo. Tách riêng khỏi `Firmware/Drivers/`
(là code tự viết: MCAL + Devices) để ranh giới bản quyền rõ ràng — `LICENSE` ở
gốc repo là all-rights-reserved và ghi *"Third-party materials added later remain
subject to their original licenses"*.

Không sửa file trong thư mục này. Cần thay đổi hành vi thì viết wrapper ở
`Drivers/MCAL/`. Muốn nâng version thì thay cả cây và ghi lại bảng dưới.

| Thành phần | Upstream | Tag | Commit | License | File license |
| --- | --- | --- | --- | --- | --- |
| CMSIS Core | `STMicroelectronics/cmsis_core` | `v5.9.0` | `1a2f783` | Apache-2.0 | `CMSIS/Core/LICENSE.txt` ¹ |
| CMSIS Device F1 | `STMicroelectronics/cmsis_device_f1` | `v4.3.5` | `8a76309` | Apache-2.0 | `CMSIS/Device/ST/STM32F1xx/LICENSE.md` |
| STM32F1xx HAL | `STMicroelectronics/stm32f1xx_hal_driver` | `v1.1.10` | `77fbb30` | BSD-3-Clause | `STM32F1xx_HAL_Driver/LICENSE.md` |
| FreeRTOS Kernel | `FreeRTOS/FreeRTOS-Kernel` | `V11.3.1` | `3a22924` | MIT | `FreeRTOS-Kernel/LICENSE.md` |

¹ Bản mirror `cmsis_core` của ST **không kèm file license** ở bất kỳ đâu trong cây;
license chỉ nằm ở dòng `SPDX-License-Identifier: Apache-2.0` trong header từng file.
Apache-2.0 §4(a) yêu cầu kèm bản license khi phân phối lại, nên `LICENSE.txt` ở đây
là bản Apache-2.0 sao từ `cmsis_device_f1/License.md` (cùng license, cùng nhà phát hành).

## Đã lược bớt những gì

Không lấy nguyên cây upstream, chỉ lấy phần dùng đến:

- **CMSIS Core** — 5 header cho Cortex-M3 (`core_cm3.h`, `cmsis_compiler.h`,
  `cmsis_gcc.h`, `cmsis_version.h`, `mpu_armv7.h`). Bỏ DSP, NN, RTOS, Documentation
  (~100 MB).
- **CMSIS Device F1** — chỉ 3 header medium-density (`stm32f1xx.h`,
  `stm32f103xb.h`, `system_stm32f1xx.h`) + `system_stm32f1xx.c` +
  `startup_stm32f103xb.s`. Bỏ header của 11 biến thể F1 khác.
- **FreeRTOS** — kernel + `portable/GCC/ARM_CM3`. **Cố tình không lấy
  `portable/MemMang/heap_*.c`.** Đó là cách rẻ nhất để ép
  `configSUPPORT_DYNAMIC_ALLOCATION = 0`: `pvPortMalloc` không tồn tại ở tầng
  link, gọi nhầm thì lỗi link chứ không âm thầm chạy được.
- **HAL** — lấy trọn `Inc/` + `Src/`, nhưng `CMakeLists.txt` chỉ compile 12 file
  ứng với 8 module bật trong `Core/Inc/stm32f1xx_hal_conf.h`. Mốc C sẽ thêm TIM,
  mốc F thêm I2C — chỉ cần sửa `CMakeLists.txt`, không phải vendor lại.

## Linker script: không dùng bản của ST

`cmsis_device_f1` có sẵn `Source/Templates/gcc/linker/STM32F103XB_FLASH.ld`
nhưng **không được đưa vào repo**. File đó mang copyright **Ac6**, ghi rõ:

> Distribution of this file (unmodified or modified) is not permitted.

Nó cũng khai 128 KB flash (biến thể `xB`), sai với C8T6 vốn chỉ có 64 KB.

Thay vào đó `Firmware/linker/STM32F103C8TX_FLASH.ld` là script tự viết cho dự án,
64 KB flash / 20 KB SRAM, xuất đúng bộ symbol mà `startup_stm32f103xb.s` cần
(`_estack`, `_sidata`, `_sdata`, `_edata`, `_sbss`, `_ebss`).
