# Toolchain file cho arm-none-eabi-gcc, target Cortex-M3 (STM32F103C8T6).
# Dung qua: cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
# hoac qua CMakePresets.json.

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# BAT BUOC. Mac dinh CMake kiem tra compiler bang cach build mot executable,
# viec do can linker script + startup nen chac chan that bai o day. Doi sang
# static library thi CMake chi kiem tra buoc compile.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER   arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_AR           arm-none-eabi-ar)
set(CMAKE_OBJCOPY      arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP      arm-none-eabi-objdump)
set(CMAKE_SIZE         arm-none-eabi-size)
set(CMAKE_NM           arm-none-eabi-nm)

set(STM32_CPU_FLAGS -mcpu=cortex-m3 -mthumb)

# Tim chuong trinh o host, con header/thu vien thi chi tim trong sysroot cua
# toolchain — tranh keo nham /usr/include cua may host vao ban firmware.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
