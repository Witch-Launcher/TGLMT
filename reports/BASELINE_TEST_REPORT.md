# BASELINE_TEST_REPORT.md — Baseline kiểm thử PHASE 4 (Full Baseline Testing)

> **Báo cáo này chỉ chứa số đo THẬT**, mỗi số đều kèm lệnh đã dùng để tái lập.
> Không có số ước lượng. Mục nào chưa đo được ⇒ ghi `NOT_VERIFIED` **kèm lệnh
> đo** để người sau đo được.
>
> **Phạm vi:** toàn bộ Desktop OpenGL 1.0–4.6, Core + Compatibility Profile.

---

## 0. Môi trường đo và nhãn (§14.F)

| Mục | Giá trị đo được |
|---|---|
| Máy | `iMac.local` — Intel(R) Core(TM) i3-10105 CPU @ 3.70 GHz |
| Kiến trúc | `x86_64` (`uname -m`) |
| Hệ điều hành | macOS 15.7.7, Darwin Kernel 24.6.0 |
| **GPU** | **Intel UHD Graphics 630** (Device ID 0x3e91, Metal 3) — `system_profiler SPDisplaysDataType` |
| Metal device do TGLMT báo | `TGLMT Apple (Intel(R) KBL Unknown)` |
| Build dir | `build-work/` — `CMAKE_BUILD_TYPE=Debug`, `TGLMT_APPLE_METAL=ON`, `TGLMT_BUILD_TESTS=ON` |
| Artifact | `build-work/libtglmt.0.1.0.dylib` — 8 552 160 byte, link lúc `Nov 11 14:56` |
| Cây nguồn | `git rev-parse --short HEAD` = **`a0347e0`**, cây **bẩn** (`git status --porcelain` có thay đổi chưa commit) |

### 0.1 Nhãn môi trường áp dụng cho TOÀN BỘ số đo trong báo cáo

> ## `OTHER_GPU`
>
> **KHÔNG phải `A11_DEVICE`.** Không phải `SIMULATOR`. Không phải
> `REFERENCE_ONLY` (vì backend Metal **thật** đã chạy).
>
> Hệ quả bắt buộc: **không** được dùng bất kỳ số nào trong báo cáo này để
> khẳng định hành vi trên Apple A11. Mọi khẳng định về A11 phải chờ số đo
> nhãn `A11_DEVICE` (xem §7, NV-01).

Các nhãn phụ (khi áp dụng):

| Nhãn | Áp dụng cho |
|---|---|
| `OTHER_GPU` | toàn bộ test CTest, coverage đo bằng `nm`/script, benchmark |
| `REFERENCE_ONLY` | `test_conformance_surface` và `test_api_surface` (dùng backend **Null**) |
| `REFERENCE_ONLY` (đối chiếu) | `test_converter_proof` — phần **reference** là `glslangValidator` + `spirv-cross`; phần **TGLMT** chạy trên `OTHER_GPU` |
| `NONE` | CTS (chưa chạy), A11, bộ ảnh tham chiếu |

---

## 1. Lệnh đã dùng (tái lập được 100 %)

```bash
cd /Volumes/project/TGLMT

# 1) số test
(cd build-work && ctest -N)

# 2) chạy test (KHÔNG build lại — build-work đã có sẵn artifact)
(cd build-work && ctest -j8 --output-on-failure)

# 3) coverage bề mặt API (giống hệt docs/OPENGL_API_COVERAGE.md)
python3 - <<'EOF'
import re, pathlib, json
stub = pathlib.Path("src/gl/gl_stubs_gen.cpp").read_text()
stubs = set(re.findall(r"UNIMPLEMENTED: (gl[A-Za-z0-9_]+)", stub))
inv = json.load(open("data/version_inventory.json"))
compat = set(inv["versions"]["4.6"]["commands"]["compatibility"])
core   = set(inv["versions"]["4.6"]["commands"]["core"])
print("compat", len(compat), "stub", len(stubs & compat), "impl", len(compat - stubs))
print("core  ", len(core),   "stub", len(stubs & core),   "impl", len(core - stubs))
print("legacy", len(compat-core), "stub", len(stubs & (compat-core)))
EOF

# 4) audit bề mặt
python3 tools/truth_manifest.py --lib build-work/libtglmt.dylib --strict
python3 tools/ext_matrix.py     --lib build-work/libtglmt.dylib --strict
python3 tools/ext_coverage.py    --dlsym build-work/libtglmt.dylib

# 5) benchmark (protocol 9 lần → median của median)
cd build-work
for i in 1 2 3 4 5 6 7 8 9; do
  ./bench_compile --repeats 30 --warmup 5 > /tmp/compile_$i.json 2>/dev/null
  ./bench_runtime --iters 2000 --warmup 300 > /tmp/runtime_$i.json 2>/dev/null
  ./bench_upload --iters 300 | sed -n '/^{/,$p' > /tmp/upload_$i.json
done
cd ..
python3 benchmarks/compare/bench_median.py /tmp/compile_*.json -o /tmp/compile_merged.json
python3 benchmarks/compare/bench_median.py /tmp/runtime_*.json -o /tmp/runtime_merged.json
python3 benchmarks/compare/bench_median.py /tmp/upload_*.json  -o /tmp/upload_merged.json
```

---

## 2. CTest — số test thật

### 2.1 Số test

```
$ (cd build-work && ctest -N | tail -3)
  Test #187: test_vertex_packed
  Test #188: test_viewport_array

Total Tests: 188
```

