# Embedding in an iOS launcher

Reference integration: `Natives/ctxbridges/tglmt_bridge.m` in the
Angel-Aura-Amethyst-iOS launcher (renderer key `libtglmt.dylib`).

## 1. Ship the dylib

Build the arm64 dylib (see `getting-started.md`) and embed as
`Frameworks/libtglmt.dylib`. It links only system frameworks
(Metal, Foundation, QuartzCore).

## 2. Renderer selection

Expose `libtglmt.dylib` in the renderer picker next to the existing
backends. On select, before JVM start:

- `dlopen(Frameworks/libtglmt.dylib, RTLD_GLOBAL)` so both the bridge and
  LWJGL resolve the same image.
- Set `org.lwjgl.opengl.libname` to the absolute Frameworks path. Absolute
  matters: a bare name lets a stale same-named file in the natives dir
  shadow the signed copy and `GL.create()` fails with `error=null`.
- Install a bridge table `{init, init_context, make_current, swap_buffers,
  swap_interval, terminate}` backed by the `TGLMT_*` C API. No EGL is
  involved on this path.

## 3. Surface rules

- Layer class must be `CAMetalLayer` (same as other Metal renderers).
- Set `layer.pixelFormat = RGBA8Unorm` — TGLMT refuses to present on
  mismatch (returns false, black screen if ignored). ANGLE needs BGRA8,
  so branch per renderer.
- `layer.framebufferOnly = NO` (blit present + readback path).
- Ensure non-zero `drawableSize` before creating the TGLMT window.
- Forward rotation/resize to `TGLMT_ResizeWindow`; otherwise presents
  letterbox into the old-size target.

## 4. Threading (phase 1)

One shared `Context`, serialized: the render thread and the chunk-uploader
thread each call `TGLMT_MakeCurrent` around their GL work and never draw
concurrently. True share-lists (parallel upload + render) are a later
stage; persistent-mapping flags are currently accepted but not fenced,
so throttle uploads on the launcher side.

## 5. First-launch checklist

1. `[TGLMT] dlopen OK` in the log; `HasRealGPU` true on device.
2. `drawsEncoded == drawsAttempted` after the first frames.
3. Pixels: clear color visible, then textured quad, then world.
4. On black screen, check in order: pixel-format match, non-zero layer
   size, program link log (`glGetProgramInfoLog`), then stats counters.
