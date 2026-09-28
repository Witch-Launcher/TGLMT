# Deploy AquariumBench lên iPhone thật

App render **100% OpenGL 4.6** (`apps/aquarium/aquarium.cpp` chỉ gọi `tglmt::gl`),
dịch runtime sang Metal qua `build-iphoneos/libtglmt.a` (đã kiểm chứng 698/698
symbols, arm64). Đã kiểm chứng tại repo này ngày 2026-09-26:
`clang++ -arch arm64 -isysroot iphoneos … → AquariumBench` link OK
(Xcode project sinh sẵn; phần duy nhất còn lại là ký bằng Team của bạn).

## 0. Yêu cầu

- Xcode mới nhất (SDK iPhoneOS 26.x), iPhone iOS 16+ (Metal), cáp USB-C.
- Apple ID đăng nhập trong Xcode → Team (miễn phí cũng deploy được 7 ngày).

## 1. Build lib TGLMT cho iOS (làm 1 lần)

```sh
cmake -S . -B build-iphoneos -DTGLMT_APPLE_METAL=ON \
  -DCMAKE_TOOLCHAIN_FILE="apps/aquarium/ios_toolchain.cmake" -DTGLMT_BUILD_TESTS=OFF
cmake --build build-iphoneos -j8
# → build-iphoneos/libtglmt.a (arm64, 698/698 hàm GL)
```

## 2. Sinh Xcode project

```sh
sh scripts/configure_ios.sh
open apps/aquarium/build-ios/AquariumBench.xcodeproj
```

## 3. Ký + chạy (trong Xcode)

1. Cắm iPhone (mở khóa, Trust máy).
2. Project → Target AquariumBench → **Signing & Capabilities** → chọn **Team**.
   Bundle ID `com.tglmt.aquarium` (đổi hậu tố nếu trùng, vd `com.tglmt.aquarium.tenban`).
3. Chọn destination = iPhone của bạn (không phải Simulator — app cần GPU thật).
4. Bấm **Run** (⌘R).
5. Lần đầu: iPhone hỏi Trust developer → Settings → General → VPN & Device
   Management → tin cậy.

## 4. Xem điểm benchmark

- Xcode → Debug area (⇧⌘Y) xem log: `[bench] frame=60 fps=...` mỗi giây.
- Để nguyên 60–120 giây lấy điểm ổn định. iOS luôn vsync (60/120Hz) nên điểm
  tối đa = tần số màn hình — so sánh iPhone với nhau là công bằng; macOS
  (`displaySyncEnabled=NO`) đo throughput thô.
- Thoát app: vuốt lên (không có nút quit trên iOS, đúng chuẩn nền tảng).

## 5. Khắc phục

| Hiện tượng | Nguyên nhân / sửa |
|---|---|
| Signing failed | Chưa chọn Team; bundle ID trùng → đổi tên |
| Màn hình đen, log `present FAIL` | drawableSize ≠ target — đã tự xử lý resize; báo log kèm size |
| `Init FAIL` | Thiếu `Resources/shaders` trong bundle — clean build folder rồi build lại |
| FPS thấp bất thường | iPhone nóng (thermal throttle) — để nguội, tắt app nền, giảm độ sáng |

## 6. Những gì đã kiểm chứng mà không cần device

- Toàn bộ sources lib + app + shell biên dịch sạch cho `iphoneos/arm64`
  (`-fsyntax-only`, xem COVERAGE.md).
- `libtglmt.a` iphoneos chứa đủ 698/698 hàm (`nm -gC`).
- App link tay thành công ra binary arm64 (chỉ thiếu chữ ký).
- Render đúng đã chứng minh trên macOS (screenshot `docs/aquarium-shot.png`
  nếu có) + 21/21 tests trên `build-apple` (7 test GPU thật).
