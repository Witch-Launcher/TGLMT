# MODERN_API_COVERAGE_REPORT.md — Coverage OpenGL 2.0–4.6 (programmable pipeline) — TGLMT

> **Mọi số liệu đo bằng script**, không viết tay. Lệnh đo kèm theo từng bảng.
>
> **Phạm vi:** OpenGL **2.0 → 4.6**, Core Profile + các extension liên quan
> (bề mặt **657** lệnh core của GL 4.6 + **136** extension được advertise).
>
> **Nhãn môi trường:** `OTHER_GPU` (Intel UHD Graphics 630, macOS 15.7.7) và
> `REFERENCE_ONLY` (backend Null). **KHÔNG phải `A11_DEVICE`**, không phải
> `SIMULATOR`. Xem `reports/FULL_AUDIT_REPORT.md` §0.
>
> **Không claim:** CTS · dTLS · "conformant" · "100 %".

---

> **Script đo:** bản script tự chứa được nhúng đầy đủ ở
> `reports/FULL_AUDIT_REPORT.md` §13 — trích ra
> `/tmp/dod_measure.py` rồi chạy như dưới đây. Không cần file ngoài repo.

## 0. Cách đo

```bash
cd /Volumes/project/TGLMT
(cd build-work && ctest -j8 -V > /tmp/tglmt_ctest_v.log 2>&1)

# bảng DoD + nhóm M01–M19 (dùng chung cả 3 báo cáo)
python3 /tmp/dod_measure.py --ctest-log /tmp/tglmt_ctest_v.log \
        --out /tmp/tglmt_dod_rows.json
python3 /tmp/dod_measure.py

# 3 cổng audit bề mặt
python3 tools/truth_manifest.py --lib build-work/libtglmt.0.1.0.dylib --strict
python3 tools/ext_matrix.py      --lib build-work/libtglmt.0.1.0.dylib --strict
python3 tools/ext_coverage.py     --dlsym build-work/libtglmt.0.1.0.dylib

# symbol export
nm -gU build-work/libtglmt.0.1.0.dylib | grep -c ' T _gl'
```

Định nghĩa vận hành của "đạt §14.1" (4 mục) — **giống hệt**
`reports/FULL_AUDIT_REPORT.md` §3.1:

1. **Code path thật** `file:line` — định nghĩa trong `src/gl/*.cpp` viết tay,
   thân `real`, không phải stub sinh tự động.
2. **Test ID PASS** — test **gọi thật** (`glXxx(`) **và** PASS trong `ctest -V`.
3. **Đối chiếu expected theo spec** — test có assertion giá trị neo theo spec.
4. **Nhãn môi trường** — ∈ {`OTHER_GPU`, `REFERENCE_ONLY`}, không phải `NONE`.

---

## 1. Bảng theo nhóm feature (GL 4.6 Core, 657 lệnh)

Nhóm lấy từ `docs/OPENGL_PROFILE_MATRIX.md` §3.4 (bảng rule **M01–M19**).

| Nhóm | Nhãn (chương spec) | Lệnh | Stub | Implement thật | Test ID chính | Đạt §14.1 | Trạng thái |
|---|---|---:|---:|---:|---|---:|---|
| **M01** | Buffer objects (§6) | 37 | 0 | 37 | `test_buffer_dsa_p5` (14), `test_core_buffer` (14), `test_core_program_framebuffer_query` (9), `test_mc_namedbuffer_apple` (9) | **27** | `PARTIALLY_IMPLEMENTED` |
| **M02** | Shaders, programs, program pipelines (§7) | 49 | 0 | 48 | `test_pipeline_sso` (20), `test_core_program_framebuffer_query` (19), `test_ssbo_graphics_apple` (17), `test_pipeline_draw_apple` (14) | **30** | `PARTIALLY_IMPLEMENTED` |
| **M03** | Program interfaces, uniforms, uniform blocks (§7.3, §7.6) | 131 | 0 | 131 | `test_core_uniform_entrypoints` (108), `test_sodium_fulldraw_apple` (7), `test_subroutine_api` (6) | **20** | `PARTIALLY_IMPLEMENTED` |
| **M04** | Textures + texture objects (§8) | 90 | 0 | 90 | `test_core_texture_sampler` (27), `test_compressed_texture` (14), `test_3d_texture_apple` (12) | **79** | `PARTIALLY_IMPLEMENTED` |
| **M05** | Sampler objects (§8.2) | 16 | 0 | 16 | `test_core_texture_sampler` (12), `test_core_program_framebuffer_query` (5) | **16** | `PARTIALLY_IMPLEMENTED` |
| **M06** | Pixel storage modes + pixel rectangles (§8.4) | 3 | 0 | 3 | `test_readpixels_region` (2) | **3** | `PARTIALLY_IMPLEMENTED` |
| **M07** | Framebuffer objects + renderbuffers (§9) | 46 | 0 | 46 | `test_core_program_framebuffer_query` (22), `test_renderbuffer` (16), `test_fbo_completeness` (10) | **40** | `PARTIALLY_IMPLEMENTED` |
| **M08** | Vertex arrays, vertex attribs, VAO (§10.3) | 108 | 0 | 108 | `test_core_vertex_attrib` (82), `test_core_raster_state` (21), `test_tess_vs_capture_apple` (10) | **104** | `PARTIALLY_IMPLEMENTED` |
| **M09** | Drawing commands + conditional rendering (§10.4, 10.9) | 33 | 0 | 33 | `test_core_draw` (12), `test_restart_xfb` (4), `test_conditional_render` (3) | **32** | `PARTIALLY_IMPLEMENTED` |
| **M10** | Rasterization + multisample state (§14) | 31 | 0 | 30 | `test_core_raster_state` (12), `test_bake_repro_apple` (5), `test_viewport_array` (5) | **27** | `PARTIALLY_IMPLEMENTED` |
| **M11** | Transform feedback (§13.3) | 15 | 0 | 15 | `test_restart_xfb` (8), `test_xfb_capture_apple` (6), `test_xfb_array_matrix_apple` (4) | **15** | `PARTIALLY_IMPLEMENTED` |
| **M12** | Per-fragment ops, blending, stencil, enable (§17) | 35 | 0 | 35 | `test_core_raster_state` (11), `test_mrt_apple` (9), `test_stencil_apple` (9) | **33** | `PARTIALLY_IMPLEMENTED` |
| **M13** | Pixel read + copy (§18) | 3 | 0 | 3 | `test_3d_texture_apple` (2), `test_copy_tex_image` (2) | **2** | `PARTIALLY_IMPLEMENTED` |
| **M14** | Compute shaders, images, memory barriers (§19) | 4 | 0 | 4 | `test_compute_dispatch` (3), `test_ssbo_graphics_apple` (1) | **3** | `PARTIALLY_IMPLEMENTED` |
| **M15** | Asynchronous queries (§4.2) | 19 | 0 | 19 | `test_core_query_sync` (12), `test_query_objects` (8), `test_query_samples_passed` (8) | **5** | `PARTIALLY_IMPLEMENTED` |
| **M16** | Sync objects + fences (§4.1) | 6 | 0 | 6 | `test_enum_behavior` (4), `test_framebuffer_state` (3) | **3** | `PARTIALLY_IMPLEMENTED` |
| **M17** | Debug API + object labels (§20) | 11 | 0 | 11 | `test_debug_p4` (10) | **11** | `PARTIALLY_IMPLEMENTED` |
| **M18** | Context queries, strings, general state (§21, 22) | 16 | 0 | 16 | `test_regression_gl_errors` (7), `test_core_program_framebuffer_query` (6) | **13** | `PARTIALLY_IMPLEMENTED` |
| **M19** | Miscellaneous general GL (§2, 17.4) | 4 | 0 | 4 | `test_core_buffer` (2) | **3** | `PARTIALLY_IMPLEMENTED` |
| | **TỔNG (M01–M19 = GL 4.6 core)** | **657** | **0** | **656** | | **466** | |

