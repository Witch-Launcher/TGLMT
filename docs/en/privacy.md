# TGLMT — Privacy Policy

TGLMT is an OpenGL 4.6 Core → Metal translation layer for iOS and macOS. It is MIT licensed.

## Data we collect

**None.** TGLMT is a rendering library. It does not collect, store, or transmit any personal data. There is no analytics, no telemetry, no advertising, and no user profiling.

## Local-only operation

TGLMT runs entirely on the host device. It:

- Translates OpenGL calls to Metal command buffers locally
- Compiles GLSL shaders to MSL locally
- Presents frames through CAMetalLayer (or headless readback)

No data leaves the device unless your application explicitly sends it.

## Logs

TGLMT may write diagnostic logs for debugging. These are local only and never uploaded by the library.

## Children

TGLMT collects no data from anyone, including children.

## Your rights

Because no data is collected, there is nothing to access, export or delete on our side. All rendering state lives in your application's process.

## Contact

Open an issue on the [TGLMT GitHub repository](https://github.com/Witch-Launcher/TGLMT).