| Hạng mục | Số đo |
|---|---:|
| **Tổng test CTest** | **188** |
| — `tests/unit/` | 95 |
| — `tests/integration/` | 77 |
| — `tests/regression/` | 13 |
| — `tests/api/` | 1 |
| — `tests/conformance/` | 1 |
| — `tests/shaders/` | 1 |
| Test có hậu tố `_apple` (cần Metal device) | 81 |
| Test tạo context GL với backend `"apple"` (`Context ctx("apple")`) | 59 |
| Test dùng `CreateDevice("apple")` trực tiếp | 13 |

*(đếm bằng `re.findall(r'^add_test\("([A-Za-z0-9_]+)"', CTestTestfile.cmake)` và
đối chiếu với danh sách file trong `tests/`)*

### 2.2 Kết quả chạy

```
$ (cd build-work && ctest -j8 --output-on-failure)
...
100% tests passed out of 188

Total Test time (real) =  84.00 sec     # lần chạy ĐẦU (cache Metal lạnh)
```

Chạy lại ngay sau đó (cache Metal ấm):

```
$ (cd build-work && ctest -j8 --output-on-failure)
100% tests passed out of 188

Total Test time (real) =   6.52 sec
```

| Lần chạy | Kết quả | Wall time |
|---|---|---:|
| Lần 1 (cache lạnh) | **188 / 188 PASS** | 84.00 s |
| Lần 2 (cache ấm) | **188 / 188 PASS** | 6.52 s |

### 2.3 Backend Null (để so)

```
$ (cd build-null && ctest -j8)
100% tests passed out of 185
Total Test time (real) =   3.70 sec
```

> ⚠️ **`build-null/` là build CŨ**: `CTestTestfile.cmake` sinh lúc `Nov 1 01:52`,
> chứa **185** test (thiếu 3 test mới của session T0/T1) và build từ cây cũ
> (`10ff95f`-era). **KHÔNG dùng làm baseline của cây `a0347e0`.** Muốn có số
> Null-backend cho cây hiện tại phải build lại:
> `cmake -S . -B build-null -DCMAKE_BUILD_TYPE=Release -DTGLMT_APPLE_METAL=OFF && cmake --build build-null -j8`.

### 2.4 10 test chậm nhất (lần chạy cache lạnh)

| # | Test | Thời gian |
|---:|---|---:|
| 1 | `test_converter_aquarium` | 4.58 s |
| 2 | `test_pack_regressions` | 4.55 s |
| 3 | `test_tess_vs_capture_apple` | 3.03 s |
| 4 | `test_shader_corpus` | 3.03 s |
| 5 | `test_spirv_chain_apple` | 1.04 s |
| 6 | `test_mc_blit_apple` | 1.03 s |
| 7 | `test_converter_proof` | 0.97 s |
| 8 | `test_frag_out_scalar` | 0.81 s |
| 9 | `test_mc_corpus` | 0.69 s |
| 10 | `test_water_translucent_apple` | 0.64 s |

---

## 3. Failure report — FAIL / SKIP / NOT_RUN **thật**

### 3.1 FAIL

```
$ (cd build-work && ctest -j8 --output-on-failure | grep -c "Failed")
0
```

**Số test FAIL: 0.** Không có test nào fail ở cây `a0347e0` trên `OTHER_GPU`.

### 3.2 SKIP — đọc lại từ **output** của test, không chỉ tin `ctest`

`ctest` báo `Passed` cho mọi test, nên phải quét output:

```
$ (cd build-work && ctest -j8 -V) | grep -E "\bSKIP\b"
19: test_gs_mesh_apple SKIP: no mesh execution (need Apple7+; observed black on Intel)
143: test_mc_terrain_ubo_real PASS (terrain blocks + atlas ROW_LENGTH/SKIP)
```

| Test | Dòng SKIP | Vì sao | Trạng thái |
|---|---|---|---|
| `test_gs_mesh_apple` | `SKIP: no mesh execution (need Apple7+; observed black on Intel)` | Geometry shader **mesh** cần `MTLGPUFamily7_v2+`; GPU đo là Intel | `NOT_VERIFIED` (phần mesh GS). **Exit code = 0 ⇒ `ctest` báo `Passed`** — đây là ví dụ đúng cho quy tắc `SKIP ≠ PASS` |
| `test_mc_terrain_ubo_real` | *(không có)* — chỉ chứa chuỗi `ROW_LENGTH/SKIP` trong thông điệp PASS | không phải SKIP | `PASS` |

**Tổng số test SKIP thật: 1** (1/188 = 0.53 %).
**Không test nào SKIP vì thiếu GPU** — Metal device tồn tại và được dùng.

### 3.3 NOT_RUN

| Hạng mục | Số | Ghi chú |
|---|---:|---|
| Test CTest chưa chạy | **0** | đã chạy hết 188 |
| **Khronos CTS** | **toàn bộ** | `NOT_RUN` — xem `docs/CONFORMANCE_TESTING.md` §6 |
| Test trên **Apple A11** | **toàn bộ** | `NOT_RUN` — xem NV-01 |
| Test trên **iOS Simulator** | **toàn bộ** | `NOT_RUN` — xem NV-02 |
| **Image-regression** (so với bộ ảnh tham chiếu) | **toàn bộ** | `NOT_RUN` — chưa có bộ ảnh — xem NV-03 |
| Test **GL 1.0–2.1** theo spec (không CTS) | **toàn bộ** | `NOT_RUN` — xem NV-04 |
| Test **Compatibility Profile** có hành vi (GPU) | **toàn bộ** | `NOT_RUN` — chưa có đường draw fixed-function — xem NV-05 |

### 3.4 Điểm yếu của bộ test đo được (phát hiện khi kiểm baseline)