> **Ghi chú:** `glHint` và `glReleaseShaderCompiler` có thân **no-op**
> (`src/gl/gl_raster.cpp:273`, `src/gl/gl_readpixels_dsa.cpp:275`) — được
> `NOOP_ALLOWED` cho phép vì spec cho phép bỏ qua (hint). Vì vậy "implement
> thật" = 656, không phải 657. Cột "Stub" = 0: **không còn stub nào** trong
> GL 4.6 Core.

---

## 2. Bảng theo băng version 2.x / 3.x / 4.x

```bash
python3 /tmp/dod_measure.py   # mục "BAND TABLE"
```

| Băng | GLSL đi kèm | Lệnh (core tại version đầu băng, hợp nhất cả băng) | Stub | Implement thật | Đạt §14.1 | Trạng thái |
|---|---|---:|---:|---:|---:|---|
| **2.0–2.1** | 1.10 / 1.20 | 551 | 240 | 311 | **148** | `PARTIALLY_IMPLEMENTED` |
| **3.0–3.3** | 1.30 / 1.40 / 1.50 / 3.30 | 694 | 240 | 454 | **268** | `PARTIALLY_IMPLEMENTED` |
| **4.0–4.6** | 4.00 / 4.10 / 4.20 / 4.30 / 4.40 / 4.50 / 4.60 | 657 | 0 | 657 | **466** | `PARTIALLY_IMPLEMENTED` |

**Đọc bảng:** 240 stub của các băng 2.x và 3.x **toàn bộ là fixed-function**
(được loại khỏi core ở GL 3.2) — xem `reports/LEGACY_COVERAGE_REPORT.md`.
Băng 4.x **không còn stub nào** vì mọi lệnh mới của GL 4.0–4.6 đều là lệnh
programmable-pipeline đã có code.

### 2.1 Lệnh **mới được yêu cầu** ở từng version

```bash
python3 - <<'EOF'
import json
inv = json.load(open("data/version_inventory.json"))
for v in inv["versions"]:
    V = inv["versions"][v]
    nc = V["commands"]["new_core"]; nk = V["commands"]["new_compatibility"]
    if not nc and not nk: continue
    print("| %s | %s | %d | %d |" % (v, V.get("glsl_version") or "-", len(nc), len(nk)))
EOF
```

| Version | GLSL | Lệnh mới (core) | Lệnh mới (compat) |
|---|---|---:|---:|
| 1.0 | — | 306 | 306 |
| 1.1 | — | 30 | 30 |
| 1.2 | — | 4 | 4 |
| 1.3 | — | 46 | 46 |
| 1.4 | — | 47 | 47 |
| 1.5 | — | 19 | 19 |
| 2.0 | 1.10 | **93** | 93 |
| 2.1 | 1.20 | 6 | 6 |
| 3.0 | 1.30 | **84** | 84 |
| 3.1 | 1.40 | 12 | 12 |
| 3.2 | 1.50 | 19 | 19 |
| 3.3 | 3.30 | 28 | **58** (+30 compat-only `L19`) |
| 4.0 | 4.00 | **46** | 46 |
| 4.1 | 4.10 | **88** | 88 |
| 4.2 | 4.20 | 12 | 12 |
| 4.3 | 4.30 | **44** | 43 (`glGetPointerv` về lại core) |
| 4.4 | 4.40 | 9 | 9 |
| 4.5 | 4.50 | **110** | **122** (+12 compat-only `L20`) |
| 4.6 | 4.60 | 4 | 4 |

