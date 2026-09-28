# Nhúng vào launcher iOS

Tham chiếu: `Natives/ctxbridges/tglmt_bridge.m` trong launcher
Angel-Aura-Amethyst-iOS (key renderer `libtglmt.dylib`).

## 1. Ship dylib

Build dylib arm64 (xem `getting-started.md`) và nhúng làm
`Frameworks/libtglmt.dylib`. Nó chỉ link framework hệ thống
(Metal, Foundation, QuartzCore).

## 2. Chọn renderer

Thêm `libtglmt.dylib` vào picker renderer cạnh các backend có sẵn. Khi
được chọn, trước khi start JVM:

- `dlopen(Frameworks/libtglmt.dylib, RTLD_GLOBAL)` để bridge và LWJGL
  resolve cùng một image.
- Đặt `org.lwjgl.opengl.libname` bằng đường tuyệt đối trong Frameworks.
  Tuyệt đối mới đúng: tên trần để file cùng tên cũ trong thư mục natives
  shadow bản đã ký và `GL.create()` chết với `error=null`.
- Dựng bảng bridge `{init, init_context, make_current, swap_buffers,
  swap_interval, terminate}` trên C API `TGLMT_*`. Đường này không qua EGL.

## 3. Quy tắc surface

- Layer class phải là `CAMetalLayer` (như các renderer Metal khác).
- Đặt `layer.pixelFormat = RGBA8Unorm` — TGLMT từ chối present khi lệch
  (trả false, bỏ qua là đen màn hình). ANGLE cần BGRA8 nên rẽ nhánh theo
  renderer.
- `layer.framebufferOnly = NO` (đường present bằng blit + readback).
- Đảm bảo `drawableSize` khác 0 trước khi tạo window TGLMT.
- Forward xoay/resize sang `TGLMT_ResizeWindow`; không là present letterbox
  trong target size cũ.

## 4. Threading (giai đoạn 1)

Một `Context` dùng chung, nối tiếp nhau: render thread và chunk-uploader
thread mỗi bên `TGLMT_MakeCurrent` quanh việc GL của mình, không draw đồng
thời. Share-lists song song thật (upload vừa render) để giai đoạn sau;
flag persistent-mapping hiện được chấp nhận nhưng chưa fence nên launcher
tự throttle upload.

## 5. Checklist lần chạy đầu

1. Log có `[TGLMT] dlopen OK`; `HasRealGPU` true trên máy thật.
2. Sau vài frame đầu `drawsEncoded == drawsAttempted`.
3. Pixel: thấy clear color, rồi textured quad, rồi world.
4. Màn đen thì kiểm tra theo thứ tự: pixel format khớp, layer size khác 0,
   log link program (`glGetProgramInfoLog`), rồi các counter stats.
