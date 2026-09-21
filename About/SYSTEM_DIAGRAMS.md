# Hồ sơ sơ đồ hệ thống

[Về mục lục](README.md) · [Vai trò firmware](FIRMWARE_GUIDE.md) · [Phần cứng](PROJECT_PLAN.md) · [Timeline Nhân–Hưng](TEAM_TIMELINE.md)

**Trạng thái: mẫu tài liệu — chưa có sơ đồ được chèn hoặc xác nhận.** Nhân và Hưng điền dần khi tự thiết kế, triển khai và đo kiểm. Các nội dung cần có bên dưới là hướng dẫn vẽ/giải thích, không khẳng định chức năng đã hoạt động.

## 1. Cách chèn hình và quản lý phiên bản

1. Tạo các thư mục cần dùng trong `Images/` ở gốc repo: `Architecture/`, `Schematic/`, `FSM/`, `Flowchart/`, `Sequence/`, `Timing/`.
2. Vẽ sơ đồ bằng công cụ hai bạn đang dùng. Lưu cả file nguồn chỉnh sửa được và bản xuất PNG/SVG/PDF; ưu tiên PNG/SVG để xem trực tiếp trong Markdown.
3. Upload ảnh vào đúng thư mục. Tên file dùng chữ thường, không dấu, dấu gạch ngang; giữ đúng chữ hoa/thường của tên thư mục.
4. Copy dòng chèn ảnh trong khung Markdown của từng mục ra **ngoài khung code**, đặt ngay dưới tiêu đề mục đó. Chỉ làm sau khi ảnh đã tồn tại.
5. Điền phần giải thích và người review dưới hình, rồi cập nhật bảng trạng thái ở mục 2.

Ví dụ khi đã có ảnh `Images/Schematic/schematic-main.png`:

```markdown
![SCH-01 — Sơ đồ nguyên lý tổng thể](../Images/Schematic/schematic-main.png)

**Giải thích:** TODO — mô tả nguồn cấp, các khối chính và đường tín hiệu.
**Liên quan:** Firmware/Config/board_config.h.
**Phiên bản:** TODO | **Ngày:** TODO | **Người vẽ:** TODO | **Review:** TODO.
**Kiểm chứng:** TODO — chưa kiểm tra / đã đối chiếu / đã đo trên board.
```

Đường dẫn bắt đầu bằng `../Images/` vì tài liệu này nằm trong `About/`. Các dòng ảnh đang đặt trong khung code để GitHub không hiển thị hình lỗi khi chưa có file. Bỏ khung code sau khi chèn hình thật; không chỉ đổi chữ TODO mà quên thêm ảnh.

Với PDF, dùng link tải thay vì cú pháp ảnh. Có thể thay một ảnh bằng khối Mermaid nếu phù hợp; không bắt buộc vẽ cả hai phiên bản. File nguồn sơ đồ đặt cạnh ảnh xuất, ví dụ `schematic-main.kicad_sch` hoặc `fsm-car.drawio`; đây là ví dụ tên, chưa có sẵn trong repo.

Mỗi sơ đồ dùng mẫu chú thích chung ở trên. Phân biệt **thiết kế đề xuất** và **hành vi đã implement/đo được**. Giá trị chưa đo giữ nhãn `[DO]`; không sao chép ngưỡng placeholder thành số liệu thực nghiệm.

## 2. Danh mục và người phụ trách

| ID | Sơ đồ | Người vẽ chính | Người review | Trạng thái |
| --- | --- | --- | --- | --- |
| SYS-01 | Block Diagram toàn hệ thống | Nhân | Hưng | TODO |
| SCH-01 | Schematic tổng thể, nguồn, MCU, motor | Nhân | Hưng | TODO |
| SCH-02 | Schematic cảm biến, mức logic, buzzer | Hưng | Nhân | TODO |
| WIR-01 | Wiring Diagram và pinout thực tế | Nhân | Hưng | TODO |
| SW-01 | Kiến trúc firmware và Task/IPC | Nhân | Hưng | TODO |
| FSM-01 | Xe / obstacle_avoidance | Nhân | Hưng | TODO |
| FSM-02 | Safety / STOP / rearm | Nhân | Hưng | TODO |
| FSM-03 | HC-SR04 và sensor_manager | Hưng | Nhân | TODO |
| FSM-04 | MPU6050 và chất lượng mẫu | Hưng | Nhân | TODO |
| FSM-05 | Motor TB6612 | Nhân | Hưng | TODO |
| FSM-06 | Button / debounce | Nhân | Hưng | TODO |
| FSM-07 | Buzzer | Hưng | Nhân | TODO |
| FSM-08 | Status LED | Nhân | Hưng | TODO |
| FLW-01 | Boot và khởi tạo FreeRTOS | Nhân | Hưng | TODO |
| FLW-02 | Flowchart của năm Task | Theo owner Task | Người còn lại | TODO |
| SEQ-01 | Sequence đo sensor → quyết định → motor | Hưng | Nhân | TODO |
| TIM-01 | Timing Diagram đo Echo và đáp ứng STOP | Nhân | Hưng | TODO |
| TST-01 | Bố trí thử nghiệm và vị trí cảm biến | Hưng | Nhân | TODO |