| Vấn đề | Đo | Ảnh hưởng |
|---|---|---|
| **1 file test còn dùng `assert()` thật** | `tests/unit/test_converter_aquarium.cpp:69` (`assert(u.uniformOffset + u.uniformSize <= vr.uniformBufferSize);`) | Trong build **Release** (`-DNDEBUG`) biểu thức bị xoá ⇒ test **không thể fail**. Vi phạm P3 của `docs/TESTING_STRATEGY.md`. Trạng thái: `INCORRECT` (cơ chế kiểm thử), không phải lỗi implementation |
| **Metric "test reference" chỉ là token match** | `tools/truth_manifest.py` đếm chuỗi `glFoo` trong file test | Một test chỉ *nhắc* `glFoo` cũng được tính là coverage cho `glFoo` ⇒ coverage **có thể bị thổi phồng** |
| **2 core function không có test trực tiếp** | `glHint`, `glReleaseShaderCompiler` (cả hai có thân `noop`, được `NOOP_ALLOWED` cho phép) | 2/657 core không có test |
| **18/42 entry point compat-only không có test trực tiếp** | `glColorP3uiv`, `glColorP4uiv`, `glMultiTexCoordP1uiv`, `glMultiTexCoordP2uiv`, `glMultiTexCoordP3ui`, `glMultiTexCoordP3uiv`, `glMultiTexCoordP4ui`, `glMultiTexCoordP4uiv`, `glNormalP3uiv`, `glSecondaryColorP3uiv`, `glTexCoordP1uiv`, `glTexCoordP2uiv`, `glTexCoordP3ui`, `glTexCoordP3uiv`, `glTexCoordP4ui`, `glTexCoordP4uiv`, `glVertexP3uiv`, `glVertexP4uiv` | 43 % bề mặt compat-only chưa có test hành vi |
| **Null backend ≠ Metal backend** | `build-null` (cũ) 185/185; `build-work` (Metal) 188/188 | MRT / upload / mip chain / compute grid chỉ được kiểm ở Metal, không ở CI Null |

### 3.5 Quan sát thêm: 24 entry point legacy **nằm ngoài bề mặt registry** và **không export**

Đo được:

```bash
$ python3 - <<'EOF'
import re, pathlib, json
md  = pathlib.Path("docs/OPENGL_PROFILE_MATRIX.md").read_text()
# ... lấy bảng rule L01-L20 + M01-M19 (như §4.4) ...
inv    = json.load(open("data/version_inventory.json"))
compat = set(inv["versions"]["4.6"]["commands"]["compatibility"])
for f in ("src/gl/gl_legacy_matrix.cpp", "src/gl/gl_legacy_vertex.cpp"):
    s = pathlib.Path(f).read_text()
    defs = sorted(set(re.findall(r'^(?:void|GLvoid|GLenum|GLuint|GLboolean) (gl[A-Za-z0-9_]+)\(', s, re.M)))
    print(f, len(defs), "defined;", [x for x in defs if x not in compat])
EOF
src/gl/gl_legacy_matrix.cpp 26 defined; ['glRotatedv', 'glRotatefv', 'glScaledv', 'glScalefv', 'glTranslatedv', 'glTranslatefv']
src/gl/gl_legacy_vertex.cpp 98 defined; ['glIndex1d', 'glIndex1dv', 'glIndex1i', 'glIndex1iv', 'glIndex1ui', 'glIndex1uiv',
 'glIndex2d', 'glIndex2dv', 'glIndex2f', 'glIndex2fv', 'glIndex2ui', 'glIndex2uiv', 'glIndex3d', 'glIndex3dv', 'glIndex3f',
 'glIndex3fv', 'glIndex3ui', 'glIndex3uiv']

$ nm -gU build-work/libtglmt.0.1.0.dylib | grep -E "_glTranslatefv|_glIndex1d"
(không có dòng nào ⇒ KHÔNG export)

$ grep -c "\bglTranslatefv\b\|\bglIndex1d\b" include/tglmt/gl_c_api.h include/tglmt/gl_legacy.h
0 / 0  ⇒ KHÔNG khai báo trong public C ABI
```

| Sự kiện | Số | Ý nghĩa |
|---|---:|---|
| Entry point legacy có định nghĩa trong `src/gl/` nhưng **không** nằm trong bề mặt 1048 của registry | 24 | Khronos registry **không** mô hình hoá các form `glIndex<n><type>` (GL 1.1) và các form mảng `glTranslatefv`/`glRotatefv`/`glScalefv` (GL 1.0) — chúng chỉ còn trong **văn bản** của đặc tả GL 1.x, không có trong `gl.xml` (đã kiểm: block `GL_VERSION_1_1` của `gl.xml` chỉ liệt kê `glIndexPointer`, `glIndexub`, `glIndexubv`) |
| Trong số đó, **không export** symbol C và **không** khai báo trong `gl_c_api.h` / `gl_legacy.h` | 24 | **Không client nào gọi được** ⇒ mã chết ở lớp C++ |

**Trạng thái:** `PARTIALLY_IMPLEMENTED` cho 24 entry point này (có code, không
reachable). **Không tính** vào bất kỳ cột coverage nào của §4 (vì bề mặt
đo là registry). Đề xuất xử lý: hoặc (a) export chúng qua C ABI + gate
theo version (chúng thuộc GL 1.1/1.0), hoặc (b) xoá khỏi `src/gl/` để tránh
mã chết. Cần quyết định của agent chính (ngoài write scope của PHASE 4).

---

## 4. Coverage baseline — GL 4.6 Core / Compatibility

### 4.1 Số lớn (lệnh §1 mục 3)

```
compat 1048 stub 240 impl 808
core   657 stub 0   impl 657
legacy 391 stub 240
```

| Bề mặt | Tổng | Còn stub `UNIMPLEMENTED` | Có định nghĩa thật |
|---|---:|---:|---:|
| GL 4.6 **Core** | **657** | **0** | **657** |
| GL 4.6 **Compatibility** | **1048** | **240** | **808** |
| — Trong đó: legacy fixed-function (compat − core) | **391** | **240** | **151** |

