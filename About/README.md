# STM32 Obstacle Avoidance Car — ý tưởng dự án

**Tác giả:** Nguyễn Trọng Nhân. **Trạng thái:** lên ý tưởng, chưa triển khai firmware.

Xe dự kiến phát hiện vật cản, dừng và lựa chọn hướng né. Kết hợp kiến thức PWM điều khiển động cơ, MPU6050, đa cảm biến và ngắt ngoài EXTI.

## Tài liệu

Đọc theo thứ tự sau. Mỗi tài liệu trả lời một nhóm câu hỏi để bạn tự triển khai mà không phải đoán vai trò của sườn file.

| Thứ tự | Tài liệu | Bạn sẽ tìm thấy |
| --- | --- | --- |
| 1 | [Ý tưởng và phạm vi dự án](PROJECT_PLAN.md) | Linh kiện đã nêu, phần còn đề xuất, hành vi xe và hướng mở rộng |
| 2 | [Cấu trúc firmware và nhiệm vụ từng file](FIRMWARE_GUIDE.md) | Vai trò `.c/.h`, 13 bộ driver, lớp App, Core và Config |
| 3 | [Hướng dẫn tự triển khai từng bước](IMPLEMENTATION_GUIDE.md) | Chuẩn bị VS Code/Git, lập pinout, tạo build, thứ tự viết module và tiêu chí hoàn thành từng mốc |
| 4 | [Kế hoạch kiểm thử](TEST_PLAN.md) | 16 ca kiểm tra, cách ghi kết quả, gợi ý tìm lỗi và quy ước sơ đồ/ảnh |

**Bắt đầu ở đâu?** Đọc mục 1–4 của hướng dẫn triển khai, chốt board/linh kiện rồi làm mốc A (khởi động, GPIO, timebase, UART). Chỉ chuyển sang motor và cảm biến sau khi xác nhận nền tảng hoạt động. Những mục ghi “chưa chốt” do tác giả tự quyết định; không phải thông số mặc định.

## Quy ước ở giai đoạn này

- `Firmware/` thay thế thư mục `Code/`.
- Mỗi file `.c` và `.h` chỉ chứa `// TO DO`. Không có API, include, cấu hình chân hoặc thuật toán sẵn.
- `CMakeLists.txt` chỉ chứa `# TO DO`; repo chưa build hoặc nạp được.
- Mọi hướng dẫn và đề xuất được ghi trong `About/`.
- `Images/` dành cho sơ đồ và hình ảnh sẽ bổ sung sau.
- Tác giả tự lựa chọn HAL/LL hoặc thanh ghi trực tiếp, tự cấu hình và triển khai từng module.

## Làm việc trong VS Code

Clone repo rồi mở thư mục gốc. Nếu đã có bản clone, lưu/commit thay đổi của bạn trước khi `git pull --ff-only`.

```sh
git clone https://github.com/NguyenTrongNhan2006/STM32-Obstacle-Avoidance-Car.git
cd STM32-Obstacle-Avoidance-Car
code .
```

Các bước tạo branch, kiểm tra diff, commit và push nằm trong [hướng dẫn triển khai, mục 2 và 8](IMPLEMENTATION_GUIDE.md). Dùng đúng branch đang làm và thay nội dung commit cho đúng phần bạn đã triển khai. Không ghi nhận kết quả thử nghiệm trước khi kiểm tra trên phần cứng.