### 2.2 Feature theo version (đối chiếu §5.1–§5.3 của đặc tả)

| Feature | Version | Trạng thái | Bằng chứng |
|---|---|---|---|
| Shaders & programs (`glCreateShader`, `glLinkProgram`, `glUseProgram`) | 2.0 | `PARTIALLY_IMPLEMENTED` | `src/gl/gl_shader.cpp`, `src/gl/gl_program.cpp`; `test_pipeline_sso` PASS |
| VBO (`glBindBuffer`, `glBufferData`) | 1.5 | `PARTIALLY_IMPLEMENTED` | `src/gl/gl_buffer.cpp`; `test_core_buffer` PASS |
| VAO | 3.0 | `PARTIALLY_IMPLEMENTED` | `src/gl/gl_vertex.cpp`; `test_core_vertex_attrib` PASS |
| FBO / renderbuffer | 3.0 | `PARTIALLY_IMPLEMENTED` | `src/gl/gl_framebuffer.cpp`; `test_fbo_completeness` PASS |
| GLSL 1.10/1.20 | 2.0/2.1 | `PARTIALLY_IMPLEMENTED` | Xem §3 |
| GLSL 1.30/1.40/1.50/3.30 | 3.0–3.3 | `PARTIALLY_IMPLEMENTED` | Xem §3 — convert được nhưng **chưa** có test execution |
| Instancing | 3.3 | `PARTIALLY_IMPLEMENTED` | `glDrawArraysInstanced`/`glDrawElementsInstanced`; `test_vertex_instanced_apple` |
| Geometry shader | 3.2 | **`BLOCKED`** | A6 **bắt buộc** emulation; **chưa có**. `test_gs_mesh_apple` SKIP (cần `MTLGPUFamily7_v2+`). **Không** kết luận `HARDWARE_LIMITED` |
| Tessellation (TCS/TES) | 4.0 | `PARTIALLY_IMPLEMENTED` | `src/gl/gl_tess_xfb.cpp`; `test_tess_vs_capture_apple`, `test_tess_mrt_msaa_depth_apple` PASS |
| Compute shader + SSBO | 4.3 | `PARTIALLY_IMPLEMENTED` | `src/gl/gl_compute.cpp`; `test_compute_dispatch`, `test_ssbo_graphics_apple` PASS |
| Image load/store | 4.2 | `PARTIALLY_IMPLEMENTED` | `test_compute_dispatch`; converter hỗ trợ `imageLoad` (đo ở §3) |
| Atomic counters | 4.2 | `PARTIALLY_IMPLEMENTED` | `tglmt_atomicAdd` trong MSL sinh; đo ở §3 |
| Texture views | 4.3 | `PARTIALLY_IMPLEMENTED` | `test_mc_mipmap_views_apple` |
| Program pipeline + separate shader objects | 4.1/4.3 | `PARTIALLY_IMPLEMENTED` | `src/gl/gl_shader_objects_p8.cpp`; `test_shader_objects_p8` PASS |
| Transform feedback | 3.0/4.0 | `PARTIALLY_IMPLEMENTED` | `src/gl/gl_tess_xfb.cpp`; 5 test `test_xfb_*` PASS — **nhưng** replay path không re-bind attribute (`reports/CONFORMANCE_REPORT.md` §4) |
| SPIR-V ingestion (`GL_ARB_gl_spirv`) | 4.6 | `PARTIALLY_IMPLEMENTED` | `test_spirv_chain_apple` PASS (GLSL → SPIR-V → MSL → Metal → pixel) — nhưng **đường SPIR-V→GLSL→MSL** không phải đường chạy chính (A1) |

---

## 3. Bảng riêng cho GLSL

### 3.1 Version nào **compile** được (đo bằng converter thật)

```bash
clang++ -std=c++17 -O1 tools/glsl2msl_cli.cpp src/gl/glsl_to_msl.cpp \
        -Iinclude -o /tmp/glsl2msl
for v in 110 120 130 140 150 330 400 410 420 430 440 450 460; do
  printf "#version %s\nvoid main(){ gl_Position = vec4(0.0); }\n" "$v" > /tmp/t_$v.vert
  /tmp/glsl2msl /tmp/t_$v.vert vsh >/dev/null 2>&1 && echo "$v OK" || echo "$v FAIL"
done
```

| GLSL | OpenGL | `#version` | Compile (convert) | Metal-compile thật | Test execution | Trạng thái |
|---|---|---|---|---|---|---|
| **1.10** | 2.0 | **REJECTED** | — | — | — | **`MISSING`** |
| **1.20** | 2.1 | OK | **FAIL** (`varying` không hỗ trợ) | — | — | **`MISSING`** (phiên bản ngôn ngữ) |
| **1.30** | 3.0 | OK | OK | — | — | `PARTIALLY_IMPLEMENTED` |
| **1.40** | 3.1 | OK | OK | — | — | `PARTIALLY_IMPLEMENTED` |
| **1.50** | 3.2 | OK | OK | — | — | `PARTIALLY_IMPLEMENTED` |
| **3.30** | 3.3 | OK | OK | — | — | `PARTIALLY_IMPLEMENTED` |
| **4.00** | 4.0 | OK | OK | — | — | `PARTIALLY_IMPLEMENTED` |
| **4.10** | 4.1 | OK | OK | — | — | `PARTIALLY_IMPLEMENTED` |
| **4.20** | 4.2 | **REJECTED** | — | — | — | **`MISSING`** |
| **4.30** | 4.3 | OK | OK | — | — | `PARTIALLY_IMPLEMENTED` |
| **4.40** | 4.4 | OK | OK | — | — | `PARTIALLY_IMPLEMENTED` |
| **4.50** | 4.5 | OK | OK | — | — | `PARTIALLY_IMPLEMENTED` |
| **4.60** | 4.6 | OK | OK | **OK** (`xcrun metal`) | **OK** (pixel) | `PARTIALLY_IMPLEMENTED` |