> **Cảnh báo diễn giải.** "Có định nghĩa thật" = có định nghĩa viết tay trong
> `src/gl/*.cpp`. **KHÔNG** đồng nghĩa hành vi đúng. Số hành vi đúng = số có
> test PASS + đối chiếu spec (xem §5).

### 4.2 Symbol export thật

```
$ nm -gU build-work/libtglmt.0.1.0.dylib | grep -c ' T _gl'
2208
```

| Mục | Số |
|---|---:|
| Symbol `gl*` export (`nm -gU`, type `T`) | **2208** |
| Lệnh GL 4.6 Core | 657 |
| Lệnh GL 4.6 Compatibility | 1048 |
| Bảng `getProcAddress` (core/legacy) | 1049 |
| Bảng `getProcAddress` (extension) | 1066 |

### 4.3 Ma trận 19 version × 2 profile

Sinh từ `data/version_inventory.json` (19 version) + `src/gl/gl_stubs_gen.cpp`.
GL 1.0–3.1 **không tách profile** (một bộ API duy nhất) theo
`docs/OPENGL_PROFILE_MATRIX.md` §1.

| Version | Profile | Lệnh (registry) | Còn stub | Có định nghĩa thật |
|---|---|---:|---:|---:|
| 1.0 | Legacy | 306 | 173 | 133 |
| 1.1 | Legacy | 336 | 189 | 147 |
| 1.2 | Legacy | 340 | 189 | 151 |
| 1.3 | Legacy | 386 | 206 | 180 |
| 1.4 | Legacy | 433 | 240 | 193 |
| 1.5 | Legacy | 452 | 240 | 212 |
| 2.0 | Legacy | 545 | 240 | 305 |
| 2.1 | Legacy | 551 | 240 | 311 |
| 3.0 | Legacy | 635 | 240 | 395 |
| 3.1 | Legacy | 647 | 240 | 407 |
| 3.2 | Core | 316 | 0 | 316 |
| 3.2 | Compatibility | 666 | 240 | 426 |
| 3.3 | Core | 344 | 0 | 344 |
| 3.3 | Compatibility | 724 | 240 | 484 |
| 4.0 | Core | 390 | 0 | 390 |
| 4.0 | Compatibility | 770 | 240 | 530 |
| 4.1 | Core | 478 | 0 | 478 |
| 4.1 | Compatibility | 858 | 240 | 618 |
| 4.2 | Core | 490 | 0 | 490 |
| 4.2 | Compatibility | 870 | 240 | 630 |
| 4.3 | Core | 534 | 0 | 534 |
| 4.3 | Compatibility | 913 | 240 | 673 |
| 4.4 | Core | 543 | 0 | 543 |
| 4.4 | Compatibility | 922 | 240 | 682 |
| 4.5 | Core | 653 | 0 | 653 |
| 4.5 | Compatibility | 1044 | 240 | 804 |
| 4.6 | Core | 657 | 0 | 657 |
| 4.6 | Compatibility | 1048 | 240 | 808 |

**Tổng 28 hàng (10 legacy + 9 × 2 profile).** Tổng lệnh (có trùng lặp giữa
các version): core 9 036 / compatibility 12 446; tổng stub tương ứng 2 197 /
4 357. Toàn bộ **240** stub của GL 4.6 nằm trong tập legacy.

### 4.4 Coverage theo subsystem — GL 4.6 Compatibility (1048 lệnh)

Phân loại first-match-wins theo bảng rule **L01–L20 + M01–M19** trích từ
`docs/OPENGL_PROFILE_MATRIX.md` §3.4 (script đọc **chỉ**
`data/version_inventory.json` + `src/gl/gl_stubs_gen.cpp`).
**Tổng khớp 1048 / 240 / 808 và 0 lệnh không phân loại được.**

| # | Subsystem | Lệnh | stub | định nghĩa thật |
|---|---|---:|---:|---:|
| L01 | Immediate mode (Begin/End + vertex specification) | 173 | 85 | 88 |
| L02 | Client-side vertex arrays | 12 | 12 | 0 |
| L03 | Fixed-function transform and matrix stack | 20 | 0 | 20 |
| L04 | Lighting, materials and shading model | 18 | 18 | 0 |
| L05 | Fog | 4 | 4 | 0 |
| L06 | Texture environment + texture coordinate generation | 15 | 15 | 0 |
| L07 | Display lists | 8 | 8 | 0 |
| L08 | Evaluators | 23 | 23 | 0 |
| L09 | Selection and feedback modes | 8 | 8 | 0 |
| L10 | Accumulation buffer | 2 | 2 | 0 |
| L11 | Pixel transfer modes + pixel drawing | 12 | 12 | 0 |
| L12 | Raster position + window position | 40 | 40 | 0 |
| L13 | Stipple (polygon + line) | 3 | 2 | 1 |
| L14 | Alpha test | 1 | 1 | 0 |
| L15 | Texture residency + priority | 2 | 2 | 0 |
| L16 | User clipping planes | 2 | 2 | 0 |
| L17 | Attribute stacks | 4 | 4 | 0 |
| L18 | Colour-index mode state | 2 | 2 | 0 |
| L19 | Compatibility-only vertex-type entry points (GL 3.3) | 30 | 0 | 30 |
| L20 | Compatibility-only n-suffixed queries (GL 4.5) | 12 | 0 | 12 |
| M01 | Buffer objects (§6) | 37 | 0 | 37 |
| M02 | Shaders, programs, program pipelines (§7) | 49 | 0 | 49 |
| M03 | Program interfaces, uniforms, uniform blocks (§7.3, §7.6) | 131 | 0 | 131 |
| M04 | Textures + texture objects (§8) | 90 | 0 | 90 |
| M05 | Sampler objects (§8.2) | 16 | 0 | 16 |
| M06 | Pixel storage modes + pixel rectangles (§8.4) | 3 | 0 | 3 |
| M07 | Framebuffer objects + renderbuffers (§9) | 46 | 0 | 46 |
| M08 | Vertex arrays, vertex attribs, VAO (§10.3) | 108 | 0 | 108 |
| M09 | Drawing commands + conditional rendering (§10.4, §10.9) | 33 | 0 | 33 |
| M10 | Rasterization + multisample state (§14) | 31 | 0 | 31 |
| M11 | Transform feedback (§13.3) | 16 | 0 | 16 |
| M12 | Per-fragment ops, blending, stencil, enable (§17) | 35 | 0 | 35 |
| M13 | Pixel read + copy (§18) | 3 | 0 | 3 |
| M14 | Compute shaders, images, memory barriers (§19) | 4 | 0 | 4 |
| M15 | Asynchronous queries (§4.2) | 19 | 0 | 19 |
| M16 | Sync objects + fences (§4.1) | 6 | 0 | 6 |
| M17 | Debug API + object labels (§20) | 11 | 0 | 11 |
| M18 | Context queries, strings, general state (§21, §22) | 16 | 0 | 16 |
| M19 | Miscellaneous general GL (§2, §17.4) | 4 | 0 | 4 |
| | **TỔNG** | **1048** | **240** | **808** |