Các sơ đồ phục vụ kế hoạch hiện có, không thêm tính năng hay kéo dài deadline 20/12/2026. Tuần 1–2 ưu tiên SYS/SCH/WIR; tuần 3–7 hoàn thiện SW/FSM/FLW/SEQ; khi đo kiểm bổ sung TIM/TST. Sơ đồ cập nhật theo tiến độ module, không cần hoàn thành toàn bộ trước khi bắt đầu bring-up.

## 3. Block Diagram — toàn hệ thống

**Mục đích:** cho người đọc thấy hệ thống có những khối nào và chúng trao đổi nguồn/tín hiệu ra sao.

```markdown
![SYS-01 — Block Diagram toàn hệ thống](../Images/Architecture/system-block.png)
```

**Cần thể hiện:** nguồn motor, nguồn logic, STM32F103C8T6, cầu H/hai motor, cảm biến khoảng cách phía trước, MPU6050 trên xe, nút START/STOP, LED, buzzer, UART và máy tính debug. Dùng kiểu nét hoặc màu khác nhau cho đường nguồn và dữ liệu, có chú giải.

**Giải thích cần điền:** TODO — chức năng mỗi khối; chiều luồng dữ liệu; khối đưa ra quyết định; khối thực thi lệnh; đường tắt motor khi lỗi; ranh giới giữa PC debug và firmware trên xe.

**Phạm vi:** không thêm IR trái/phải. TB6612 và tên HC-SR04 còn cần xác nhận linh kiện như PROJECT_PLAN; không biến sơ đồ đề xuất thành BOM đã chốt.

## 4. Schematic Diagram — sơ đồ nguyên lý điện

### SCH-01 — tổng thể, nguồn, MCU và motor

```markdown
![SCH-01 — Schematic nguồn, MCU và motor](../Images/Schematic/schematic-main.png)
```

**Cần thể hiện:** ký hiệu/tên linh kiện, net label, connector, mức nguồn, GND, ổn áp/decoupling, reset/clock/SWD, chân PWM/hướng/STBY của cầu H, đầu nối motor và linh kiện bảo vệ theo thiết kế đã chọn.

**Giải thích cần điền:** TODO — nguồn đi từ đâu đến đâu; lý do chọn nguồn/driver theo motor thực tế; trạng thái chân khi reset; cách STBY được giữ tắt; vị trí đo nguồn và tín hiệu điều khiển.

**Đối chiếu:** TODO — revision schematic, datasheet dùng để kiểm tra, pinout trong board_config, số đo điện áp/dòng. Wiring Diagram không thay thế Schematic.

### SCH-02 — sensor, giao tiếp và buzzer

```markdown
![SCH-02 — Schematic cảm biến và buzzer](../Images/Schematic/schematic-sensors.png)
```

**Cần thể hiện:** TRIG/ECHO và mạch chuyển mức phù hợp, I2C/pull-up/AD0 của MPU6050, nguồn module, nút, LED, mạch kích buzzer và mức logic UART.

**Giải thích cần điền:** TODO — mức điện áp ở hai phía từng kết nối; lý do cần chuyển mức Echo; loại buzzer và cách kích; hướng trục MPU6050; vị trí đầu nối để hạn chế cắm sai.

**Đối chiếu:** TODO — tên module thực tế, điểm nối GND, nguồn pull-up và kết quả kiểm tra trước khi cấp nguồn motor. Không tự giả định mọi chân MCU đều chịu được 5 V.

## 5. Wiring Diagram — sơ đồ đấu nối

```markdown
![WIR-01 — Đấu nối thực tế](../Images/Schematic/wiring-diagram.png)
```

