#!/usr/bin/env bash
# Kiem tra tinh cac rang buoc trong CLAUDE.md. Chay sau khi build:
#   ./tools/check_constraints.sh [duong-dan-toi-elf]
set -uo pipefail
cd "$(dirname "$0")/.."
ELF="${1:-build/debug/obstacle_car.elf}"
NM=arm-none-eabi-nm
fail=0
ok(){ printf '  \033[32mOK\033[0m   %s\n' "$1"; }
bad(){ printf '  \033[31mLOI\033[0m  %s\n' "$1"; fail=1; }

echo "== Nguon =="
if grep -rn --include=*.c --include=*.h -w 'HAL_Delay' App Core Drivers Config 2>/dev/null; then
  bad "co HAL_Delay() — dung vTaskDelay()"; else ok "khong co HAL_Delay()"; fi
if grep -rn --include=*.c --include=*.h -wE 'printf|sprintf|snprintf|vsnprintf' App Core Drivers Config 2>/dev/null; then
  bad "co printf cua newlib — dung formatter rieng cua uart_debug"; else ok "khong co printf/sprintf/snprintf"; fi
if grep -rn --include=*.c --include=*.h -wE 'malloc|calloc|realloc|free|pvPortMalloc|vPortFree' App Core Drivers Config 2>/dev/null; then
  bad "co cap phat dong"; else ok "khong co cap phat dong"; fi

echo "== Build =="
if [ ! -f "$ELF" ]; then
  echo "  (bo qua: chua co $ELF — chay 'cmake --build --preset debug' truoc)"
else
  if $NM "$ELF" | grep -qiE ' [TtWwBbDd] .*(pvPortMalloc|printf|sprintf)'; then
    bad "elf co chua pvPortMalloc/printf"; else ok "elf sach: khong co pvPortMalloc/printf"; fi
  if find ThirdParty -name 'heap_*.c' | grep -q .; then
    bad "co heap_*.c trong ThirdParty"; else ok "khong co heap_*.c"; fi
fi
echo
[ $fail -eq 0 ] && echo "Tat ca dat." || echo "CO VI PHAM."
exit $fail
