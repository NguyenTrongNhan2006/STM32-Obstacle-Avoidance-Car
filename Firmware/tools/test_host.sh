#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/host-tests
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
  -ITests/Host/Inc -IConfig -IApp/Inc -IDrivers/Devices/Inc -IDrivers/MCAL/Inc \
  Tests/Host/test_safety_imu_buzzer.c \
  App/Src/safety_monitor.c Drivers/Devices/Src/mpu6050.c \
  Drivers/Devices/Src/buzzer.c Drivers/MCAL/Src/uart_debug.c \
  -o build/host-tests/safety_imu_buzzer
./build/host-tests/safety_imu_buzzer