**Mục đích:** giúp người khác lắp lại đúng xe từ board/module/connector thật, không chỉ đọc được nguyên lý.

**Giải thích cần điền:** TODO — tên đầu nối, số chân, màu dây nếu sử dụng, chiều nhìn connector, cực nguồn, vị trí sensor/motor. Ảnh breadboard hoặc ảnh dây cần có nhãn, không để người đọc đoán.

| Tín hiệu | Chân MCU | Đầu nối/chân module | Điện áp/cực tính | Schematic net | Đã kiểm tra |
| --- | --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO | Chưa |

Bảng pin dự kiến có trong PROJECT_PLAN và board_config; chỉ điền bảng này sau khi đối chiếu board thật. Khi đổi chân, cập nhật cả ba nơi trong cùng thay đổi.

## 6. Kiến trúc firmware và Task/IPC

```markdown
![SW-01 — Kiến trúc firmware và Task/IPC](../Images/Architecture/firmware-tasks.png)
```

**Cần thể hiện:** Core → App → Devices → MCAL → Config; năm Task tSafety/tSensor/tDecision/tLog/tBuzzer; hai mailbox range/IMU; egSafety; ownership timer/bus; nguồn sự kiện ISR. Có thể chia một hình thành hai phần “tầng phần mềm” và “luồng runtime” để tránh rối.

**Giải thích cần điền:** TODO — ai publish, ai peek; mẫu gồm status/timestamp gì; Task nào có quyền ghi UART; ai gọi motor_apply; callback kiểm tra safety đi qua đâu. Phân biệt mũi tên include/dependency với mũi tên dữ liệu runtime.

| Tài nguyên | Owner ghi/cấu hình | Bên đọc/sử dụng | Cơ chế trao đổi | Quy tắc lỗi |
| --- | --- | --- | --- | --- |
| Range/IMU mailbox | sensor_manager / tSensor | TODO | Queue static dài 1, copy/peek | TODO |
| egSafety | safety_monitor | TODO | Event group | TODO |
| Motor output | robot_car → motor_apply | TODO | Lệnh + safety predicate | TODO |
| UART runtime | tLog | TODO | TODO | TODO |

Sơ đồ hiện trạng phải ghi rõ Task chỉ delay, driver còn NOT_READY; sơ đồ mục tiêu phải được gắn nhãn “đề xuất”. Không vẽ mọi đường xử lý như đã chạy thật.

## 7. FSM từng khối

FSM mô tả **state và điều kiện chuyển state**; Flowchart mô tả **trình tự xử lý trong một lần thực thi**. Mỗi mũi tên FSM nên ghi `event [guard] / action`, kèm timeout nếu có. Không gọi mọi enum/pattern là state của một FSM độc lập.

Các FSM dưới đây là chỗ để hai bạn thiết kế. Riêng CAR_* đã có enum trong header; các state nội bộ của driver còn để TODO, chưa yêu cầu thêm vào code.

### FSM-01 — Xe / obstacle_avoidance

```markdown
![FSM-01 — Xe / obstacle_avoidance](../Images/FSM/fsm-car.png)
```

**Owner:** Nhân. **Module liên quan:** `robot_car, obstacle_avoidance`.

**Cần thể hiện:** Dùng CAR_IDLE, CAR_FORWARD, CAR_STOP, CAR_TURN_LEFT, CAR_TURN_RIGHT, CAR_CHECK, CAR_FAULT theo header. Vẽ START/STOP, guard dữ liệu hợp lệ, hysteresis khoảng cách, timeout quay và giới hạn retry. Không khẳng định bên trái/phải trống khi chỉ có sensor trước.

**Giải thích cần điền:** TODO — Vì sao chuyển state; lệnh motor tương ứng; STOP/fault ưu tiên ra sao; lúc nào được rearm; quãng thời gian TURN được kiểm tra bằng deadline thế nào.

| State hiện tại | Event / guard | Action / output | State tiếp theo | Timeout / lỗi |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |

### FSM-02 — Safety / STOP / rearm

```markdown
![FSM-02 — Safety / STOP / rearm](../Images/FSM/fsm-safety.png)
```

**Owner:** Nhân. **Module liên quan:** `safety_monitor`.

