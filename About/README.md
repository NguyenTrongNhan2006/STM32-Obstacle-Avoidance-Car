# Mục lục dự án

**Nhân + Hưng · 20/09–20/12/2026 · FreeRTOS trên STM32F103C8T6.**

Đây là nền để hai người tự viết firmware. Build thành công chỉ xác nhận cú pháp, liên kết và bộ nhớ tĩnh; chưa xác nhận xe chạy, thời gian đáp ứng hay đấu nối.

| Đọc theo thứ tự | Nội dung |
| --- | --- |
| [Đánh giá ý tưởng](DESIGN_REVIEW.md) | Vì sao áp dụng skeleton có API; các lỗi kỹ thuật đã sửa |
| [Kế hoạch và phần cứng](PROJECT_PLAN.md) | Phạm vi 3 tháng, BOM, pinout, tính năng mở rộng có điều kiện |
| [Vai trò từng file](FIRMWARE_GUIDE.md) | Danh sách file, trạng thái thực tế, API và người phụ trách |
| [Hướng dẫn triển khai](IMPLEMENTATION_GUIDE.md) | Cài công cụ, build, debug, từng mốc phát triển và cách phối hợp |
| [Timeline Nhân–Hưng](TEAM_TIMELINE.md) | Công việc mỗi tuần, cân bằng tải, mốc tích hợp và deadline |
| [Kế hoạch kiểm thử](TEST_PLAN.md) | Điều kiện đạt, tình huống lỗi, cách lưu số liệu |
| [Kết quả kiểm tra skeleton](BUILD_VERIFICATION.md) | Lệnh đã chạy, số Flash/RAM, phần chưa được thử |

## Ba quy ước cần nhớ

- `[CO DINH]`: ràng buộc từ phần cứng/kiến trúc đã chọn; muốn đổi cần đối chiếu nguồn.
- `[DO]`: giả định hoặc tham số cần đo, **không phải số liệu đã được chứng minh**.
- `STATUS_NOT_READY` / `SAMPLE_NOT_READY`: chức năng chưa sẵn sàng; tuyệt đối không suy ra thao tác đã thành công hoặc đường đang trống.

Tài liệu chi tiết nằm trong `About/`; comment trong source chỉ giữ hợp đồng API và TODO cần thiết. Nếu thay đổi thiết kế, cập nhật config và tài liệu trong cùng PR.