**Tổng: 2 / 13 version `MISSING`, 11 / 13 `PARTIALLY_IMPLEMENTED`, 0 / 13 `FULLY_IMPLEMENTED`.**

### 3.2 Nguyên nhân gốc — bằng chứng `file:line`

`src/gl/glsl_to_msl.cpp:8482–8487`:

```cpp
if (t.find("300") == std::string::npos && t.find("460") == std::string::npos &&
    t.find("450") == std::string::npos && t.find("440") == std::string::npos &&
    t.find("410") == std::string::npos && t.find("400") == std::string::npos &&
    t.find("430") == std::string::npos &&
    t.find("330") == std::string::npos && t.find("150") == std::string::npos &&
    t.find("140") == std::string::npos && t.find("130") == std::string::npos &&
    t.find("120") == std::string::npos && t.find("100") == std::string::npos)
    return fail("phiên bản GLSL không nhận diện: " + t);
```

Đây là **so khớp chuỗi con**, không phải phân tích số version. Đo được:

```bash
python3 - <<'EOF'
import re, pathlib
t = pathlib.Path('src/gl/glsl_to_msl.cpp').read_text()
blk = re.search(r'if \(dir == "version"\) \{(.*?)\n                \}', t, re.S).group(1)
vers = re.findall(r't\.find\("(\d+)"\)', blk)
for v in ["100","110","120","130","140","150","300","330","400","410","420","430","440","450","460"]:
    hit = [x for x in vers if x in v]
    print("#version %-4s -> %s" % (v, "ACCEPTED" if hit else "REJECTED"))
EOF
```

| `#version` | Kết quả | Giải thích |
|---|---|---|
| **110** | **REJECTED** | Không khớp chuỗi con nào trong danh sách (`"110"` không chứa `"100"`) ⇒ GLSL **1.10 bị từ chối** dù nó là version hợp lệ của OpenGL 2.0 |
| **420** | **REJECTED** | Không khớp (`"420"` không chứa `"410"`/`"430"`) ⇒ GLSL **4.20 bị từ chối** dù nó là version hợp lệ của OpenGL 4.2 |
| 100 | ACCEPTED | Khớp `"100"` — nhưng **GLSL 1.00 không tồn tại** trong danh mục (số 1.10 là bản đầu). Không gây hại nhưng cho thấy bộ kiểm tra không phải là whitelist đúng |
| 120–460 (còn lại) | ACCEPTED | Khớp đúng một chuỗi con |

**Ngoài lỗi `#version`, còn thiếu hỗ trợ ngôn ngữ cũ.** Đo được: GLSL 1.20 dùng
`varying` / `attribute` / `gl_FragColor` bị từ chối:

```bash
printf '#version 120\nvarying vec4 c;\nvoid main(){ gl_FragColor = c; }\n' > /tmp/v120.frag
/tmp/glsl2msl /tmp/v120.frag fsh
# FAIL /tmp/v120.frag: câu lệnh không hỗ trợ: varying vec4 c;
```

⇒ để hỗ trợ GLSL ≤ 1.20 phải thêm: `attribute` → `in`, `varying` → `out`/`in`,
`gl_FragColor` → `out`, `texture2D` → `texture`, cùng hàng loạt khác biệt
1.10→1.30.

### 3.3 Đối chiếu thực thi (execution) — đo được

| Bằng chứng | Nội dung | Kết quả | Nhãn |
|---|---|---|---|
| `tests/shaders/test_shader_corpus.cpp` | 7 shader `#version 460 core` → convert → **`xcrun metal`** thật → `.air`; kiểm nội dung MSL (bắt buộc có `[[flat]]`, `bias(...)`, `[[color(1)]]`, `calculate_unclamped_lod`; bắt buộc **không** có mẫu cấm) | **PASS** | `OTHER_GPU` |
| `tests/integration/test_converter_proof.cpp` | **Differential** với reference `glslangValidator -G` + `spirv-cross`: so pixel ±5 **và** kiểm giá trị vật lý (r ≈ 216) | **PASS** | `OTHER_GPU` |
| `tests/integration/test_spirv_chain_apple.cpp` | GLSL → SPIR-V → MSL → Metal → pixel | **PASS** | `OTHER_GPU` |
| `tests/corpus/mc-26.1.2-shaders/` | **88/88** shader Minecraft đều `#version 330` | convert OK | `OTHER_GPU` |
| `tests/unit/test_converter_aquarium.cpp` | corpus shader lớn (Aquarium) | **PASS** | `OTHER_GPU` |
| GLSL **≠ 4.60** execution | — | **không có test nào** | `NOT_VERIFIED` |

**Kết luận §14.E:** GLSL 4.60 có **bằng chứng execution mạnh** (differential với
reference implementation). GLSL 1.30–4.50 **chỉ có bằng chứng compile**, chưa
có bằng chứng execution ⇒ `PARTIALLY_IMPLEMENTED`. GLSL 1.10 và 4.20
`MISSING`.

---

## 4. Bảng extension

### 4.1 Số đo

```bash
python3 tools/ext_matrix.py --lib build-work/libtglmt.0.1.0.dylib --strict
```
```
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
```

```bash
python3 tools/ext_coverage.py --dlsym build-work/libtglmt.0.1.0.dylib
```
```
=== dlsym truth check: 136 advertised ext vs build-work/libtglmt.0.1.0.dylib ===
136 advertised, 0 thiếu symbol dlsym.
```