**Nhóm 0/1048 định nghĩa thật (toàn bộ vẫn là stub): L02, L04, L05, L06, L07,
L08, L09, L10, L11, L12, L14, L15, L16, L17, L18** — tổng **148 lệnh**.
Cộng thêm L01 (85) + L13 (2) ⇒ **240 stub**.

### 4.5 Audit bề mặt (3 tool CI)

```
$ python3 tools/truth_manifest.py --lib build-work/libtglmt.dylib --strict
=== TGLMT truth manifest ===
library: build-work/libtglmt.dylib (found)
core functions: 657 | with symbol: 657 | missing: 0
core without direct test refs: 2
compat-profile-only functions (NOT core): 42 | missing symbol: 0
core no-op/constant bodies: 2 (0 static gaps, 2 allowed)
extension rows: 320 | advertised: 136
advertised missing symbols: 0
advertised no-op/constant: 0
advertised with no direct test refs: 75
state pnames not in gl.xml registry: 0
state-only (shadow-only) functions: 0
(exit = 0)

$ python3 tools/ext_matrix.py --lib build-work/libtglmt.dylib --strict
=== TGLMT extension truth matrix ===
core functions: 698
exported gl* symbols: 2208
glcore extensions: 246
rows (glcore + advertised non-glcore): 320
advertised (kExts): 136 total, 136 in rows
glcore with missing functions: 0 (0 functions)
glcore without missing functions: 246
  advertised       136
  symbol-complete  74
  behavior-only    110
(exit = 0)

$ python3 tools/ext_coverage.py --dlsym build-work/libtglmt.dylib
=== dlsym truth check: 136 advertised ext vs build-work/libtglmt.dylib ===

136 advertised, 0 thiếu symbol dlsym.
(exit = 0)
```

| Chỉ số | Số |
|---|---:|
| Core function thiếu symbol | 0 |
| Compat-only function thiếu symbol | 0 |
| Extension advertise thiếu symbol | 0 |
| Extension advertise có thân no-op/constant | 0 |
| Extension advertise không có test trực tiếp | 75 / 136 (55 %) |
| Core không có test trực tiếp | 2 / 657 |

---

## 5. Baseline rendering / shader / performance

### 5.1 Baseline **legacy** rendering

| Hạng mục | Trạng thái | Bằng chứng |
|---|---|---|
| Matrix stack (L03, 20 lệnh theo taxonomy) | `PARTIALLY_IMPLEMENTED` | `src/gl/gl_legacy_matrix.cpp` — **26** entry point định nghĩa (20 thuộc L03 + 6 form mảng ngoài registry, xem §3.5); test `test_legacy_matrix` **PASS** |
| Immediate mode + current vertex attribs (L01, 88/173 lệnh) | `PARTIALLY_IMPLEMENTED` | `src/gl/gl_legacy_vertex.cpp` — **98** entry point định nghĩa (80 thuộc L01 + 18 form `glIndex<n><type>` ngoài registry); test `test_legacy_immediate` **PASS** |
| **Đường draw fixed-function** | **`MISSING`** | Immediate mode đã gom đỉnh + ghép primitive nhưng **chưa nối vào Metal draw path** ⇒ **không render được hình fixed-function**. Không có ảnh legacy nào. |
| Lighting / material / fog / texenv / display list / evaluator / selection-feedback / pixel ops / accumulation | `MISSING` | 148 lệnh còn stub (§4.4) |
| **Baseline ảnh legacy** | **`NOT_VERIFIED`** | Không có ảnh, không có pixel nào. Lệnh đo: xem NV-06 |

### 5.2 Baseline **modern** rendering (pixel-level)

**64 / 188** file test đọc pixel (`glReadPixels`) ⇒ đây là bằng chứng mạnh
nhất hiện có. Phân bố theo nhóm so ảnh của `docs/TESTING_STRATEGY.md` §5.2
(rule first-match-wins, tái lập được):

| Nhóm so ảnh | Số file test đọc pixel |
|---|---:|
| rasterization | 17 |
| tessellation / GS | 10 |
| real-app (Minecraft) | 12 |
| framebuffer | 8 |
| texture | 6 |
| shading | 5 |
| depth / stencil | 1 |
| blend | 1 |
| compute | 1 |
| sync / query | 1 |
| perf | 1 |
| khác (`test_bake_repro_apple`) | 1 |
| **TỔNG** | **64** |

