# ==============================================================================
#  Xe tranh vat can — STM32F103C8T6 + FreeRTOS
#  Moc A: dung duong build toi thieu (CMSIS + HAL + startup + linker script).
#
#  Build:
#    cmake --preset debug && cmake --build --preset debug
#  hoac:
#    cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
#    cmake --build build
# ==============================================================================
cmake_minimum_required(VERSION 3.22)

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR
    "Thieu toolchain file. Chay lai voi:\n"
    "  cmake --preset debug\n"
    "hoac -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake")
endif()

project(obstacle_car C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(TARGET_ELF ${PROJECT_NAME}.elf)
set(TP ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F103C8TX_FLASH.ld)

# ------------------------------------------------------------------------------
# Co chung cho moi target
# ------------------------------------------------------------------------------
add_library(mcu_flags INTERFACE)
target_compile_definitions(mcu_flags INTERFACE
  STM32F103xB
  USE_HAL_DRIVER
)
target_compile_options(mcu_flags INTERFACE
  ${STM32_CPU_FLAGS}
  -ffunction-sections -fdata-sections   # cho phep --gc-sections vut ham khong dung
  -fno-common
  -fstack-usage                          # sinh .su: doi chieu voi RAM budget o app_config.h
  $<$<CONFIG:Debug>:-Og;-g3;-gdwarf-4>
  $<$<CONFIG:Release>:-Os;-g>
)
target_link_options(mcu_flags INTERFACE ${STM32_CPU_FLAGS})

# ------------------------------------------------------------------------------
# CMSIS (Apache-2.0) + HAL (BSD-3-Clause) — code cua ben thu ba.
# De SYSTEM include va tat canh bao: warning cua ST khong phai viec cua du an nay,
# nhung code cua minh thi van bi soi bang -Wall -Wextra -Wpedantic o duoi.
# ------------------------------------------------------------------------------
add_library(stm32_hal STATIC
  ${TP}/CMSIS/Device/ST/STM32F1xx/Source/system_stm32f1xx.c
  ${TP}/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c
  ${TP}/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c
  ${TP}/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c
  ${TP}/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c
  ${TP}/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c
  ${TP}/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c
  ${TP}/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c
  ${TP}/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c
  ${TP}/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c
  ${TP}/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c
  ${TP}/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c
  ${TP}/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c
)
target_include_directories(stm32_hal SYSTEM PUBLIC
  ${TP}/CMSIS/Core/Include
  ${TP}/CMSIS/Device/ST/STM32F1xx/Include
  ${TP}/STM32F1xx_HAL_Driver/Inc
)
# stm32f1xx_hal_conf.h nam trong Core/Inc, khong phai trong ThirdParty
target_include_directories(stm32_hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc)
target_compile_options(stm32_hal PRIVATE -w)
target_link_libraries(stm32_hal PUBLIC mcu_flags)

# ------------------------------------------------------------------------------
# FreeRTOS kernel (MIT), port GCC/ARM_CM3.
#
# Chu y: KHONG co portable/MemMang/heap_*.c trong danh sach nguon. Do la cach
# re nhat de bao dam configSUPPORT_DYNAMIC_ALLOCATION = 0 — pvPortMalloc khong
# ton tai o tang link, goi nham thi loi link chu khong am tham chay duoc.
#
# Kernel chi duoc build khi da co Core/Inc/FreeRTOSConfig.h. File do thuoc ve
# tac gia (xem CLAUDE.md), chua co trong repo nen moc A2 tam thoi bo qua.
# ------------------------------------------------------------------------------
set(FREERTOS_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/FreeRTOSConfig.h)
if(EXISTS ${FREERTOS_CONFIG})
  set(HAVE_FREERTOS TRUE)
  add_library(freertos_kernel STATIC
    ${TP}/FreeRTOS-Kernel/croutine.c
    ${TP}/FreeRTOS-Kernel/event_groups.c
    ${TP}/FreeRTOS-Kernel/list.c
    ${TP}/FreeRTOS-Kernel/queue.c
    ${TP}/FreeRTOS-Kernel/stream_buffer.c
    ${TP}/FreeRTOS-Kernel/tasks.c
    ${TP}/FreeRTOS-Kernel/timers.c
    ${TP}/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c
  )
  target_include_directories(freertos_kernel SYSTEM PUBLIC
    ${TP}/FreeRTOS-Kernel/include
    ${TP}/FreeRTOS-Kernel/portable/GCC/ARM_CM3
  )
  target_include_directories(freertos_kernel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc)
  target_compile_options(freertos_kernel PRIVATE -w)
  target_link_libraries(freertos_kernel PUBLIC mcu_flags)
else()
  set(HAVE_FREERTOS FALSE)
  message(STATUS "FreeRTOS: BO QUA — chua co Core/Inc/FreeRTOSConfig.h")
endif()

# ------------------------------------------------------------------------------
# Firmware
# ------------------------------------------------------------------------------
add_executable(${TARGET_ELF}
  ${TP}/CMSIS/Device/ST/STM32F1xx/Source/gcc/startup_stm32f103xb.s
  Core/Src/main.c
)
target_include_directories(${TARGET_ELF} PRIVATE
  Config
  Core/Inc
  Drivers/MCAL/Inc
  Drivers/Devices/Inc
  App/Inc
)
target_compile_options(${TARGET_ELF} PRIVATE
  $<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wdouble-promotion>
)
target_link_libraries(${TARGET_ELF} PRIVATE mcu_flags stm32_hal)
if(HAVE_FREERTOS)
  target_link_libraries(${TARGET_ELF} PRIVATE freertos_kernel)
endif()

target_link_options(${TARGET_ELF} PRIVATE
  -T${LINKER_SCRIPT}
  --specs=nano.specs        # newlib-nano: nho hon, khong keo printf day du
  # KHONG dung --specs=nosys.specs: khong co libnosys thi printf/malloc/file-IO
  # cua newlib se LOI LINK ngay thay vi am tham lot qua. Luat "cam printf" trong
  # CLAUDE.md nho vay duoc linker cuong che, khong phu thuoc vao review.
  -Wl,--gc-sections
  -Wl,-Map=${TARGET_ELF}.map,--cref
  -Wl,--print-memory-usage
  -Wl,--no-warn-rwx-segments
)
set_target_properties(${TARGET_ELF} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

# .hex / .bin de nap, va bang kich thuoc sau moi lan build
add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${TARGET_ELF}> ${PROJECT_NAME}.hex
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_ELF}> ${PROJECT_NAME}.bin
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_ELF}>
  COMMENT "Sinh .hex/.bin va do kich thuoc"
)
