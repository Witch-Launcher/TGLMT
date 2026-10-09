# TGLMT

[English version](README.md)

TGLMT là lớp dịch cho phép render **OpenGL 4.6 Core** chạy trên **Metal**. Ứng dụng
vẫn gọi lệnh OpenGL; TGLMT kiểm tra tham số, ghi trạng thái GL trên CPU, dịch GLSL sang
Metal Shading Language khi link program, và gom các lệnh kết quả thành command buffer
của Metal.

Không có driver Vulkan nào trong đường đi: không MoltenVK, không ANGLE, không SPIR-V ở
runtime.

| | |
|---|---|
| Mặt API | OpenGL 4.6 Core — 698 hàm khai báo trong `include/tglmt/gl46.h`, 701 ký hiệu C xuất ra |
| Đầu vào shader | GLSL vertex + fragment, chuyển thẳng sang MSL tại `glCompileShader` |
| Nền tảng | iOS arm64 (thiết kế theo chuẩn A11), macOS host |
| Backend | `apple` (Metal, GPU thật) và `null` (trace/shadow trên CPU, không cần GPU) |
| Sản phẩm | `libtglmt.a` (static) và `libtglmt.dylib` (shared, dùng `dlopen`/`dlsym`) |
| Giấy phép | MIT |

Trường hợp sử dụng chính là đường boot vanilla của launcher Minecraft Java trên iOS:
LWJGL/GLFW phân giải ký hiệu `gl*` lúc runtime và renderer phát ra các lệnh OpenGL
thông thường.

## Kiến trúc

```
  Ứng dụng OpenGL 4.6 (LWJGL / GLFW / Blaze3D / bất kỳ ai gọi GL)
        |
        |  698 điểm vào gl* + window API TGLMT_*
        v
  +------------------------------------------------------------------+
  | Lớp dispatch        src/gl/*.cpp                                   |
  |   kiểm tra tham số, ghi trạng thái GL, shadow các glGet*           |
  +------------------------------------------------------------------+
        |
        +--> StateTracker / ObjectRegistry / ErrorTracker   (include/tglmt)
        |
        +--> Bộ chuyển GLSL -> MSL  src/gl/glsl_to_msl.cpp
        |        (chỉ vertex + fragment; các stage khác bị từ chối trung thực)
        v
  Đường vẽ hoãn        src/gl/gl_apple_draw.cpp
        |  shadow trạng thái, bỏ qua trạng thái trùng lặp
        |  dùng lại encoder giữa các draw, cache pipeline-key
        |  staging buffer/texture có gộp vùng, upload qua ring-buffer
        v
  Trừu tượng Metal     include/tglmt/MetalInterface.h
        |
        +--> Backend Apple   src/metal/AppleMetalBridge.mm  (GPU thật)
        +--> Backend Null    src/metal/NullMetalBackend.cpp (trace/shadow)
        v
  Present lên CAMetalLayer   hoặc   đọc lại headless từ buffer shadow
```

Mô tả đầy đủ từng stage nằm trong [Kiến trúc](docs/vi/architecture.md).

## Nên đọc tiếp

| Tài liệu | Nội dung |
|---|---|
| [Bắt đầu](docs/vi/getting-started.md) | Ma trận build, test, frame đầu tiên, cách xác nhận backend GPU đang chạy |
| [Kiến trúc](docs/vi/architecture.md) | Vòng đời draw-call, thực thi hoãn, đổi tọa độ |
| [Tham khảo API](docs/vi/api.md) | `Renderer`, `tglmt::gl`, các export C, GLFW shim |
| [Tích hợp](docs/vi/integration.md) | Nhúng dylib, `CAMetalLayer`, luồng, chẩn đoán màn đen |
| [Tương thích & Hiệu năng](docs/vi/compatibility.md) | Cái gì đã làm, làm một phần, chỉ trace hay không hỗ trợ; counter; test chạy lại được |

## Build và test

```sh
cmake -S . -B build              # backend Null, build được trên mọi host
cmake --build build -j8
ctest --test-dir build --output-on-failure
```

```sh
cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON   # macOS, Metal thật
cmake --build build-apple -j8
ctest --test-dir build-apple --output-on-failure
```

Test cần GPU sẽ in `SKIP` và thoát 0 khi không có GPU, nên cả hai cấu hình đều chạy
an toàn trong CI. Xem [Bắt đầu](docs/vi/getting-started.md) để biết cách cross-compile
cho iOS và ma trận build đầy đủ.

## Trang tài liệu

`docs/index.html` là trang tài liệu song ngữ (Anh/Việt, sáng/tối, tìm kiếm). Trang này
được sinh từ Markdown trong repository:

```sh
python3 tools/gen_docs_site.py
```

Trình sinh là nơi duy nhất định nghĩa danh sách trang; `docs/index.html` chỉ là mẫu
giao diện. Xem [Bắt đầu](docs/vi/getting-started.md) để biết quy trình tái sinh.
