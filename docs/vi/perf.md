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

## Cái gì CHƯA gom (trung thực)

- `glBufferData`/`glTexImage2D` alloc ngay; chỉ `SubData`/`TexSubImage` được stage.
- `glCopyTexSubImage`/`BlitFramebuffer` scale dùng CPU fallback + wait (hiếm).
- PSO compile đồng bộ ở draw đầu mỗi key lạ (ms). Async + fallback PSO là roadmap.
- Bung FAN/LOOP alloc `vector` mỗi draw (hiếm: vanilla dùng triangles).
- `ReadPixels`/`GetTexImage` flush + đợi (đúng spec §18.2).
