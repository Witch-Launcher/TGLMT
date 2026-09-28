# Tham chiếu API

Hai mặt API. C++ cho app link static lib, C cho loader `dlsym` theo tên
trần (LWJGL) và cho bridge của launcher.

## API C++ (`tglmt::gl`, `tglmt::Renderer`)

Headers: `include/tglmt/gl46.h` (698 khai báo, sinh từ `gl.xml`),
`include/tglmt/Renderer.h`.

```cpp
tglmt::Renderer r;
bool ok = r.Init(backend, w, h, depth, fmt);
// backend: "apple" (GPU thật) hoặc "null" (trace CPU). depth: gắn depth
// buffer. fmt PHẢI bằng pixelFormat của CAMetalLayer.
bool frame = r.BeginFrame();   // MakeCurrent + default target (FBO 0)
bool shown = r.EndFrame(layer); // CAMetalLayer*; nullptr = headless
void r.Resize(w, h);           // dựng lại target ở BeginFrame kế tiếp
bool px = r.ReadPixels(x, y, w, h, out); // RGBA8, gốc bottom-left
bool gpu = r.hasRealGPU();
```

Mọi hàm `gl::` chỉ gọi sau `BeginFrame` trên thread gọi. Mỗi thread cần
`MakeCurrent` riêng; giai đoạn 1 dùng chung một `Context` nối tiếp nhau
(xem `launcher.md`).

### Context và state (headers trong `include/tglmt/`)

| Header | Giữ gì |
|---|---|
| `Context.h` | State GL mỗi thread: buffer, VAO, texture, shader, program, FBO, UBO binding, clear values, stats |
| `StateTracker.h` | Shadow capability/blend/depth/stencil/cull/viewport/scissor/clip-control để bake state Metal |
| `MetalInterface.h` | `IDevice`/`IBuffer`/`ITexture`/`IRenderPipeline`/`IRenderEncoder` + `CustomAttrib` + `DrawTrace` |
| `GLSLConverter.h` | Kết quả `ConvertGLSLtoMSL(src, stage)`: MSL + varyings/uniforms/samplers/UBO blocks |
| `GLFWShim.h` | Singleton `GLFWShim` đứng sau C API dưới đây |

## C API (ổn định cho `dlsym`)

Mọi entry `gl*` trong `gl46.h` đều export không-mangle từ `libtglmt.dylib`
(`src/gl/gl_c_exports.cpp`, file sinh — đừng sửa tay). Forward 1:1 sang
`tglmt::gl::*`.

Window API (`include/tglmt/GLFWShim.h`, `extern "C"`):

```c
TGLMT_Window* TGLMT_CreateWindow(uint32_t w, uint32_t h, const char* backend, int depth);
int  TGLMT_MakeCurrent(TGLMT_Window* w);
void TGLMT_ClearCurrent(void);
int  TGLMT_SwapBuffers(TGLMT_Window* w, void* metalLayer); // CAMetalLayer*
void TGLMT_SwapInterval(int interval);
void TGLMT_ResizeWindow(TGLMT_Window* w, uint32_t width, uint32_t height);
void* TGLMT_GetProcAddress(const char* name); // "glDrawElements", NULL nếu không có
int  TGLMT_HasRealGPU(void);
void TGLMT_DestroyWindow(TGLMT_Window* w);
```

Ghi chú hành vi: `SwapBuffers` xả draws đang chờ, present lên layer rồi bắt
đầu frame kế. `GetProcAddress` phủ tập vanilla (draw, buffer, VAO, texture,
shader, program, FBO, sync); ngoài tập đó trả NULL — link trực tiếp dylib
nếu cần đủ 701 symbols.