| Chỉ số | Số | Ý nghĩa |
|---|---:|---|
| Hàng extension trong ma trận | **320** | 246 `glcore` + 74 non-glcore đã advertise |
| Extension **được advertise** | **136** | trong `kExts[]` (`src/gl/gl_error_string_debug.cpp:92`) |
| — `glcore` | 62 | phải hỗ trợ đúng theo spec |
| — non-glcore | 74 | tiện ích, không bắt buộc |
| Advertise **thiếu symbol** | **0** | mọi extension advertise đều resolve được bằng `dlsym` |
| Advertise có **thân no-op/constant** | **0** | không extension nào "advertise rồi làm rỗng" |
| Advertise **không có test trực tiếp** | **75 / 136** (55.1 %) | xem 4.4 |
| `symbol-complete` (không advertise, **có** đủ symbol) | **74** | có thể bật advertise khi đạt spec |
| `behavior-only` (không advertise, **không** có entry point nào) | **110** | extension chỉ thêm state/enum, không thêm hàm |

Phân bố theo vendor của 136 extension advertise (đo từ `kExts[]`):

| Vendor | Số |
|---|---:|
| ARB | 61 |
| EXT | 40 |
| OES | 15 |
| NV | 8 |
| KHR | 3 |
| ANGLE | 3 |
| APPLE | 2 |
| ATI | 2 |
| MESA | 1 |
| AMD | 1 |
| **TỔNG** | **136** |

### 4.2 "136 advertise · 0 thiếu symbol · 110 behavior-only" nghĩa là gì

Đây là **ba phép đo khác nhau**, không mâu thuẫn:

| Phép đo | Câu hỏi trả lời | Kết quả | **KHÔNG** nói được |
|---|---|---|---|
| **136 advertise** | "TGLMT có **string** extension nào trong `GL_EXTENSIONS`?" | 136 string trong `kExts[]` | ❌ Không nói hành vi extension đã đúng spec |
| **0 thiếu symbol** | "Mọi entry point mà extension đó **yêu cầu** đều export & dlsym được?" | 0 thiếu / 136 | ❌ Không nói hành vi đúng; chỉ nói **tồn tại** |
| **110 behavior-only** | "Extension nào **chưa** advertise mà lại **không có** entry point nào cần?" | 110 hàng | ❌ Không phải "110 extension chưa làm" |

**Cụ thể:**

- **74 hàng `symbol-complete`**: extension **chưa** advertise, nhưng TGLMT đã
  export **đủ** symbol mà extension đó yêu cầu (thường vì các symbol đó trùng
  với core/extension khác đã có). Bật advertise là **một dòng** trong `kExts[]`
  — **nhưng** chỉ được bật khi hành vi đạt spec (§4.5).
- **110 hàng `behavior-only`**: extension mà `gl.xml` **không yêu cầu entry
  point nào** (chỉ thêm state/enum/token). Không có "symbol để thiếu" ⇒ không
  thể dùng phép đo `dlsym` để kết luận gì về chúng. Với nhóm này, **phép đo
  đúng là hành vi state**, và đó mới là phần chưa có bằng chứng.
- **15 extension advertise có 0 entry point** (thuần state): `GL_ARB_depth_clamp`,
  `GL_ARB_conservative_depth`, `GL_ARB_explicit_uniform_location`,
  `GL_ARB_half_float_vertex`, `GL_ARB_map_buffer_alignment`,
  `GL_ARB_occlusion_query2`, `GL_ARB_texture_filter_anisotropic`,
  `GL_ARB_texture_mirror_clamp_to_edge`, `GL_ARB_texture_swizzle`,
  `GL_ARB_vertex_array_bgra`, `GL_EXT_texture_mirror_clamp_to_edge`,
  `GL_EXT_texture_swizzle`, `GL_EXT_vertex_array_bgra`, `GL_KHR_no_error`,
  `GL_OES_element_index_uint`.
  ⇒ Với 15 cái này, "0 thiếu symbol" là **điều kiện vô nghĩa**; bằng chứng
  phải là hành vi state/enum.

### 4.3 Phân rã 75 extension advertise không có test trực tiếp

```bash
python3 - <<'EOF'
import json
m = json.load(open("tests/truth/api_manifest.json"))
adv = [e for e in m["extensions"] if e["advertised"]]
notest = [e for e in adv if not e["tests"]]
print("advertised:", len(adv), " without test ref:", len(notest))
print("  of which 0 required funcs (state/enum only):",
      sum(1 for e in notest if e["funcs"] == 0))
print("  of which >0 required funcs:",
      sum(1 for e in notest if e["funcs"] > 0))
EOF
```

| Nhóm | Số | Trạng thái |
|---|---:|---|
| Extension chỉ thêm **state/enum** (0 entry point) | **15** | `NOT_VERIFIED` — cần test hành vi state |
| Extension có **≥1 entry point** nhưng chưa test | **60** | `NOT_VERIFIED` — cần test gọi thật entry point |

60 extension có entry point chưa có test (xếp theo số entry point):

| Extension | Entry point | Scope |
|---|---:|---|
| `GL_ARB_robustness` | 20 | glcore |
| `GL_EXT_disjoint_timer_query` | 12 | gl-only |
| `GL_EXT_draw_buffers_indexed` / `GL_OES_draw_buffers_indexed` | 8 + 8 | gl-only |
| `GL_EXT_texture_border_clamp` / `GL_OES_texture_border_clamp` | 8 + 8 | gl-only |
| `GL_APPLE_sync` | 7 | gl-only |
| `GL_EXT_occlusion_query_boolean` | 7 | gl-only |
| `GL_NV_non_square_matrices` | 6 | gl-only |
| `GL_ARB_draw_buffers_blend` | 4 | glcore |
| `GL_EXT_draw_elements_base_vertex` / `GL_OES_draw_elements_base_vertex` | 4 + 4 | gl-only |
| `GL_OES_vertex_array_object` | 4 | gl-only |
| 47 extension còn lại | 1–3 | gl-only / glcore |

