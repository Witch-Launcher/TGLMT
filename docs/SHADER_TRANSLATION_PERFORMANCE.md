# Hiệu năng dịch GLSL → MSL

Tài liệu này mô tả **đo** và **tối ưu** bước dịch GLSL → MSL của TGLMT
(`src/gl/glsl_to_msl.cpp`), với ràng buộc bất di bất dịch:

> **MSL sinh ra phải BYTE-IDENTICAL trước và sau mọi tối ưu.**
> Hành vi OpenGL Desktop sau khi compile (định nghĩa shader, layout, giá trị
> uniform, vị trí `[[attribute]]`/`[[user(locn)]]`, thứ tự phát biểu, …) phải
> **không đổi 1 byte**.

## 1. Benchmark

`benchmarks/compilation/bench_translate.cpp` (target CMake `bench_translate`).

| Đặc điểm | Nội dung |
|---|---|
| Phạm vi đo | **chỉ** `ConvertGLSLtoMSL(source, stage)` — KHÔNG Metal, KHÔNG pipeline |
| Corpus | 86 shader: 79 shader `.vsh`/`.fsh` thật của Minecraft (`tests/corpus/mc-26.1.2-shaders`, `#moj_import` được inline) + 7 shader nhỏ của repo (`tests/shaders/glsl/`) |
| Protocol | `benchmarks/README.md`: warm-up bỏ, ≥ 9 lần chạy, báo **median của các median** + **min** |
| Thống kê | median của median (chịu nhiễu tốt hơn) và min (robust nhất) |
| Output | JSON trên stdout (cùng schema `bench_compile`) |
| Cổng đối chiếu | `--dump <dir>` ghi MSL của từng shader ra file ⇒ `diff -rq` trước/sau |

Lệnh:

```sh
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release -DTGLMT_APPLE_METAL=ON
cmake --build build-bench -j8 --target bench_translate
./build-bench/bench_translate --repeats 9 --warmup 2
./build-bench/bench_translate --dump /tmp/msl_before   # cổng đối chiếu
```

> **LƯU Ý ĐĨA:** benchmark dùng chung `build-bench` (Release). **Không** tạo thêm
> thư mục build — mỗi thư mục ~2 GB và đã làm đầy phân vùng nhiều lần.

## 2. Phân rã phase (đo, không đoán)

Công cụ `sample` (macOS) trên build Release cho ra profile phẳng. Đo thêm bằng
cách **instrument từng pass** (đóng gói bản copy tạm, không sửa file chính):

