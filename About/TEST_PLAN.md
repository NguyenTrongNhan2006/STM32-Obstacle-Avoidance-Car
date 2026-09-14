# Kế hoạch kiểm thử và ghi kết quả

[Về mục lục](README.md) · [Thứ tự triển khai](IMPLEMENTATION_GUIDE.md)

**Trạng thái hiện tại: chưa thực hiện các kiểm thử dưới đây.** Đây là hướng dẫn để tác giả dùng khi đã tự triển khai từng phần, không phải báo cáo firmware đang hoạt động.

## Chuẩn bị

Ghi mã board, phiên bản mạch nối, nguồn, linh kiện, commit firmware và công cụ đo. Thử driver motor khi kê bánh khỏi mặt đất trước, rồi mới thử xe chạy trong vùng kiểm soát. Xác nhận mức điện áp và sơ đồ module thực tế trước khi nối; tài liệu hiện chưa phải sơ đồ đấu dây.

Trong mỗi ca thử, ghi kỳ vọng **trước** khi chạy. Khi cần ngưỡng định lượng như sai số khoảng cách hoặc độ trễ STOP, tự điền ngưỡng theo yêu cầu dự án; “chưa chốt” không được tính là đạt.

## Ma trận kiểm thử

| ID | Phần cần kiểm tra | Cách thử | Kết quả mong đợi |
| --- | --- | --- | --- |
| T01 | Khởi động/reset | Bật nguồn và reset nhiều lần | Vào `IDLE`; motor giữ trạng thái dừng đã định nghĩa |
| T02 | GPIO/LED | Đổi trạng thái output, đọc input đã biết | Mức đo và log trùng trạng thái mong muốn |
| T03 | Timebase | So sánh chu kỳ với công cụ đo; kiểm tra xử lý bộ đếm tràn khi đã có cách mô phỏng | Đúng đơn vị, timeout không mất tác dụng khi tràn |
| T04 | Nút | Nhấn ngắn, giữ, nhả và nhấn liên tiếp | Quy tắc sự kiện rõ ràng; chống dội; không tự START lại sau STOP |
| T05 | PWM/motor | Thử riêng bánh trái/phải; đổi lệnh; dừng và reset | Đúng bánh, đúng chiều, giới hạn lệnh có hiệu lực |
| T06 | Khoảng cách | Dùng thước tại nhiều vị trí trong vùng module hỗ trợ; đổi bề mặt/góc vật cản | Ghi sai số và tỷ lệ mẫu hợp lệ; đạt dung sai tự đặt |
| T07 | Mất Echo | Tạo tình huống không nhận Echo theo cách thử phù hợp phần cứng | Có timeout; mẫu bị đánh dấu lỗi; vòng lặp vẫn xử lý được STOP |
| T08 | IR, nếu có | Che từng bên, cả hai bên, thử ánh sáng/bề mặt khác nhau | Đúng trái/phải và cực tính; ghi nhận giới hạn phát hiện |
| T09 | MPU6050 | Đặt yên, xoay/nghiêng theo từng trục đã đánh dấu | Dữ liệu đổi đúng hướng; ghi offset và ảnh hưởng rung |
| T10 | Lỗi I2C | Mô phỏng lỗi trả về hoặc ngắt kết nối theo cách thử đã chuẩn bị | Thao tác có giới hạn; lỗi được báo; không dùng mẫu cũ như mẫu mới |
| T11 | Dữ liệu quá hạn | Ngừng cập nhật một cảm biến mà ứng dụng yêu cầu | `sensor_manager` đánh dấu quá hạn; xe dừng theo chính sách đã chốt |
| T12 | Chuyển trạng thái | Đường trống → vật cản → quay → đo lại | Có log nguyên nhân chuyển; `CHECK` đợi mẫu mới trước khi tiến |
| T13 | Không tìm được lối | Giữ vật cản qua nhiều lượt thử | Đến giới hạn thì dừng/báo lỗi; không quay vô hạn |
| T14 | Ưu tiên STOP | Nhấn STOP trong khi tiến, quay, chờ sensor và phát còi | Không còn lệnh chạy vượt qua STOP; ghi độ trễ thực tế |
| T15 | Phục hồi lỗi | Gây lỗi, khắc phục, xác nhận theo quy tắc | Không tự chạy lại chỉ vì lỗi vừa biến mất; quay về `IDLE` |
| T16 | Ghép toàn hệ thống | Cho motor hoạt động cùng cảm biến, log và còi | Không mất mẫu/treo/reset ngoài dự kiến; đo tải xử lý và độ trễ |

Không có IR thì đánh dấu T08 “Không áp dụng — chưa lắp IR”, không đánh dấu “Đạt”. Các ca chưa triển khai cũng ghi “Chưa thử”.

## Mẫu ghi kết quả

Sao chép mẫu này cho mỗi lần thử; có thể tạo `About/TEST_RESULTS.md` khi bắt đầu có dữ liệu thật.

| Trường | Nội dung tác giả tự điền |
| --- | --- |
| ID / ngày thử | Chưa thử |
| Commit / board / phiên bản nối dây | Chưa điền |
| Mục tiêu / điều kiện ban đầu | Chưa điền |
| Dụng cụ / cách tạo tình huống | Chưa điền |
| Kỳ vọng / ngưỡng chấp nhận | Chưa điền |
| Số lần lặp / số lần đạt | Chưa điền |
| Kết quả đo / log / ảnh | Chưa điền |
| Kết luận | Chưa thử / Đạt / Không đạt / Không áp dụng |
| Lỗi phát hiện / bước tiếp theo | Chưa điền |

Nên ghi log gồm thời điểm, trạng thái xe, nguyên nhân chuyển trạng thái, tuổi dữ liệu cảm biến và lệnh motor. Giới hạn tần suất log để bản thân việc debug không làm sai thời gian xử lý.

## Khi gặp lỗi, kiểm tra từ lớp gần phần cứng nhất

| Hiện tượng | Thứ tự kiểm tra |
| --- | --- |
| Không build | Startup/linker/toolchain đã có chưa → mã MCU → nguồn/include → symbol trùng → cấu hình liên kết |
| Nạp được nhưng không chạy | Reset/boot/nguồn → breakpoint → clock → handler lỗi → vòng chờ không timeout |
| Không có Echo | Đúng module/nguồn/mức logic → dây → Trigger → dạng sóng Echo → cách đo xung/đơn vị → timeout |
| MPU không trả dữ liệu | Nguồn/dây/pull-up → quy ước địa chỉ của API I2C đã chọn → nhận dạng → cấu hình → timeout |
| Motor sai chiều hoặc không quay | Nguồn motor → standby/enable → pin hướng → PWM → ánh xạ bánh → giới hạn driver |
| Xe đổi trạng thái liên tục | Dữ liệu nhiễu/quá hạn → điều kiện chuyển → hysteresis → chu kỳ lấy mẫu/cập nhật |
| Xe phản hồi STOP chậm | Delay/vòng chờ → I2C/Echo timeout → ISR dài → log/còi chặn → thứ tự áp dụng lệnh motor |

## Ảnh và sơ đồ

Khi có phần cứng thật, lưu hình trong `Images/` với tên dễ hiểu, chẳng hạn `block-diagram.png`, `wiring-v1.png`, `prototype-top.jpg`. Đây là tên gợi ý, chưa phải file đã tồn tại. Mỗi sơ đồ cần ngày/phiên bản, tên tín hiệu, nguồn và liên kết đến bảng pinout tương ứng. Khi đổi dây, cập nhật ảnh và cấu hình cùng một mốc Git để tránh tài liệu lệch phần cứng.
