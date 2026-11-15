# Báo cáo benchmark dịch GLSL → MSL

**Nguồn số:** `benchmarks/baseline/translate.json` (trước tối ưu) và
`benchmarks/optimized/translate.json` (sau tối ưu) — sinh bằng
`./build-bench/bench_translate --repeats 9 --warmup 2`.

Phương pháp, phân rã phase và cổng đúng đắn: `docs/SHADER_TRANSLATION_PERFORMANCE.md`.

## Máy đo

| Mục | Giá trị |
|---|---|
| Hệ điều hành | macOS 15.7.7 (Darwin 24.6.0) |
| Kiến trúc | x86_64 (Intel) |
| Build | Release `-O3 -DNDEBUG`, `TGLMT_APPLE_METAL=ON` |
| Backend khi đo | Null (chỉ đo CPU — không đo Metal) |
| Corpus | 86 shader (79 Minecraft `.vsh`/`.fsh` + 7 của repo) |
| Protocol | warm-up 2, 9 lần chạy; báo **best-of-7** theo median của median |

## Kết quả

| Chỉ số | Trước tối ưu | Sau tối ưu | Cải thiện |
|---|---|---|---|
| Tổng thời gian dịch 86 shader — median của median (µs) | 63350.9 | 53347.7 | **−15.8%** |
| Tổng — min (µs) | 62182.5 | 52602.9 | **−15.4%** |
| µs / shader (median) | 736.6 | 620.3 | **−15.8%** |
| Throughput (source) | 2.013 MB/s | 2.380 MB/s | **+18.2%** |
| Shader dịch được | 86/86 | 86/86 | — |
| Byte nguồn / byte MSL | 131264 / 827826 | 131264 / 827826 | — |

## Shader chậm nhất (sau tối ưu, µs)

| Shader | Stage | µs |
|---|---|---|
| `rendertype_clouds.vsh` | vertex | 2307.6 |
| `terrain.fsh` | fragment | 2242.8 |
| `rendertype_end_portal.fsh` | fragment | 1697.9 |
| `rendertype_text.vsh` | vertex | 1696.5 |
| `entity.vsh` | vertex | 1696.0 |
| `rendertype_lines.vsh` | vertex | 1590.9 |
| `item.vsh` | vertex | 1553.5 |
| `rendertype_text_background.vsh` | vertex | 1467.6 |
| `rendertype_text_intensity.vsh` | vertex | 1452.9 |
| `transparency.fsh` | fragment | 1357.0 |

## Cổng đúng đắn

| Cổng | Kết quả |
|---|---|
| Corpus MSL byte-identical (`diff -rq` 86 file) | **0 khác biệt** |
| `ctest -j8` (toàn bộ suite) | **207/207 PASS** |

## Hạn chế

- Số đo **chỉ phần CPU** (dịch GLSL→MSL). Thời gian Metal
  (`newLibraryWithSource`) nằm ngoài phạm vi — xem `bench_compile`.
- Máy dev có nhiễu lớn; mọi so sánh đều chạy **đan xen** và lấy
  **best-of-7** để giảm ảnh hưởng nhiễu nền.
- Chưa đo trên **A11** (`A11_DEVICE`) — số đo trên x86_64 của máy dev.