Các test pixel mạnh nhất (đã PASS, `OTHER_GPU`):

| Test | Nhóm | Điều kiện |
|---|---|---|
| `test_blend_apple` | blend | công thức blend SRC_ALPHA/ONE_MINUS_SRC_ALPHA đúng |
| `test_raster_apple` | rasterization | fill/wire/scissor/depth |
| `test_rop_apple` | ROP | 16/16 phép ROP |
| `test_msaa_apple` | rasterization | MSAA resolve |
| `test_texture_apple` | texture | upload + sampler + sample |
| `test_spirv_chain_apple` | shading | GLSL → SPIR-V → MSL → Metal → pixel |
| `test_converter_proof` | shading (đối chiếu) | so với `glslangValidator` + `spirv-cross` |
| `test_stencil_apple` | depth/stencil | stencil |
| `test_device_suite_apple` | nhiều | bộ thiết bị dùng chung host/iOS |
| `test_mc_*` (12 file) | real-app | workload Minecraft (LWJGL) |

> **Baseline ảnh tham chiếu: `NOT_VERIFIED`** — chưa có bộ ảnh, chưa có
> `tools/compare_images.py` (xem NV-03). 64 test pixel trên **so với giá trị
> kỳ vọng viết tay / công thức spec**, **không** phải so với ảnh tham chiếu.

### 5.3 Baseline biên dịch shader (`bench_compile`, 9 lần → median của median)

Máy: `OTHER_GPU`, `build-work` (Debug), backend Metal thật, device
`Intel(R) KBL Unknown`.

| Metric | median (ms) | min (ms) | max (ms) |
|---|---:|---:|---:|
| `vs_complex.glsl_to_msl` | 0.2459 | 0.2381 | 0.2947 |
| `vs_complex.msl_to_metal_library` | 0.0657 | 0.0595 | 0.0862 |
| `vs_complex.pipeline_creation` | 0.0085 | 0.0079 | 0.0125 |
| **`vs_complex.total`** | **0.3246** | 0.3075 | 0.4084 |
| `fs_complex.glsl_to_msl` | 0.7608 | 0.7314 | 0.8188 |
| `fs_complex.msl_to_metal_library` | 0.0778 | 0.0682 | 0.1119 |
| `fs_complex.pipeline_creation` | 0.0091 | 0.0082 | 0.0143 |
| **`fs_complex.total`** | **0.8619** | 0.8086 | 0.9388 |
| `cs_compute.glsl_to_msl` | 0.2982 | 0.2851 | 0.3507 |
| `cs_compute.msl_to_metal_library` | 0.0675 | 0.0572 | 0.0857 |
| `cs_compute.pipeline_creation` | 0.0062 | 0.0057 | 0.0090 |
| **`cs_compute.total`** | **0.3738** | 0.3498 | 0.4469 |

> **Baseline chỉ cho shader GLSL 4.60.** `glsl_to_msl.cpp` hiện chỉ hiểu
> GLSL 4.60 ⇒ **GLSL 1.10 → 3.30 chưa có baseline** (xem NV-07).
>
> **`msl_to_metal_library` là trình biên dịch Metal của Apple**, không phải
> TGLMT. Số này phụ thuộc cache hệ thống ⇒ khi so A/B phải dùng protocol
> 9 lần + median và min (xem `benchmarks/README.md`).

### 5.4 Baseline runtime (`bench_runtime`, 9 lần → median của median)

| Metric | median (µs) | min (µs) | max (µs) |
|---|---:|---:|---:|
| `glBindBuffer` | 0.110 | 0.110 | 0.145 |
| `glGetIntegerv` | 0.139 | 0.139 | 0.178 |
| `glUniform1f` | 0.231 | 0.231 | 0.290 |
| `glUniform1f_batched64` | 0.196 | 0.1957 | 0.2258 |
| `control_glGetIntegerv_batched64` | 0.1043 | 0.1038 | 0.1451 |
| `glBufferSubData_64B` | 0.288 | 0.285 | 0.368 |
| `glDrawArrays_3` | 11.580 | 11.395 | 12.785 |
| `glDrawElements_3` | 13.673 | 13.236 | 14.201 |
| `glClear` | 13.952 | 12.378 | 15.570 |
| `glTexImage2D_64x64_full_upload` | 217.084 | 207.113 | 266.696 |
| `glTexSubImage2D_64x64_staged` | 235.410 | 234.612 | 241.057 |
| **`frame_100_draws`** | **1369.977** | 1295.124 | 1450.886 |

**Phát hiện bất thường trong log benchmark (đo được):**

```
[TGLMT] glerr#1 code=0x502 <- _ZN5tglmt2gl12glBindBufferEjj <- ...
```

`glBindBuffer` sinh `GL_INVALID_OPERATION` (0x502) trong benchmark ⇒
**metric `glBindBuffer` có thể đang đo đường lỗi, không phải đường hợp lệ**.
Cần điều tra trước khi dùng làm baseline A/B. Ghi nhận, chưa sửa (ngoài write
scope).

### 5.5 Baseline upload (`bench_upload`, 9 lần → median của median)

| Metric | median (µs) | min (µs) | max (µs) |
|---|---:|---:|---:|
| `newBufferWithBytes_16KB` | 8.620 | 8.384 | 12.509 |
| `newTexture_64x64` | 10.876 | 10.539 | 15.735 |
| `managed_staging_plus_blit_macos_path` | 41.224 | 33.908 | 49.174 |
| `shared_replaceRegion_x1_ios_path` | 0.738 | 0.548 | 1.985 |
| `shared_replaceRegion_x2_old_buggy_path` | 1.932 | 1.370 | 4.223 |