**Lưu ý quan trọng:** phần lớn 60 extension này là **alias** — extension entry
point chỉ forward 1:1 sang hàm core đã có. `src/gl/gl_ext_aliases_gen.cpp`
(sinh bởi `tools/gen_ext_aliases.py`) chứa **578** alias definition. Vì vậy
"chưa có test trực tiếp" ở đây phần lớn là **hình thức**: hành vi đã được
kiểm qua đường core, nhưng **không có test nào gọi đúng tên entry point
extension** ⇒ `NOT_VERIFIED`, không phải `MISSING`.

Ví dụ kiểm chứng:

```bash
grep -n "glDrawBuffersARB" src/gl/gl_ext_aliases_gen.cpp
# 793:void glDrawBuffersARB(GLsizei a0, const GLenum * a1) { tglmt::gl::glDrawBuffers(a0, a1); }
```

### 4.4 Trạng thái của 61 extension advertise **có** test

Đo được: 61 / 136 extension advertise có ít nhất một file test nhắc tới
(`tests/truth/api_manifest.json` → `tests[]`). Ví dụ (10 / 61):
`GL_ARB_uniform_buffer_object` (10 entry point, `test_core_uniform_entrypoints`),
`GL_KHR_debug` (22, `test_debug_p4`), `GL_ARB_shader_objects` (39,
`test_shader_objects_p8`), `GL_ARB_sampler_objects` (14,
`test_core_texture_sampler`), `GL_ARB_vertex_attrib_binding` (6,
`test_core_vertex_attrib`), `GL_ARB_viewport_array` (12,
`test_viewport_array`), `GL_ARB_multi_bind` (6, `test_enum_behavior`).

### 4.5 Danh sách extension **chưa đạt spec** — quy tắc "chỉ advertise khi đúng spec"

Quy tắc (T4 của `other/prompt.md` §11.2, và `AGENTS.md`): **chỉ công bố
extension khi implementation thực sự đạt spec của extension đó.**

**Đánh giá hiện tại (đo được):**

| Tiêu chí | Số | Đạt? |
|---|---:|---|
| Extension advertise có đủ symbol theo `gl.xml` | 136 / 136 | ✅ |
| Extension advertise không có thân no-op | 136 / 136 | ✅ |
| Extension advertise có test trực tiếp | **61 / 136** | ⚠️ **75 chưa có** |
| Extension advertise có **bằng chứng hành vi theo spec** | **không đo được bằng script** | ❌ |

⇒ **Trạng thái của toàn bộ 136 extension advertise: `PARTIALLY_IMPLEMENTED`.**
Không extension nào đạt `FULLY_IMPLEMENTED` theo §14.1 vì §14.1 đòi hỏi
"đối chiếu với expected theo spec **hoặc reference implementation**" ở mức
từng extension — điều mà phép đo `dlsym` **không** đủ để chứng minh.

**Ba nhóm extension advertise cần xử lý theo thứ tự ưu tiên:**

| # | Nhóm | Số | Đề xuất |
|---|---|---:|---|
| 1 | Advertise, **có** entry point, **chưa** có test | **60** | Thêm test gọi đúng tên entry point extension (alias ⇒ rẻ). Đây là cách nhanh nhất để nâng số "advertised có test" từ 61 lên 121. |
| 2 | Advertise, **chỉ** state/enum, **chưa** có test hành vi | **15** | Cần test state/enum (vd `GL_ARB_texture_swizzle` ⇒ kiểm `GL_TEXTURE_SWIZZLE_RGBA` đọc lại đúng). |
| 3 | **Chưa** advertise nhưng đã đủ symbol (`symbol-complete`) | **74** | **KHÔNG** bật advertise chỉ vì có symbol. Phải kiểm spec từng cái (`docs/khronos/extensions/`) rồi mới thêm vào `kExts[]`. |

**Nhóm 3 — `symbol-complete` (74) — là nhóm cần thận trọng nhất.** Ví dụ:
`GL_ARB_texture_filter_anisotropic` — có symbol `glTexParameterf` (dùng chung
với core) nên "symbol-complete", nhưng spec yêu cầu hành vi anisotropic
filtering **và** các giá trị `GL_TEXTURE_MAX_ANISOTROPY_EXT` — điều mà phép
đo symbol **không** kiểm được. Bật advertise khi chưa có hành vi là **nói dối
app** ⇒ vi phạm đúng quy tắc của đặc tả.

---

## 5. Symbol export và bảng getProcAddress

```bash
nm -gU build-work/libtglmt.0.1.0.dylib | grep -c ' T _gl'
# 2208
```

| Mục | Số |
|---|---:|
| Symbol `gl*` export (`nm -gU`, type `T`) | **2208** |
| — trong đó là lệnh GL 4.6 Compatibility | 1048 |
| — trong đó là lệnh GL 4.6 Core | 657 (⊂ 1048) |
| — **không** nằm trong bề mặt GL 4.6 Compatibility | **1160** (alias extension + entry point chỉ có trong extension) |
| Bảng `getProcAddress` (`kTglmtProcTable`, `src/gl/gl_c_exports.cpp:7468`) | **2115** entry — **một** bảng duy nhất, gate theo (version, profile) tại `src/gl/gl_context_api.cpp:78-102` |
| `extern "C"` prototype trong `include/tglmt/gl_c_api.h` | 1049 |
| `extern "C"` prototype trong `include/tglmt/gl_legacy.h` | 349 |

Lệnh đo:

```bash
nm -gU build-work/libtglmt.0.1.0.dylib | grep -c ' T _gl'   # 2208
python3 - <<'EOF'
import re, pathlib
t = pathlib.Path('src/gl/gl_c_exports.cpp').read_text()
m = re.search(r'kTglmtProcTable\s*\[\]\s*=\s*\{(.*?)\n\};', t, re.S)
print("kTglmtProcTable entries:",
      len(re.findall(r'\{"(gl[A-Za-z0-9_]+)"', m.group(1))))
EOF
```

> **Khác biệt với `reports/BASELINE_TEST_REPORT.md` §4.2:** báo cáo đó ghi
> "bảng `getProcAddress` (core/legacy) 1049 / (extension) 1066". Đo lại trên
> cây hiện tại: chỉ có **một** bảng `kTglmtProcTable` với **2115** entry.
> Con số 1049 khớp với số prototype trong `gl_c_api.h`; 1066 không khớp với
> bất kỳ bảng nào đo được. Số **1049** là đúng, số **1066** không tái lập
> được ⇒ ghi ở đây là số đo thật.

Alias extension sinh tự động: **578** definition trong
`src/gl/gl_ext_aliases_gen.cpp` (sinh bởi `tools/gen_ext_aliases.py`; chỉ sinh
khi chữ ký khớp).

---

## 6. Test hiện có cho bề mặt modern

```bash
(cd build-work && ctest -N | tail -2)
#   Test #188: test_viewport_array
# Total Tests: 188
```

| Hạng mục | Số | Ghi chú |
|---|---:|---|
| Tổng test CTest | **188** | `tests/{unit,integration,regression,api,conformance,shaders}` |
| Kết quả | **188/188 PASS** | 0 FAIL, 1 SKIP (`test_gs_mesh_apple`) |
| Test có hậu tố `_apple` (cần Metal device) | 81 | |
| File test đọc pixel qua `glReadPixels` | **64** | bằng chứng mạnh nhất cho hành vi GPU |
| File test đọc pixel qua `readback` (Metal API trực tiếp) | 38 | (17 file dùng cả hai) |
| Test có assertion giá trị neo theo spec | **155 / 190** file test | đo bằng `dod_full.py` |

**Bằng chứng pixel mạnh nhất cho bề mặt modern (đã PASS):**
`test_blend_apple` (công thức blend), `test_raster_apple`,
`test_rop_apple` (16/16 phép ROP), `test_msaa_apple`,
`test_texture_apple`, `test_stencil_apple`, `test_spirv_chain_apple`,
`test_converter_proof` (differential vs `glslangValidator` + `spirv-cross`),
`test_mrt_apple`, `test_tess_vs_capture_apple`, `test_compute_dispatch`,
`test_ssbo_graphics_apple`, `test_xfb_capture_apple`,
`test_mdi_count_baseinstance_apple`, `test_device_suite_apple`,
`test_mc_*` (12 file, workload Minecraft).

> **Giới hạn của bằng chứng pixel:** 64 test đọc pixel so với **giá trị kỳ
> vọng viết tay / công thức spec**, **không** phải so với **bộ ảnh tham
> chiếu** — bộ ảnh đó chưa tồn tại (`tests/reference/` chưa có) ⇒ `NV-03`.

---

## 7. Tổng hợp §14.D / §14.E

| Mục | Yêu cầu | Kết quả đo | Trạng thái |
|---|---|---|---|
| **§14.D** Modern API | 2.0–4.6 đối chiếu + kiểm chứng đầy đủ | Bề mặt: **657/657** có prototype + định nghĩa thật (0 stub). Hành vi: **466/657** đạt §14.1 (**70.9 %**) | `PARTIALLY_IMPLEMENTED` |
| **§14.E** GLSL | Đúng version + xác minh execution | **11/13** version convert được; **2/13** `MISSING` (1.10, 4.20); execution chỉ có bằng chứng ở **4.60** | `PARTIALLY_IMPLEMENTED` |
| **Extension (T4)** | Chỉ advertise khi đúng spec | 136 advertise, 0 thiếu symbol, 0 no-op; **75 chưa có test**; **110 behavior-only** | `PARTIALLY_IMPLEMENTED` |

**Điểm số cần nhớ:**
- GL 4.6 Core: **657/657** có code thật, **466/657** đạt đủ 4 mục §14.1.
- Không có stub nào trong GL 4.6 Core.
- **0** extension advertise thiếu symbol; **0** extension advertise là no-op.
- **2** GLSL version trong phạm vi không compile được.
- **Không** có claim về CTS, A11, hay "conformant".

---

## 8. Danh sách `NOT_VERIFIED` / `BLOCKED` kèm lệnh đo