**Cần thể hiện:** Vẽ điều kiện khóa chạy, lỗi sensor/stale, lỗi nghiêng, STOP latch và rearm có chủ đích. Event bits có thể đồng thời tồn tại: dùng sơ đồ guard hoặc state machine mở rộng, không ép chúng thành các trạng thái loại trừ nhau.

**Giải thích cần điền:** TODO — Ai đặt/xóa fault; điều kiện nào cấm rearm; đường tắt motor khi tDecision trễ; phần nào đã đo và phần nào còn đề xuất.

| State hiện tại | Event / guard | Action / output | State tiếp theo | Timeout / lỗi |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |

### FSM-03 — HC-SR04 / sensor_manager

```markdown
![FSM-03 — HC-SR04 / sensor_manager](../Images/FSM/fsm-ultrasonic.png)
```

**Owner:** Hưng. **Module liên quan:** `ultrasonic_hcsr04, sensor_manager, timebase`.

**Cần thể hiện:** Thiết kế các pha chờ lượt đo, trigger, chờ cạnh Echo, hoàn tất và timeout; nêu cách tránh yêu cầu đo chồng nhau. Nhánh lỗi vẫn phải publish chất lượng mẫu phù hợp.

**Giải thích cần điền:** TODO — Thời điểm bắt đầu deadline; timestamp lấy tại đâu; phân biệt NOT_READY/TIMEOUT với khoảng cách hợp lệ; điều kiện cho phép lượt đo tiếp theo.

| State hiện tại | Event / guard | Action / output | State tiếp theo | Timeout / lỗi |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |

### FSM-04 — MPU6050

```markdown
![FSM-04 — MPU6050](../Images/FSM/fsm-mpu6050.png)
```

**Owner:** Hưng. **Module liên quan:** `mpu6050, sensor_manager`.

**Cần thể hiện:** Thiết kế khởi tạo/identity, hiệu chuẩn khi đứng yên, lấy mẫu, lỗi giao dịch và thử phục hồi có giới hạn. Chất lượng mẫu và trạng thái driver cần được phân biệt.

**Giải thích cần điền:** TODO — Điều kiện chấp nhận bias; hướng trục/đơn vị raw; hành vi khi xe đang chuyển động lúc hiệu chuẩn; cách lỗi I2C tác động đến safety.

| State hiện tại | Event / guard | Action / output | State tiếp theo | Timeout / lỗi |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |

### FSM-05 — Motor TB6612

```markdown
![FSM-05 — Motor TB6612](../Images/FSM/fsm-motor.png)
```

**Owner:** Nhân. **Module liên quan:** `motor_tb6612, pwm, gpio`.

**Cần thể hiện:** Thiết kế trạng thái bị inhibit, dừng, chạy và chuyển hướng. MOTOR_* là lệnh; tự xác định trạng thái nội bộ cần thiết thay vì ánh xạ máy móc mỗi lệnh thành một state.

**Giải thích cần điền:** TODO — STBY/PWM/hướng ở mỗi trạng thái; chính sách khi đổi hướng; sự khác nhau STOP/COAST/BRAKE sau khi đối chiếu driver thật; fault thắng lệnh chạy thế nào.

| State hiện tại | Event / guard | Action / output | State tiếp theo | Timeout / lỗi |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |

### FSM-06 — Button / debounce

```markdown
![FSM-06 — Button / debounce](../Images/FSM/fsm-button.png)
```

**Owner:** Nhân. **Module liên quan:** `button, exti`.

**Cần thể hiện:** Thiết kế xử lý nhấn/nhả, lọc dội theo thời gian và sinh event START/STOP. ISR chỉ báo sự kiện ban đầu; giải thích việc xử lý tiếp ở Task.

**Giải thích cần điền:** TODO — Một lần nhấn tạo bao nhiêu event; nút giữ lâu xử lý thế nào; vì sao không dùng chung thao tác xóa fault với mọi cạnh tín hiệu.

| State hiện tại | Event / guard | Action / output | State tiếp theo | Timeout / lỗi |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |

### FSM-07 — Buzzer

```markdown
![FSM-07 — Buzzer](../Images/FSM/fsm-buzzer.png)
```

**Owner:** Hưng. **Module liên quan:** `buzzer / tBuzzer`.

**Cần thể hiện:** Thiết kế pattern SILENT/START/OBSTACLE/FAULT và các pha bật/tắt theo deadline. Nếu chỉ cần bộ phát pattern đơn giản, ghi rõ thay vì dựng FSM phức tạp.