> `shared_*` là **đường shared-storage kiểu iOS** được đo trên máy Intel ⇒
> cho số liệu *liên quan* tới A11 (đường chia sẻ bộ nhớ), **không** phải số
> đo A11. Nhãn vẫn là `OTHER_GPU`.

### 5.6 Baseline legacy (glBegin/glEnd, matrix ops, display list, lighting)

| Metric | Trạng thái |
|---|---|
| `glBegin/glEnd` overhead | **`NOT_VERIFIED`** — chưa có đường draw legacy (NV-06) |
| Immediate mode vertex conversion | `NOT_VERIFIED` |
| CPU primitive assembly | `PARTIALLY_IMPLEMENTED` (đã có, chưa nối draw path) |
| Display list compile / replay | `NOT_VERIFIED` — display list còn stub |
| Matrix operation | `PARTIALLY_IMPLEMENTED` — có code, chưa có benchmark |
| Fixed-function shader generation | `NOT_VERIFIED` — chưa có |
| Legacy state change | `NOT_VERIFIED` |
| Texture environment generation | `NOT_VERIFIED` |
| Lighting shader execution | `NOT_VERIFIED` |

---

## 6. Baseline theo lát (version × subsystem)

Trạng thái lát dùng đúng bộ §14.I. Quy tắc: `FULLY_IMPLEMENTED` cần đủ 4 mục
(code path `file:line` + test PASS + đối chiếu spec + nhãn môi trường).

| Lát | Trạng thái | Bằng chứng | Nhãn |
|---|---|---|---|
| **GL 4.6 Core — bề mặt API** | `FULLY_IMPLEMENTED` | `include/tglmt/gl_c_api.h` (**1049** prototype `extern "C"`) + `src/gl/gl_c_exports.cpp` (wrapper) + `tests/api/test_api_surface.cpp` PASS + đối chiếu `gl.xml` | `REFERENCE_ONLY` (test dùng backend Null) |
| **GL 4.6 Compat — bề mặt API** | `FULLY_IMPLEMENTED` | `include/tglmt/gl_legacy.h` (349 prototype) + `tools/truth_manifest.py --strict` exit 0 | `OTHER_GPU` |
| **Version-gate 19 version × 28 context** | `FULLY_IMPLEMENTED` | `src/gl/gl_version_gate_gen.cpp` (1048 entry) + `tests/unit/test_context_version.cpp` PASS (self-check 657/1048) | `REFERENCE_ONLY` |
| **GL 4.6 Core — hành vi runtime từng lệnh** | `PARTIALLY_IMPLEMENTED` | 64 test đọc pixel + 188 test CTest PASS; **chưa** phủ 657 lệnh | `OTHER_GPU` |
| **Legacy L03 — matrix stack (20 lệnh)** | `PARTIALLY_IMPLEMENTED` | `src/gl/gl_legacy_matrix.cpp` + `test_legacy_matrix` PASS; **chưa** nối draw path | `REFERENCE_ONLY` |
| **Legacy L01 — immediate mode (88/173)** | `PARTIALLY_IMPLEMENTED` | `src/gl/gl_legacy_vertex.cpp` + `test_legacy_immediate` PASS; **chưa** nối draw path | `REFERENCE_ONLY` |
| **Legacy L02, L04–L18 (148 lệnh)** | `STUB` | `src/gl/gl_stubs_gen.cpp` — `UNIMPLEMENTED` | `NONE` |
| **Legacy L19, L20 (42 lệnh)** | `PARTIALLY_IMPLEMENTED` | Có định nghĩa thật; 18/42 không có test trực tiếp | `NONE` |
| **GLSL 4.60** | `PARTIALLY_IMPLEMENTED` | `tests/shaders/test_shader_corpus.cpp` PASS + `bench_compile`; **chưa** phủ GLSL 1.10–3.30 | `OTHER_GPU` |
| **Geometry shader** | `BLOCKED` | Chưa có emulation (A6 bắt buộc emulation); `test_gs_mesh_apple` SKIP vì cần Apple7+ | `NONE` |
| **Image regression (bộ ảnh tham chiếu)** | `MISSING` | `tests/reference/` chưa tồn tại | `NONE` |
| **Khronos CTS** | `NOT_VERIFIED` | 0 case đã chạy | `NONE` |
| **Apple A11** | `NOT_VERIFIED` | Chưa có thiết bị | `NONE` |

---

## 7. Danh sách `NOT_VERIFIED` (kèm lệnh đo)

Mỗi mục: **không phải PASS, không phải FAIL**. Lệnh đo là lệnh sẽ tạo ra
bằng chứng khi chạy được.

