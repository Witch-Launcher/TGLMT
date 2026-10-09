# Bắt đầu

Trang này gồm ma trận build, cách chạy test, một frame đầu tiên tối giản, và cách
phân biệt backend Metal thật có đang chạy hay không.

## Yêu cầu tiên quyết

| Yêu cầu | Dùng cho |
|---|---|
| CMake 3.20+ | mọi build |
| Compiler C++17 | mọi build |
| Python 3 | sinh lại `gl46.h`, `gl46_types.h`, `gl_dispatch_gen.cpp`, `gl_c_exports.cpp`, `gl_stubs_gen.cpp` |
| Xcode command line tools | build macOS và iOS, framework Metal |
| `glslangValidator` + `spirv-cross` | tùy chọn; bật `test_spirv_chain_apple` và `test_converter_proof` |
| `xcrun metal` | tùy chọn; bật kiểm tra MSL offline trong `test_converter_aquarium` |

Backend Null build được trên mọi host, kể cả Linux, vì không biên dịch bất kỳ source
Apple nào.

## Ma trận build

Bốn cấu hình đều được hỗ trợ. `TGLMT_APPLE_METAL` mặc định `OFF`; bật cờ này mới kéo
`src/metal/AppleMetalBridge.mm` vào và link `-framework Metal`.

| Cấu hình | Configure | Kết quả |
|---|---|---|
| Host, backend Null (mặc định) | `cmake -S . -B build` | `libtglmt.a`, `libtglmt.dylib`, toàn bộ test |
| Host, Apple Metal | `cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON` | như trên, cộng test dùng GPU thật |
| iOS device, arm64 | xem bên dưới | `build-iphoneos/libtglmt.a` |
| iOS device, dylib chia sẻ | xem bên dưới | `build-ios/libtglmt.dylib` arm64 |

### Host, backend Null

```sh
cmake -S . -B build
cmake --build build -j8
```

### Host, Apple Metal (macOS)

```sh
cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON
cmake --build build-apple -j8
```

Cấu hình này bật Objective-C++ của CMake, biên dịch `AppleMetalBridge.mm` với ARC, định
nghĩa `TGLMT_APPLE_METAL=1`, và link `Metal`, `Foundation`, `QuartzCore`.

### iOS device, arm64

Hai cách gọi tương đương. Dạng dùng toolchain file là dạng mà `Makefile` ở gốc dùng và
là dạng nên dùng, vì nó còn ép thêm `-miphoneos-version-min`:

```sh
cmake -S . -B build-iphoneos \
  -DTGLMT_APPLE_METAL=ON \
  -DCMAKE_TOOLCHAIN_FILE=apps/aquarium/ios_toolchain.cmake \
  -DIOS_DEPLOYMENT_TARGET=16.0 \
  -DTGLMT_BUILD_TESTS=OFF
cmake --build build-iphoneos -j8
```

Dạng trực tiếp, không qua toolchain file:

```sh
cmake -S . -B build-ios \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT="$(xcrun --sdk iphoneos --show-sdk-path)" \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0 \
  -DTGLMT_APPLE_METAL=ON \
  -DTGLMT_BUILD_TESTS=OFF
cmake --build build-ios -j8
```

Kiến trúc chỉ `arm64`. Deployment target mặc định `16.0` (`DEPLOY_TARGET` trong
`Makefile` ở gốc, `IOS_DEPLOYMENT_TARGET` trong `apps/aquarium/ios_toolchain.cmake`).

Để build app benchmark Aquarium trên iOS, kể cả ký, xem
[`apps/aquarium/DEPLOY_IOS.md`](../../apps/aquarium/DEPLOY_IOS.md).

## Đầu ra thư viện

Cả hai target dựng từ cùng một danh sách source.

| Target | Loại | Đầu ra | Ghi chú |
|---|---|---|---|
| `tglmt` | static | `libtglmt.a` | cho app link trực tiếp |
| `tglmt_shared` | shared | `libtglmt.dylib` → `libtglmt.0.dylib` → `libtglmt.0.1.0.dylib` | cho `dlopen`/`dlsym` |

Visibility của symbol cố ý giữ ở mặc định của C++ (`CXX_VISIBILITY_PRESET default`,
`VISIBILITY_INLINES_HIDDEN OFF`), vì loader giải quyết tên `gl*` qua `dlsym`. Không có
quy tắc `install()`; dùng trực tiếp sản phẩm trong thư mục build.

## Test

Mỗi file dưới `tests/unit/` và `tests/integration/` thành một executable và một test
CTest mang tên file stem.

```sh
cmake --build build -j8
ctest --test-dir build --output-on-failure
```

`make test` bọc lệnh này qua `scripts/build_and_test.sh`, vốn còn sinh lại header GL
từ `docs/khronos/gl.xml` trước (đưa `--no-gen` để bỏ qua).

Test cần GPU sẽ in `SKIP` và thoát 0 khi không có GPU, nên build backend Null và build
backend Apple đều báo thành công trên máy chạy không người.

Một số hành vi cần biết trước khi đọc kết quả:

- Test có assert thật cần GPU thì bị skip, không fail, khi thiết bị là Null device.
- Test đọc corpus shader Minecraft kiểm tra `TGLMT_CORPUS_DIR` có tồn tại, không có thì
  skip. Corpus không được commit; tải bằng `tests/corpus/fetch-corpus.sh`.
- Test gọi ra `glslangValidator`, `spirv-cross` hay `xcrun metal` sẽ skip khi thiếu công
  cụ đó.

Muốn thử backend Apple thì cần build trên macOS:

```sh
cmake --build build-apple -j8
ctest --test-dir build-apple --output-on-failure
```