**Giải thích cần điền:** TODO — Ưu tiên pattern khi nhiều sự kiện đồng thời; có ngắt pattern đang chạy hay không; thời lượng cần đo/chốt và trạng thái sau khi pattern kết thúc.

| State hiện tại | Event / guard | Action / output | State tiếp theo | Timeout / lỗi |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |

### FSM-08 — Status LED

```markdown
![FSM-08 — Status LED](../Images/FSM/fsm-status-led.png)
```

**Owner:** Nhân. **Module liên quan:** `status_led`.

**Cần thể hiện:** Mô tả IDLE/RUNNING/FAULT và quy tắc đổi/nháy. Có thể dùng bảng pattern nếu không cần state machine riêng; không bắt buộc tạo thêm Task.

**Giải thích cần điền:** TODO — Ý nghĩa từng chỉ thị; LED active-low theo board thực tế; đồng bộ với state xe/fault thế nào; ai cập nhật LED.

| State hiện tại | Event / guard | Action / output | State tiếp theo | Timeout / lỗi |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |

## 8. Flowchart — luồng xử lý

### FLW-01 — boot và khởi tạo

```markdown
![FLW-01 — Boot và khởi tạo FreeRTOS](../Images/Flowchart/boot-flowchart.png)
```

**Cần thể hiện:** thứ tự thực tế trong main.c: HAL/grouping, GPIO an toàn, clock, timebase/UART, App/static objects, tạo Task, start scheduler. Thể hiện nhánh init thất bại/assert và trạng thái motor trong bring-up.

**Giải thích cần điền:** TODO — bước nào phải hoàn tất trước bước nào; tại sao motor phải bị inhibit trước khi vận hành; điểm debug quan trọng; nhánh dừng khi scheduler không khởi động.

### FLW-02 — một vòng xử lý của từng Task

Chèn từng hình riêng để dễ đọc:

```markdown
![FLW-02A — tSafety](../Images/Flowchart/task-safety.png)
![FLW-02B — tSensor](../Images/Flowchart/task-sensor.png)
![FLW-02C — tDecision](../Images/Flowchart/task-decision.png)
![FLW-02D — tLog](../Images/Flowchart/task-log.png)
![FLW-02E — tBuzzer](../Images/Flowchart/task-buzzer.png)
```

| Task / owner | Nội dung cần vẽ | Giải thích cần điền |
| --- | --- | --- |
| tSafety / Nhân | Input STOP + chất lượng mẫu → kiểm tra/latch fault → inhibit/rearm theo thiết kế → chờ lượt kế | TODO — ưu tiên lỗi, deadline đáp ứng và cơ chế shutdown |
| tSensor / Hưng | Kiểm tra deadline riêng từng sensor → transaction/capture → status/timestamp → overwrite mailbox → chờ | TODO — tần suất khác nhau, timeout và tránh một sensor chặn sensor còn lại |
| tDecision / Nhân | Peek mẫu → kiểm tra điều kiện → FSM → motor_apply → chờ lượt kế | TODO — deadline state; kiểm tra safety tại output boundary |
| tLog / Hưng | Lấy snapshot → tạo bản ghi giới hạn → UART timeout → chờ | TODO — một writer, giới hạn dữ liệu và cách xử lý log không gửi được |
| tBuzzer / Hưng | Đọc sự kiện/pattern → xét deadline pha → cập nhật output → chờ | TODO — ưu tiên pattern và bảo đảm nhường CPU |

Các hàng mô tả **luồng mục tiêu cần tự triển khai**; source Task hiện tại chỉ delay. Nhánh “chờ lượt kế” không đồng nghĩa chặn cả thời gian TURN hoặc beep dài.

## 9. Sequence Diagram — thứ tự tương tác giữa các khối

```markdown
![SEQ-01 — Từ lấy mẫu đến điều khiển motor](../Images/Sequence/sensor-to-motor.png)
```

**Cần thể hiện:** sensor/ISR (nếu dùng) → tSensor → mailbox; tSafety và tDecision đọc bản copy; robot_car gọi motor_apply; driver kiểm tra predicate an toàn. Vẽ thêm nhánh mẫu lỗi hoặc quá hạn.

**Giải thích cần điền:** TODO — bên phát/bên nhận, thao tác nào đồng bộ hoặc bất đồng bộ, thời điểm publish, lý do hai Task dùng peek, thứ tự khi fault xuất hiện giữa lúc quyết định và xuất lệnh.