| ID | Hạng mục | Trạng thái | Nhãn hiện tại | Lệnh đo / điều kiện |
|---|---|---|---|---|
| **NV-01** | Hành vi trên **Apple A11** (iPhone 8/X) | `NOT_VERIFIED` | `NONE` | Build `build-iphoneos` + cài lên iPhone 8/X + `ctest -j1` trên thiết bị; log phải chứa `MTLDevice.name` và `supportsFamily(MTLGPUFamily4_v1)`; nhãn chuyển thành `A11_DEVICE` |
| **NV-02** | Hành vi trên **iOS Simulator** | `NOT_VERIFIED` | `NONE` | `xcodebuild -sdk iphonesimulator` + `xcrun simctl spawn booted <ctest>`; nhãn `SIMULATOR` |
| **NV-03** | **Image regression** (so ảnh tham chiếu) | `NOT_VERIFIED` | `NONE` | Tạo `tools/gen_reference.py` + `tools/compare_images.py` + `tests/reference/<v>_<profile>/`; chạy `python3 tools/compare_images.py ref.png out.png --group shading` (thiết kế ở `docs/TESTING_STRATEGY.md` §6) |
| **NV-04** | Test **GL 1.0–2.1** theo spec (không có CTS) | `NOT_VERIFIED` | `NONE` | `ctest -R "test_gl1[0-9]_\|test_gl2[01]_"`. Cần tách `tests/unit/test_legacy_matrix.cpp` và `test_legacy_immediate.cpp` thành các case theo version, rồi viết test cho lighting/fog/texenv/display list/evaluator/pixel ops (§9.3 đặc tả) |
| **NV-05** | Test hành vi **Compatibility Profile** trên GPU | `NOT_VERIFIED` | `NONE` | Cần đường draw fixed-function (T1-S4) rồi `ctest -R test_compat_` trên backend `"apple"` |
| **NV-06** | **Baseline ảnh legacy** (fixed-function) | `NOT_VERIFIED` | `NONE` | Sau NV-05: `ctest -R test_legacy_render_apple` đọc pixel và so công thức spec §17.3.6 (lighting/fog/texenv) |
| **NV-07** | Baseline **GLSL 1.10 → 3.30** | `NOT_VERIFIED` | `NONE` | Sau khi `glsl_to_msl.cpp` hỗ trợ các version cũ: `cd build-work && for i in 1..9: ./bench_compile --repeats 30 --warmup 5` với shader `#version 110`…`330` |
| **NV-08** | **Khronos CTS** | `NOT_VERIFIED` | `NONE` | Sau khi có `tools/cts_shim/`: `third_party/VK-GL-CTS/executor/glcts --case KHR-GL46.* --deqp-log-file-name=…` (kế hoạch: `docs/CONFORMANCE_TESTING.md` §7) |
| **NV-09** | Baseline **Null backend** cho cây hiện tại | `NOT_VERIFIED` (chỉ có số của build cũ) | `REFERENCE_ONLY` | `cmake -S . -B build-null -DCMAKE_BUILD_TYPE=Release -DTGLMT_APPLE_METAL=OFF && cmake --build build-null -j8 && (cd build-null && ctest -j8)` ⇒ kỳ vọng `Total Tests: 188` |
| **NV-10** | **Memory leak** (ASan/LSan) trong CI | `NOT_VERIFIED` | `NONE` | `cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address" && cmake --build build-asan -j8 && (cd build-asan && ctest -j8)` (thư mục `build-asan/` có sẵn nhưng **không** phải gate) |
| **NV-11** | **Fuzzing / property testing** của lớp API | `NOT_VERIFIED` | `NONE` | Cần harness libFuzzer trên `tglmtGetProcAddress` + các entry point; chưa có |
| **NV-12** | `glBindBuffer` sinh `GL_INVALID_OPERATION` trong `bench_runtime` | `INCORRECT` (nghi vấn) | `OTHER_GPU` | Chạy `cd build-work && ./bench_runtime --iters 10 --warmup 1` và đọc dòng `[TGLMT] glerr#…`; tìm entry point trong `src/gl/gl_buffer.cpp` |
| **NV-13** | **Release build** có xanh giả không | `NOT_VERIFIED` | `NONE` | `cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release -DTGLMT_APPLE_METAL=ON && cmake --build build-rel -j8 && (cd build-rel && ctest -j8)` — cần xác nhận `tests/unit/test_converter_aquarium.cpp:69` **không** còn là điểm mù (`assert` bị xoá) |
| **NV-14** | Geometry shader **mesh** output | `NOT_VERIFIED` | `NONE` | `test_gs_mesh_apple` cần `MTLGPUFamily7_v2+`; chạy trên thiết bị Apple silicon mới hơn A11 |

---

## 8. Kết luận PHASE 4 (chỉ dựa trên số đo ở trên)

1. **Bộ test hiện tại xanh**: 188/188 PASS trên Metal thật (`OTHER_GPU`), 0 FAIL,
   1 SKIP (có lý do), 0 test chưa chạy.
2. **Bề mặt API đã đầy đủ về hình thức**: GL 4.6 Core 657 lệnh, Compatibility
   1048 lệnh — tất cả đều có prototype export; 240 lệnh legacy còn là stub
   `UNIMPLEMENTED` (trung thực, không giả pass).
3. **Khoảng trống lớn nhất**: (a) **chưa có đường draw fixed-function** ⇒
   **không render được hình legacy**; (b) **chưa chạy CTS**; (c) **chưa có
   bộ ảnh tham chiếu**; (d) **chưa đo trên Apple A11**.
4. **Không có claim nào về A11 hay về tuân thủ CTS** trong báo cáo này.
5. Baseline này là **điểm xuất phát** cho PHASE 5–10; mọi thay đổi implementation
   phải làm **giữ nguyên** 188/188 và **không giảm** các cột coverage ở §4.

---

## 9. Phụ lục — lệnh sinh từng bảng

| Bảng | Lệnh |
|---|---|
| §2.1 số test | `(cd build-work && ctest -N)` + đếm file trong `tests/{unit,integration,regression,api,conformance,shaders}` |
| §3.2 SKIP | `(cd build-work && ctest -j8 -V) \| grep -E "\bSKIP\b"` |
| §4.1 | script ở §1 mục 3 |
| §4.3 | cùng script, lặp trên 19 version × 2 profile trong `data/version_inventory.json` |
| §4.4 | bảng rule L01–L20 + M01–M19 trích từ `docs/OPENGL_PROFILE_MATRIX.md` §3.4, áp first-match-wins lên `compat` của GL 4.6 |
| §5.1 | `tools/truth_manifest.py` + `data/version_inventory.json` |
| §5.2 | đếm `glReadPixels` trong `tests/**/*.cpp`, gán nhóm theo rule first-match-wins |
| §5.3–§5.5 | `bench_compile` / `bench_runtime` / `bench_upload` × 9 + `benchmarks/compare/bench_median.py` |