## Frame đầu tiên

```cpp
#include "tglmt/Renderer.h"
#include "tglmt/gl46.h"

tglmt::Renderer r;
if (!r.Init("apple", 960, 600, /*depth=*/true)) return 1;

// Mọi lệnh gl:: đều phải gọi sau Init / BeginFrame: chúng tác động lên Context
// hiện hành, và BeginFrame chính là thứ làm Context của Renderer này trở nên
// hiện hành.
r.BeginFrame();

tglmt::gl::glViewport(0, 0, 960, 600);
tglmt::gl::glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
tglmt::gl::glClear(0x4000 /* GL_COLOR_BUFFER_BIT */);

r.EndFrame(metalLayer);   // nullptr = headless, vẫn trả true
```

Toàn bộ lệnh dựng hình nằm trong namespace `tglmt::gl` và nhận đúng tham số như
OpenGL 4.6 bản desktop.

## Bất biến lúc chạy

Nên nói rõ các điểm này trước khi viết code trên API:

1. **Lệnh `gl::` tác động lên Context hiện hành.** `Renderer::BeginFrame` làm Context
   của Renderer trở nên hiện hành. Lệnh gọi trước `Init`/`BeginFrame` sẽ rơi vào Context
   dự phòng và đối tượng của chúng nằm sai chỗ.
2. **Một Context, một thread tại một thời điểm.** Context hiện hành là `thread_local`.
   Ứng dụng có thread nạp block chạy song song với render thread phải tự tuần tự hóa
   bên ngoài và gọi `MakeCurrent` trên từng thread.
3. **`Resize` có hiệu lực ở `BeginFrame` kế tiếp.** Render target bị hủy ngay và dựng lại
   lười; frame kế tiếp bị ép phải clear.
4. **`EndFrame` flush.** Render encoder đang mở và mọi upload buffer/texture đang staging
   được đẩy ra trước khi present hoặc trước khi trả về headless.
5. **Định dạng màu phải khớp layer.** `Init` nhận pixel format; `presentTarget` từ chối
   `CAMetalLayer` có `pixelFormat` khác, và biểu hiện là màn hình đen.
6. **`BeginFrame(false)` không được xoay gì cả.** Chế độ đó chỉ re-bind Context, dành
   cho người gọi `MakeCurrent` giữa chừng một frame. Xoay ring hay target ở đó sẽ làm
   hỏng frame.

## Backend GPU thật đã bật chưa?

Có bốn cách kiểm tra độc lập nhau, từ rẻ nhất tới chi tiết nhất.

```cpp
r.hasRealGPU();          // C++: true chỉ khi device là Metal device
r.backendName();         // C++: "apple", "null", hoặc "apple(fallback:null,no-MTL-device)"
```

```c
TGLMT_HasRealGPU();      // C ABI, cho JNI / Objective-C launcher
TGLMT_BackendName();     // C ABI
```

Trên Null device, tên backend là `apple(null-fallback,no-MTL-device)` — tín hiệu trung
thực cho biết backend "apple" đã được yêu cầu nhưng không tạo ra Metal device: hoặc
build không bật `TGLMT_APPLE_METAL`, hoặc tạo device thất bại. Khi đó mọi draw chỉ là
trace và `ReadPixels` trả về shadow CPU, không phải ảnh GPU.

Ở runtime, stderr có dòng tóm tắt frame cho 12 swap đầu rồi cứ 600 swap một lần:

```
[TGLMT] build=... frame=1 ... dAtt=... dEnc=... target=0x... WxH vp=... center=(r,g,b,a) present=1
[TGLMT] ir frame=1 encNew=... encReuse=... pipeRe=... pipeLk=... uniRe=... stSkip=...
```

`dAtt` là số draw đã thử, `dEnc` là số draw thực sự được encode. Nếu `dEnc` bằng 0 trong
khi `dAtt` không bằng 0 thì draw đang bị bỏ — xem
[Tương thích & Hiệu năng](compatibility.md) để biết các counter từ chối.

## Sửa lỗi nhanh

| Triệu chứng | Kiểm tra đầu tiên |
|---|---|
| `presentTarget` từ chối, màn hình vẫn đen | `CAMetalLayer.pixelFormat` phải là `MTLPixelFormatRGBA8Unorm` |
| Draw tụt về 0 | `dAtt` vs `dEnc` trong dòng tóm tắt frame; rồi `noProg`, `noTgt`, `noPipe`, `misc` |
| `ReadPixels` trả về đen hoặc dữ liệu cũ | `hasRealGPU()` — Null device đọc lại buffer shadow |
| Test không configure được | CMake 3.20+ và compiler C++17 |
| `test_converter_*` hay `test_spirv_chain_apple` skip | cài `glslangValidator`, `spirv-cross`, `xcrun metal` |
| Test riêng của Minecraft skip | chạy `tests/corpus/fetch-corpus.sh` |
| Không thấy chẩn đoán trên stderr | đặt `TGLMT_DIAG=1`, hoặc configure với `-DTGLMT_FORCE_DIAG=ON` cho máy mà launcher không truyền được biến môi trường |

## Sinh lại trang tài liệu

```sh
python3 tools/gen_docs_site.py
```

Lệnh này đọc `README.md`, `README.vi.md` và `docs/{en,vi}/*.md`, chuyển sang HTML, xuất
`docs/content-en.js`, `docs/content-vi.js` và `docs/nav.js`, rồi in ra file nào đã đổi.
Sau khi sửa bất kỳ Markdown chuẩn nào, hãy chạy lệnh này và commit file sinh ra cùng
file nguồn.