| Hạng mục | Trước tối ưu | Ghi chú |
|---|---|---|
| `_platform_strlen` | 163 mẫu | so sánh chuỗi tuyến tính + `std::string` tạm |
| `_platform_memcmp` | 92 | **quét tuyến tính bảng đổi tên trong `RewriteIdents`** |
| `std::string::push_back` | 91 | **copy từng ký tự trong mọi pass viết lại text** |
| `_platform_memchr` | 101 | quét text |
| `_platform_memmove` | 94 | copy text |
| `ExpandRelationalBuiltins` | 71 | dựng `std::string nm` **cho mỗi phép so sánh ở mỗi vị trí** |
| `RewriteIdents` | 53 | tra cứu tên O(#token × |mp|) |
| `ParseHelperDefs` | 39 | parse helper |

**Kết luận cấu trúc:** `ConvertGLSLtoMSL` chạy **~40 pass viết lại toàn bộ
`body`** và ~40 pass trên `helpersMSL`. Trên corpus Minecraft, **chỉ 2–3 pass
thực sự thay đổi text** (`PromoteVanillaIntMixing`, `PromoteIntVecArith`,
`RenameShadowedVars`) — phần còn lại là *no-op* nhưng vẫn quét + copy hết text.

## 3. Các tối ưu đã áp dụng

Tất cả nằm trong `src/gl/glsl_to_msl.cpp`. Mỗi mục đều **giữ nguyên output**.

### 3.1 Tra cứu tên O(1) thay vì quét tuyến tính — `RewriteIdents` (`:6254`)

Bảng đổi tên (`mp`) được tra bằng vòng lặp tuyến tính **cho mỗi identifier**
⇒ O(#token × |mp|). Đổi sang `unordered_map<string_view, string_view>`
(`emplace` giữ đúng ngữ nghĩa "khớp đầu tiên thắng"). Chuỗi khoá trỏ vào `mp`
(const, sống suốt lời gọi) ⇒ không copy chuỗi.

### 3.2 `string_view` thay `std::string` tạm — `EraseNamesAlsoDeclaredNonInt` (`:1852`), `NearestDeclFamily` (`:1893`)

Hai hàm này dựng `std::string` **cho mỗi identifier** trong text để so sánh với
literal. Đổi sang `std::string_view` (không cấp phát) + `set<string_view>` cho
bảng từ khoá.

### 3.3 Kiểm tra ký tự đầu trước `memcmp` — `CollectIntFamNames` (`:1791`)

`"uint"`/`"int"` đều bắt đầu bằng `'u'`/`'i'` ⇒ kiểm tra ký tự đầu trước khi
so sánh, loại ~99 % phép so sánh.

### 3.4 Literal thay `std::string` tạm + copy theo lô — `ExpandRelationalBuiltins` (`:1441`, `:1457`)

- Bảng tên hàm đổi từ `std::pair<const char*, const char*>` sang `const char*`
  ⇒ không cấp phát `std::string` cho **mỗi phép so sánh ở mỗi vị trí**.
- **Copy theo lô**: `out += text[i]` (push_back mỗi ký tự) → `out.append(text, a, b)`
  cho cả đoạn không khớp. Chuỗi kết quả giống hệt.

### 3.5 Copy theo lô — `RewriteIdents` (`:6266`)

Cùng kỹ thuật: phần text không phải identifier được copy một lần thay vì
từng ký tự.

### 3.6 Guard "token kích hoạt không có ⇒ pass không thể đổi gì"

Pass nào **chắc chắn** là no-op khi thiếu một token thì bỏ qua ngay, thay vì
quét + copy toàn bộ text. Mỗi guard đều **chứng minh được** từ mã nguồn của
pass (xem cột "điều kiện"):

| Pass | Điều kiện bắt buộc để pass đổi text | Guard |
|---|---|---|
| `ExpandRelationalBuiltins` | cả 6 tên hàm đều chứa `Than` hoặc `qual` | `find("Than")` / `find("qual")` |
| `CoerceFloatArgsInIntCtors` | chỉ khớp lời gọi `intN(`/`uintN(` (N ∈ 2..4) | `find("int2(")` / `int3(` / `int4(` (khớp luôn `uintN(`) |
| `PromoteUintBuiltinCompare` | cả 6 token đều bắt đầu `tglmt_` | `find("tglmt_")` |
| `PromoteVectorBoolOps` | chỉ xử lý toán tử `&&` / `\|\|` | `find("&&")` / `find("\|\|")` |
| `RenameLocalShadowUniforms` | cần tên uniform để shadow + cần scope | `mp.empty()` / không có `{` |

> **Không** đặt guard cho `PromoteBoolVecMix`: `PromoteBoolVecMixOnce` còn xử lý
> `mix(vec, vec, bvec)` → `select(...)` với `bvec` sinh từ phép so sánh
> (`lessThan`/`equal`) — không có literal `boolN` nào trong text. Guard sai này
> làm hỏng `test_pack_regressions` (đã phát hiện và gỡ).

### 3.7 Một hướng đã thử và **bị loại** (đo thấy chậm hơn)

`RenameLocalShadowUniforms` / `RenameParamShadowUniforms` dùng
`unordered_set<string_view>` thay quét tuyến tính `mp` ⇒ **chậm hơn** (bảng `mp`
nhỏ, chi phí dựng set > lợi ích). Đã **hoàn tác**, không giữ trong mã.

## 4. Cổng đúng đắn (correctness gate)

```sh
# 1. dump MSL TRƯỚC tối ưu
./build-bench/bench_translate --dump /tmp/msl_before
# 2. sau mỗi thay đổi: build lại rồi dump
./build-bench/bench_translate --dump /tmp/msl_after
# 3. diff — PHẢI rỗng
diff -rq /tmp/msl_before /tmp/msl_after
```

Kết quả: **86/86 file giống hệt** (0 khác biệt) cho toàn bộ tối ưu ở §3.

Ngoài ra: `ctest -j8` phải xanh toàn bộ (207/207). Cổng này **đã bắt được một
lỗi thật**: lần đầu chuyển `RewriteIdents` sang copy-theo-lô, nhánh "identifier
không khớp" quên flush đoạn chờ ⇒ text bị **nhân bản** (cả 86 file MSL khác
baseline). Sửa và chạy lại cổng.

> Corpus Minecraft **không** phủ hết: lỗi trên được `tests/unit/test_pack_regressions.cpp`
> bắt (case `mix(vec, vec, bvec)` → `select()`). Vì vậy cả hai cổng đều bắt buộc.

## 5. Kết quả đo

Số đo thật, máy phát triển (xem `reports/SHADER_TRANSLATION_BENCHMARK.md` cho
bảng đầy đủ kèm mô tả máy). Tóm tắt:

| Chỉ số | Trước | Sau | Cải thiện |
|---|---|---|---|
| median của median (86 shader) | 63 350,9 µs | 53 347,7 µs | **−15,8 %** |
| min (86 shader) | 62 182,5 µs | 52 602,9 µs | **−15,4 %** |
| µs / shader | 736,6 µs | 620,3 µs | **−15,8 %** |
| throughput (source) | 2,013 MB/s | 2,380 MB/s | **+18,2 %** |

## 6. Hạn chế / việc chưa làm

- Đo trên **Null backend** (chỉ đo CPU): số Metal không nằm trong phạm vi
  benchmark này (xem `bench_compile`).
- Số đo trên máy dev có nhiễu lớn (median dao động ±10 %, min ±5 %): mọi so
  sánh đều **đan xen** (baseline/optimized chạy luân phiên) và lấy **best-of-N**.
- Chưa tối ưu: `ParseHelperDefs`/`ThreadHelpersMSL` (~5 % profile), các pass
  `Promote*`/`Coerce*` còn lại (chưa tìm được điều kiện bắt buộc rẻ để guard).
- Chưa cache kết quả dịch: **không** làm vì sẽ thay đổi hành vi quan sát
  được của `glCompileShader`/`glLinkProgram` (và app thật chỉ dịch 1 lần/shader).
