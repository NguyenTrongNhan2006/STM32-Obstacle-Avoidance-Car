# Kế hoạch kiểm thử

**Trạng thái:** chỉ các kiểm tra build trong [BUILD_VERIFICATION](BUILD_VERIFICATION.md) đã được chạy. Mọi kiểm thử board/xe bên dưới là kế hoạch, chưa được đánh dấu đạt.

Mỗi biên bản ghi commit SHA, người thử/review, ngày, board/module, nguồn và pinout revision, điều kiện thử, kết quả mong đợi/thực tế, log/waveform/ảnh, pass/fail và issue liên quan. Chạy lại case bị ảnh hưởng khi sửa firmware hoặc phần cứng.

## Ma trận kiểm tra

| ID / owner chính | Cách thử | Tiêu chí đạt |
| --- | --- | --- |
| B01 · Nhân | Debug + Release từ cây sạch | Compile/link thành công, source dự án không warning, đủ mọi module trong compile_commands |
| B02 · Hưng review | Chạy constraints với ELF tồn tại | Không dùng API bị cấm, không bỏ qua ELF, in “Tat ca dat.” |
| B03 · Nhân | size + map + high-water mark khi chạy | Flash <64 KB, SRAM tĩnh + vùng dự phòng <20 KB; stack margin có số đo cho từng Task và MSP |
| H01 · cả hai | Đo nguồn/reset/STBY khi chưa cấp motor | Đúng mức logic, STBY thấp qua reset, không có xung kích motor ngoài ý muốn |
| C01 · Nhân | Scope/debug clock và tick trước/sau scheduler | 72 MHz theo thiết kế; HAL/kernel tick đúng nhịp, HSE lỗi dẫn đến chẩn đoán hữu hạn |
| C02 · Nhân | Enable từng IRQ sau khi route | Pending được clear; priority/FromISR đúng; không rơi vào default trap |
| M01 · Nhân | Motor trên giá đỡ, test direction/duty/stop | Hai bánh đúng hướng, duty đo được, đổi hướng có chính sách an toàn đã review |
| S01 · Hưng | Khoảng cách chuẩn, nhiều góc/vật liệu, ngoài khoảng đo | Sai số được ghi theo khoảng; dữ liệu ngoài vùng hợp lệ không dùng để kết luận đường trống |
| S02 · Hưng | Mất Echo, Echo stuck high/low, pulse rollover | Hết deadline, mẫu TIMEOUT/ERROR; không treo Task, không publish khoảng cách hợp lệ giả |
| S03 · Hưng | MPU đứng yên rồi nghiêng theo từng trục | Signed data, axes/scale đúng, bias được ghi; ngưỡng/false alarm có số đo |
| S04 · Hưng | Sai địa chỉ MPU, rút sensor, bus stuck (bench có kiểm soát) | I2C timeout hữu hạn; lỗi được phản ánh status/fault, không kẹt scheduler |
| A01 · cả hai | Hai consumer peek mailbox; producer overwrite | Cả hai đọc được mẫu mới, timestamp không tự refresh; hiểu rõ snapshot khác thời điểm |
| A02 · Nhân | Chặn publish lâu hơn stale threshold | tSafety nhận stale, dừng và latch; không coi mẫu cũ là đường trống |
| A03 · cả hai | STOP ở FORWARD, TURN, CHECK và khi log tải cao | Đo latency từ chân nút đến STBY/PWM; đạt deadline do nhóm chốt từ quãng dừng |
| A04 · Nhân | Giả lập tDecision trễ/treo trên bench, bánh nâng | Đường inhibit được chứng minh độc lập theo thiết kế; không chỉ chờ lần gọi motor_apply tiếp theo |
| A05 · Hưng | Tilt/fault xảy ra rồi sensor bình thường trở lại | Không tự restart; rearm cần thao tác rõ và điều kiện an toàn đủ |
| F01 · Nhân | Chuỗi mẫu qua stop/clear boundary, tick wrap, retry hết | FSM không rung trạng thái, không sleep hết lượt quay, hết retry sang FAULT |
| V01 · cả hai | Vật cản phía trước và góc khó trong khu thử | Tối thiểu 10 lần mỗi bố trí; ghi số thành công, va chạm/dừng lỗi và giới hạn |
| W01 · Nhân/Hưng | Nếu mở rộng: 10 lượt quay mỗi hướng, nhiều mức pin | Báo mean/max sai lệch góc và drift; chỉ giữ khi cải thiện so với quay theo thời gian |
| L01 · Hưng | UART chuỗi 0/127/128 ký tự và u32 0/4294967295 | Biên chuỗi được xử lý đúng, số không sai/tràn buffer; timeout/lưu lượng được đo |
| R01 · cả hai | Soak mục tiêu 30 phút, nhiều trạng thái và nguồn thực | Không reset/treo ngoài ý muốn; fault/log/stack được lưu, phát hiện lỗi phải mở issue |

Các con số 10 lần/30 phút là mức kiểm thử đề xuất cho học phần, không là chứng nhận độ tin cậy sản phẩm. Ngưỡng latency/độ chính xác cần chốt bằng đo trước khi nghiệm thu; không tự ghi “đạt” khi chưa có tiêu chí.

## Test logic trước khi chạy xe

Khi bắt đầu implement, dùng chuỗi mẫu mock để kiểm tra chuyển state, STOP override, sensor stale, hysteresis, retry count và tick wrap. Không cần dựng mock cho các stub chỉ trả NOT_READY; ưu tiên test quyết định ảnh hưởng hành vi thật.

Mỗi tính năng mới có ít nhất happy path + lỗi input/sensor + timeout hoặc boundary liên quan. Người còn lại review dữ liệu test để tránh tác giả tự xác nhận chỉ một case thuận lợi.

## Điều kiện demo

MVP phải qua nguồn/STBY, direction/PWM, Echo/IMU hợp lệ, sensor fault, STOP/rearm và retry/FSM trước khi đặt xuống sàn chạy tự động. Bắt đầu tốc độ thấp trong vùng trống có người ngắt nguồn; không thử gần mép bàn/cầu thang vì xe chưa có cảm biến mép.

Demo nâng cao chỉ dùng tính năng qua go/no-go 21/11. Trình bày cả lỗi và giới hạn: một sensor phía trước không quan sát mọi phía; gyro không tạo heading tuyệt đối; kết quả build không thay thế kết quả chạy.
