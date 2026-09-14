# STM32 Obstacle Avoidance Car — ý tưởng dự án

**Tác giả:** Nguyễn Trọng Nhân. **Trạng thái:** lên ý tưởng, chưa triển khai firmware.

Xe dự kiến phát hiện vật cản, dừng và lựa chọn hướng né. Kết hợp kiến thức PWM điều khiển động cơ, MPU6050, đa cảm biến và ngắt ngoài EXTI.

## Tài liệu

- [Cấu trúc firmware và nhiệm vụ từng file](FIRMWARE_GUIDE.md)
- [Đề xuất phần cứng, hành vi và thứ tự triển khai](PROJECT_PLAN.md)

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

Sau khi tự triển khai một phần:

```sh
git status
git add Firmware About Images
git commit -m "Implement selected module"
git push origin main
```

Thay nội dung commit cho đúng phần bạn đã làm. Không ghi nhận kết quả thử nghiệm trước khi kiểm tra trên phần cứng.