| ID | Hạng mục | Trạng thái | Lệnh đo / điều kiện |
|---|---|---|---|
| **MD-01** | **GLSL 1.10** không compile | `MISSING` | `printf '#version 110\nvoid main(){}\n' > /tmp/g.vert && /tmp/glsl2msl /tmp/g.vert vsh` — hiện FAIL *"phiên bản GLSL không nhận diện"*; sửa `src/gl/glsl_to_msl.cpp:8482-8487` thành so khớp **số** |
| **MD-02** | **GLSL 4.20** không compile | `MISSING` | `printf '#version 420\nvoid main(){}\n' > /tmp/g.vert && /tmp/glsl2msl /tmp/glsl2msl /tmp/g.vert vsh` — hiện FAIL; cùng nguyên nhân MD-01 |
| **MD-03** | **GLSL 1.20** `varying`/`attribute`/`gl_FragColor` | `MISSING` | `printf '#version 120\nvarying vec4 c;\nvoid main(){ gl_FragColor = c; }\n' > /tmp/v120.frag && /tmp/glsl2msl /tmp/v120.frag fsh` — hiện FAIL *"câu lệnh không hỗ trợ"* |
| **MD-04** | GLSL 1.30–4.50 **chưa** có test execution | `NOT_VERIFIED` | Thêm shader cùng version vào `tests/shaders/glsl/` + case trong `tests/shaders/test_shader_corpus.cpp` (hiện 7 file, đều `#version 460 core`) |
| **MD-05** | **Geometry shader** emulation | `BLOCKED` | A6 **bắt buộc** emulation. `test_gs_mesh_apple` SKIP (cần `MTLGPUFamily7_v2+`). Thiết kế: `docs/METAL_FEATURE_MAPPING.md` H19. **Không** kết luận `HARDWARE_LIMITED` |
| **MD-06** | **75 extension advertise** chưa có test | `NOT_VERIFIED` | `python3 - <<'EOF'` … (đo ở §4.3) rồi thêm test theo từng extension; ưu tiên 60 extension có entry point (alias ⇒ rẻ) |
| **MD-07** | **74 extension `symbol-complete`** chưa advertise | `NOT_VERIFIED` | Đọc spec từ `docs/khronos/extensions/`, kiểm hành vi, **rồi mới** thêm vào `kExts[]` (`src/gl/gl_error_string_debug.cpp:92`). **Không** bật chỉ vì có symbol |
| **MD-08** | `GL_EXTENSIONS` không lọc theo (version, profile) | `NOT_VERIFIED` | `TODO.md` T0.11 |
| **MD-09** | Texture **completeness** chưa có mô hình | `NOT_VERIFIED` | `reports/CONFORMANCE_REPORT.md` §5 mục 1 — cần mô hình đầy đủ; lệnh kiểm: tạo texture chỉ có level 0 + filter mipmap ⇒ phải sample `(0,0,0,1)` |
| **MD-10** | `std140`/`std430` **không** được honor | `NOT_VERIFIED` | `reports/CONFORMANCE_REPORT.md` §5 mục 4 — converter bỏ qua layout qualifier |
| **MD-11** | `glClientWaitSync` **không** chặn | `NOT_VERIFIED` | `reports/CONFORMANCE_REPORT.md` §5 mục 3 — cần `MTLSharedEvent` |
| **MD-12** | `GL_MAX_TEXTURE_IMAGE_UNITS` **over-report** (32 vs 31) | `INCORRECT` | `reports/CONFORMANCE_REPORT.md` §5 mục 5 — shader 32 sampler sẽ làm Metal abort |
| **MD-13** | `GL_COMPRESSED_TEXTURE_FORMATS` over-report trên thiết bị không có BC | `INCORRECT` | `reports/CONFORMANCE_REPORT.md` §5 mục 6 |
| **MD-14** | `INCOMPLETE_MULTISAMPLE` / `FRAMEBUFFER_UNSUPPORTED` **không** bao giờ trả về | `MISSING` | `reports/CONFORMANCE_REPORT.md` §4 — `test_fbo_completeness` chỉ kiểm 4 status |
| **MD-15** | Transform feedback: replay **không** re-bind attribute | `INCORRECT` | `reports/CONFORMANCE_REPORT.md` §4 — `test_xfb_*` PASS nhưng replay path thiếu re-bind |
| **MD-16** | **183 API** modern đạt mục 1–3 nhưng mất mục 4 (chỉ có test host-CPU) | `NOT_VERIFIED` | Chạy lại bộ test trên `build-work` (Metal) hoặc `build-null` (Null) rồi chạy lại `dod_full.py` |
| **MD-17** | `glBindBuffer` sinh `GL_INVALID_OPERATION` trong `bench_runtime` | `INCORRECT` (nghi vấn) | `cd build-work && ./bench_runtime --iters 10 --warmup 1` và đọc dòng `[TGLMT] glerr#…` |
| **MD-18** | **Release build** có xanh giả không | `NOT_VERIFIED` | `cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release -DTGLMT_APPLE_METAL=ON && cmake --build build-rel -j8 && (cd build-rel && ctest -j8)` — xác nhận `tests/unit/test_converter_aquarium.cpp:69` không còn là điểm mù |
| **MD-19** | A11 cho bất kỳ mục modern nào | `NOT_VERIFIED` | `build-iphoneos` + `ctest -j1` trên iPhone 8/X; nhãn → `A11_DEVICE` |
| **MD-20** | **CTS** | `NOT_VERIFIED` | `reports/CONFORMANCE_REPORT.md` §2, §7 |

---

## 9. Kết luận

1. **Bề mặt:** GL 4.6 Core **657/657** có prototype export + định nghĩa thật,
   **0** stub. Compatibility **1048/1048** có prototype.
2. **Hành vi (§14.1):** **466 / 657** (70.9 %) của GL 4.6 Core đạt đủ 4 mục.
   Nhóm mạnh nhất: M08 (104/108), M04 (79/90), M07 (40/46).
   Nhóm yếu nhất: M15 async queries (5/19), M16 sync (3/6), M03 uniforms
   (20/131), M02 programs (30/49).
3. **GLSL:** 11/13 version convert được; **2 version `MISSING`** (1.10, 4.20 —
   lỗi so khớp chuỗi con tại `src/gl/glsl_to_msl.cpp:8482`); execution chỉ
   có bằng chứng ở 4.60.
4. **Extension:** 136 advertise · **0** thiếu symbol · **0** no-op ·
   **75** chưa có test · 110 `behavior-only` (chưa advertise, không có entry
   point) · 74 `symbol-complete` (chưa advertise, đã đủ symbol — **không** bật
   advertise khi chưa đạt spec).
5. **Không có claim** về CTS, A11, conformance, hay 100 %.
