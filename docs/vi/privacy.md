# TGLMT — Chính sách bảo mật

TGLMT là lớp chuyển đổi OpenGL 4.6 Core → Metal cho iOS và macOS. Nó được phân phối theo giấy phép MIT.

## Dữ liệu chúng tôi thu thập

**Không có.** TGLMT là thư viện kết xuất. Nó không thu thập, lưu trữ hoặc truyền bất kỳ dữ liệu cá nhân nào. Không có phân tích, không có télémétri, không có quảng cáo và không phân hạng người dùng.

## Hoạt động chỉ ở cục bộ

TGLMT chạy hoàn toàn trên thiết bị chủ. Nó:

- Chuyển đổi lệnh OpenGL thành bộ lệnh Metal cục bộ
- Biên dịch shader GLSL thành MSL cục bộ
- Trình bày khung hình qua CAMetalLayer (hoặc đọc đầu.out đầu ra không đầu cuối)

Không dữ liệu nào rời khỏi thiết bị trừ khi ứng dụng của bạn tự gửi nó.

## Nhật ký

TGLMT có thể ghi nhật ký chẩn đoán để gỡ lỗi. Chúng chỉ ở cục bộ và không bao giờ được tải lên bởi thư viện.

## Trẻ em

TGLMT không thu thập dữ liệu từ bất kỳ ai, bao gồm cả trẻ em.

## Quyền của bạn

Vì không có dữ liệu được thu thập, không có gì để truy cập, xuất hoặc xóa ở phía chúng tôi. Tất cả trạng thái kết xuất sống trong tiến trình ứng dụng của bạn.

## Liên hệ

Mở sự kiện trên [kho lưu trữ GitHub của TGLMT](https://github.com/Witch-Launcher/TGLMT).