Lưu ý callback kiểm tra safety không tự chứng minh motor sẽ dừng khi tDecision treo; sơ đồ phải chỉ rõ đường inhibit bổ sung khi thiết kế và kiểm chứng phần đó.

## 10. Timing Diagram — đo thời gian và chứng minh đáp ứng

```markdown
![TIM-01A — Timing đo khoảng cách](../Images/Timing/ultrasonic-timing.png)
![TIM-01B — Timing STOP tới motor output](../Images/Timing/stop-response.png)
```

**Cần thể hiện:** trục thời gian có đơn vị, TRIG, ECHO, cạnh capture, deadline/timeout, thời điểm publish; hình riêng cho nút STOP/event, Task xử lý, STBY/PWM và trạng thái motor.

**Giải thích cần điền:** TODO — mốc bắt đầu/kết thúc phép đo; latency tệ nhất; giới hạn đã chốt; ảnh scope/logic analyzer nếu đã thử. Nhãn “đề xuất” cho hình lý tưởng và “đo thực tế” cho waveform thu được.

| Đại lượng | Giá trị thiết kế [DO] | Giá trị đo / min–max | Điều kiện tải/nguồn | Bằng chứng |
| --- | --- | --- | --- | --- |
| Chu kỳ lấy mẫu khoảng cách | TODO | Chưa đo | TODO | TODO |
| Chu kỳ lấy mẫu IMU | TODO | Chưa đo | TODO | TODO |
| STOP → STBY/PWM tắt | TODO | Chưa đo | TODO | TODO |
| Tuổi mẫu khi tDecision đọc | TODO | Chưa đo | TODO | TODO |
| Thời gian UART chiếm CPU | TODO | Chưa đo | TODO | TODO |

Không dùng số stack/RAM hoặc “build PASS” để suy ra đã đạt deadline runtime. So sánh waveform với app_config và TEST_PLAN sau khi có phần cứng.

## 11. Sơ đồ bố trí và các sơ đồ bổ sung

```markdown
![TST-01 — Vị trí cảm biến và khu vực kiểm thử](../Images/Architecture/test-layout.png)
```

**Cần thể hiện:** vị trí/hướng lắp sensor, trục IMU, phía trước xe, motor trái/phải, vật cản, vùng thử và vị trí đo. Ghi kích thước nếu đã đo, không đặt kích thước ước đoán thành thông số chính thức.

**Giải thích cần điền:** TODO — tại sao chọn vị trí sensor; vùng nào xe không quan sát được; bố trí kiểm thử tái lập; liên hệ với test case V01/S01/S03 trong TEST_PLAN.

Chỉ thêm khi hữu ích cho phần đang làm:

| Sơ đồ bổ sung | Mục đích | Điều kiện thêm |
| --- | --- | --- |
| Resource Map timer/bus/IRQ | Làm rõ ownership và tránh xung đột | Bảng board_config không đủ trực quan |
| Error/Fault Propagation | Theo dõi lỗi sensor → safety → motor/log/buzzer | Khi ghép mốc safety và fault injection |
| Gyro-assisted Turn | Giải thích góc tương đối và timeout quay | Chỉ khi tính năng nâng cao qua mốc go/no-go |
| Test Coverage Map | Nối requirement → module → test → bằng chứng | Giai đoạn regression/bàn giao |

Không cần thêm Zephyr, SLAM, wireless hay cảm biến trái/phải chỉ để có nhiều hình; phạm vi theo PROJECT_PLAN.

## 12. Review và nhật ký thay đổi

Trước khi đánh dấu một sơ đồ đã review:

- [ ] Hình đã tồn tại, đường dẫn đúng và chữ đọc được trên GitHub.
- [ ] Có ID, tên, revision/ngày, owner, reviewer và phần giải thích.
- [ ] Nhãn nguồn, mức logic, chân, đơn vị và chiều mũi tên rõ ràng.
- [ ] FSM có state ban đầu, guard, timeout/lỗi, STOP và điều kiện rearm phù hợp.
- [ ] Không nhầm sơ đồ đề xuất với code đã implement hoặc số đo đã có.
- [ ] Pinout/state/API/Task khớp source và các tài liệu liên quan ở cùng revision.
- [ ] Link file nguồn chỉnh sửa được và bằng chứng kiểm thử đã điền nếu có.

| Ngày | ID / revision | Nội dung thay đổi | Commit firmware liên quan | Người sửa / review |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |
