# Hiệu năng (gom lệnh đầy đủ — số liệu trung thực)

Mục tiêu: Minecraft Java vanilla trên iPhone A11+ (TBDR) và MacIntel (IMR),
cả hai đều Metal thật. Backend Null vẫn xanh trên CI (chỉ logic).

## Trước đây chậm ở đâu (1 GL → ~10-20 Metal, đã đo trước fix)

- `EmitDraw` tạo trace `IEncoder` VÀ encode GPU mỗi draw (double-encoder).
- Mỗi draw 2× `newBufferWithBytes` (uniforms) + N× UBO + viết lại index +
  build string pipeline-key + mutex, dù không có gì đổi.
- `StageBufferRange/TexRegion` flush encoder mở MỖI `SubData`, kể cả buffer
  không dùng trong pass → `encodersCreated ≈ draws`.
- Chẩn đoán trong hot-path: `fprintf` mỗi combo, readback GUI 256×256 TRONG
  draw (stall đồng bộ), scan 32k index mỗi draw, dump draw-state kèm readback.
- `blitCopy`/`generateMipmaps`/`CopyFB` dùng `commit+wait` trong frame.

## Đã sửa gì (bản này)

| Sửa | File | Counter |
|---|---|---|
| Bỏ trace encoder trên Apple | `src/gl/gl_draw.cpp` | `traceSkipped` |
| Conditional-flush staging | `gl_buffer/texture.cpp`, used-sets | `flushAvoided` |
| Ring triple-buffer 3×4MB, ổn định 0 alloc | `Context::RingAlloc`, uniforms/UBO/index | `ringAllocs`, `tempAllocs==0` |
| Cache pipeline-key, bỏ lookup bridge | `pendingPipeKey` | `pipelineLookupSkipped` |
| Cache uniform + dirty-check state | `uniformCache`, `pending*Valid` | `uniformReused`, `stateSkipped`, `depthReused` |
| Tách pass khi feedback hazard (đúng TBDR) | pre-scan + `hazardSplits` | `hazardSplits`, `hazardWarn` |
| Đếm batch MultiDraw | `glMultiDraw*` | `multidrawBatched` |
| Cổng chẩn đoán `TGLMT_DIAG=1` | mọi `fprintf`/`readback` nóng | `diagSkipped` |
| Validator bỏ qua sau warn đầu (cap 4k) | kiểm tra range | `rangeWarn` |
| Prewarm 16 depth states | `PrewarmPipelineForProgram` | `psoPrewarmed` |

## Chạy lại kiểm chứng

```sh
cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON && cmake --build build-apple -j8
./build-apple/test_perf_deferred_apple   # muốn: 100 draws -> 1 encoder, ring>0, tempFallback=0
./build-apple/test_ir_batch_apple
./build-apple/test_ir_full_apple
ctest --test-dir build-apple             # 42/42 xanh

# Release (Minecraft): không spam stderr, không stall readback
unset TGLMT_DIAG
# Debug trên máy: đủ dump + soi texture
TGLMT_DIAG=1 ./build-ios-pkg/run.sh 2> latestlog.txt
```

Kết quả `test_perf_deferred_apple` (MacIntel UHD 630, 2026-11-02, 5 lần):

```
batch100: enc=1 reuse=99 traceSkip=100 ring=2 tempFallback=0
          us=16171, 8075, 4276, 8344, 20420  (median 8344)
cond-flush: enc=1 flushAvoided>=5
used-flush: enc=2 (tách khi stage buffer đang dùng)
multidraw: enc=1 batched=5
mid=(255,0,0)
```

`ring=2` = draw đầu upload VS+FS một lần, 99 draws sau tái dùng (ổn định 0 alloc).
Số cũ `us=~2000-3000` không tái lập được trên host này; wall-clock hiện dao động
mạnh (4.3–20.4 ms). Test chỉ assert counter, chưa có performance gate.
`test_perf_draw_apple` đo median 31.6 µs/draw qua 5 lần (22.8–38.1 µs/draw).

## Phân tích "Vanilla nghẽ CPU, Sodium ổn" (đo từ log thật iPhone 8 Plus, 2026-11-10)

Số liệu lấy từ `latestlog.txt` (vanilla) và `latestlog 2.txt` (Sodium), cùng một
thiết bị, cùng một map, cùng interval 600 frame:

| | Vanilla | Sodium |
|---|---|---|
| thời gian/frame | **31.63 ms** (31.6 fps) | **31.51 ms** (31.7 fps) |
| GL call/frame | 927 | 598 |
| draw/frame | 72 | 51 |
| **trong GL calls (TGLMT)** | **12.30 ms/frame (39%)** | **13.13 ms/frame (42%)** |
| drawMs / clrMs / endMs | 7.64 / 3.22 / 0.69 ms | 8.19 / 2.93 / 0.66 ms |
| ngoài GL calls (GPU chờ + MC Java) | 19.33 ms/frame | 18.38 ms/frame |

KẾT LUẬN: **chi phí của TGLMT gần như BẰNG NHAU ở vanilla và Sodium**
(12.3 vs 13.1 ms/frame), và cả hai cùng tốc độ frame. Khác biệt người dùng
cảm nhận **KHÔNG phải do TGLMT CPU nặng hơn** — vanilla đơn giản là vẽ nặng
hơn (shader + số chunk) nên GPU A11 bị ngấn.

Trong 12.3 ms/frame mà TGLMT tiêu:
- `drawMs` 7.64 ms / 72 draw = **106 µs/draw**. Đo cùng pattern trên máy dev
  (Metal backend, 700 draw/frame, 4 texture, UBO đổi mỗi draw) → **3.4 µs/draw**.
  Chênh lệch 31× là do **A11 + Metal driver + GPU back-pressure**, không phải
  code TGLMT: profile cục bộ cho thấy phần lớn thời gian nằm trong
  `objc_msgSend` (Metal API), phần C++ của TGLMT rất nhỏ.
- `clrMs` 3.22 ms / 5 clear = 644 µs/clear: clear 2208×1242 color+depth → write
  ~11 MB ra DRAM mỗi lần. Đây là chi phí GPU thật, không tránh được (GL bắt buộc
  clear phải hiện dụng).
- 19.33 ms/frame NGOÀI GL calls = thời gian chờ GPU (present/back-pressure) +
  Java của MC. **Đây mới là phần "CPU 100%" mà launcher báo** — thread render
  bị kẹt trong Metal chờ GPU, không phải TGLMT tốn CPU.

Đã kiểm chứng không phải lỗi TGLMT (tất cả counter đều tốt):
`ringAllocs` 424/frame với `tempAllocs=0` (không cấp Metal buffer tạm),
`ringWrap` 0.2/frame, `bufWait` 0, `texWasRT` 0, `texWait` 0, `encNew` 7.9/frame
(≈8 render pass — đúng số pass MC cần), `pipeSkip` 58/frame (cache pipeline
đang hiệu lực), `uniformReused` gần 100%.

### Cách giảm tải (theo thứ tự hiệu quả trên A11)
1. **Giảm render distance / số chunk** (MC setting) — giảm trực tiếp GPU load.
2. **Giảm độ phân giải render** — launcher đang render ở 2208×1242 (1.25× logical).
   Xuống 0.75× → giảm ~56% pixel → giảm tương ứng GPU load.
3. **Dùng Sodium** — giảm draw + shader nhẹ hơn nhiều.

### Tối ưu TGLMT đã làm cho vấn đề này
- Dedup bind trong `AppleRenderEncoder` (`setVertexBuffer`/`setFragmentBuffer`/
  `set*Texture`/`set*SamplerState`): Metal GIỮ state giữa các draw trong cùng
  encoder → bind lại y hệt là lệnh thừa (mỗi lệnh = 1 message send tới
  driver). Cache SỐNG TRONG ENCODER nên tự đúng với vòng đời state Metal
  (encoder mới → state rỗng), không cần quản lý thủ công.

### Công cụ đo (không chạy trong CI)
`tools/bench_gl_calls.cpp` — đo µs/call từng lệnh GL trên backend Metal:
```sh
g++ -std=c++17 -O2 -I include -I build-apple/generated tools/bench_gl_calls.cpp \
    -o /tmp/bench -L build-apple -ltglmt -Wl,-rpath,$PWD/build-apple && /tmp/bench
```

## Cái gì CHƯA gom (trung thực)

- `glBufferData`/`glTexImage2D` alloc ngay; chỉ `SubData`/`TexSubImage` được stage.
- `glCopyTexSubImage`/`BlitFramebuffer` scale dùng CPU fallback + wait (hiếm).
- PSO compile đồng bộ ở draw đầu mỗi key lạ (ms). Async + fallback PSO là roadmap.
- Bung FAN/LOOP alloc `vector` mỗi draw (hiếm: vanilla dùng triangles).
- `ReadPixels`/`GetTexImage` flush + đợi (đúng spec §18.2).
