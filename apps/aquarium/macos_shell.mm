// macos_shell.mm — Shell macOS: cửa sổ + CAMetalLayer + vòng lặp benchmark.
// Render HOÀN TOÀN qua aquarium (OpenGL 4.6 → TGLMT). Shell chỉ: tạo layer,
// target, present, đo FPS. Hỗ trợ --bench-frames N (chạy N frame rồi in điểm + thoát).
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#include "aquarium.h"
#include "tglmt/Context.h"
#include "tglmt/MetalInterface.h"
#include "tglmt/gl46.h"
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

static int gBenchFrames = 0; // 0 = chạy liên tục
static std::string gShaderDir = "../apps/aquarium/shaders";
static std::string gShotPath; // rỗng = không chụp
static int gShotFrame = 30;
static bool gSelftest = false;

@interface AquariumView : NSView {
@public
    CAMetalLayer* mLayer;
    id<MTLDevice> device;
    tglmt::Context* ctx;
    std::shared_ptr<tglmt::metal::IRenderTarget> target;
    int vw, vh;
    tglmt::metal::PixelFormat tfmt;
    NSTimer* timer;
    std::chrono::steady_clock::time_point t0;
    double simTime;
    int frames;
}
- (void)startLoop;
- (void)tick;
@end

@implementation AquariumView
- (instancetype)initWithFrame:(NSRect)f {
    if ((self = [super initWithFrame:f])) {
        // Tạo CAMetalLayer trực tiếp (không dựa +layerClass — đã quan sát thất bại).
        CAMetalLayer* ml = [CAMetalLayer layer];
        [self setLayer:ml];
        [self setWantsLayer:YES];
        self->mLayer = ml;
        self->device = MTLCreateSystemDefaultDevice();
        self->mLayer.device = self->device;
        self->mLayer.pixelFormat = MTLPixelFormatRGBA8Unorm; // khớp target TGLMT
        self->mLayer.framebufferOnly = NO; // presentTarget blit vào drawable (TBDR cần NO)
        self->mLayer.displaySyncEnabled = NO; // benchmark không vsync (trung thực đo throughput)
        MTLPixelFormat actual = self->mLayer.pixelFormat;
        self->tfmt = tglmt::metal::PixelFormat::RGBA8Unorm;
        if (actual == MTLPixelFormatBGRA8Unorm) self->tfmt = tglmt::metal::PixelFormat::BGRA8Unorm;
        else if (actual == MTLPixelFormatBGRA8Unorm_sRGB)
            self->tfmt = tglmt::metal::PixelFormat::BGRA8Unorm_sRGB;
        self->mLayer.drawableSize = f.size;
        self->vw = (int)f.size.width;
        self->vh = (int)f.size.height;
        self->ctx = new tglmt::Context("apple");
        tglmt::Context::MakeCurrent(self->ctx);
        if (!aquarium::Init(gShaderDir, 48, 60)) {
            printf("Aquarium Init FAIL (shaderDir=%s)\n", gShaderDir.c_str());
            exit(1);
        }
        self->target = self->ctx->device->makeRenderTargetWithDepth(self->vw, self->vh,
            self->tfmt);
        self->ctx->device->setDefaultRenderTarget(self->target);
        self->t0 = std::chrono::steady_clock::now();
        self->simTime = 0;
        self->frames = 0;
    }
    return self;
}
- (void)startLoop {
    self->timer = [NSTimer scheduledTimerWithTimeInterval:0.0 target:self
        selector:@selector(tick) userInfo:nil repeats:YES];
}
- (void)tick {
    auto now = std::chrono::steady_clock::now();
    static auto last = now;
    double dt = std::chrono::duration<double>(now - last).count();
    last = now;
    if (dt <= 0) dt = 1.0 / 60;
    self->simTime += dt;
    // Resize: dựng lại target theo layer
    CGSize ds = self->mLayer.drawableSize;
    if ((int)ds.width != self->vw || (int)ds.height != self->vh) {
        self->vw = (int)ds.width; self->vh = (int)ds.height;
        self->target = self->ctx->device->makeRenderTargetWithDepth(self->vw, self->vh,
            self->tfmt);
        self->ctx->device->setDefaultRenderTarget(self->target);
    }
    tglmt::Context::MakeCurrent(self->ctx);
    if (gSelftest && self->frames == 5) {
        int r = 0, g = 0, b = 0;
        printf("[selftest] %s center=(%d,%d,%d)\n",
               aquarium::RunSelfTest(self->vw, self->vh, &r, &g, &b) ? "PASS" : "FAIL", r, g, b);
    } else if (!aquarium::RenderFrame(self->simTime, dt, self->vw, self->vh)) {
        printf("RenderFrame FAIL frame %d: %s\n", self->frames, aquarium::LastError());
    }
    if (!self->ctx->device->presentTarget(self->target.get(), (__bridge void*)self->mLayer)) {
        printf("present FAIL frame %d\n", self->frames);
    }
    self->frames++;
    double el = std::chrono::duration<double>(now - self->t0).count();
    if (self->frames % 60 == 0) {
        double fps = self->frames / el;
        printf("[bench] frame=%d fps=%.1f\n", self->frames, fps);
        NSWindow* w = self.window;
        w.title = [NSString stringWithFormat:@"AquariumBench (TGLMT Metal) — %.1f FPS", fps];
    }
    if (!gShotPath.empty() && self->frames == gShotFrame) {
        // Chụp framebuffer qua glReadPixels (đường GPU M5b) → file PPM
        int w = self->vw, h = self->vh;
        std::vector<unsigned char> px((size_t)w * h * 4);
        tglmt::Context::MakeCurrent(self->ctx);
        // glReadPixels qua TGLMT (namespace tglmt::gl)
        tglmt::gl::glReadPixels(0, 0, w, h, 0x1908, 0x1401, px.data());
        FILE* f = fopen(gShotPath.c_str(), "wb");
        if (f) {
            fprintf(f, "P6\n%d %d\n255\n", w, h);
            for (int r = h - 1; r >= 0; --r)
                for (int x = 0; x < w; ++x) {
                    unsigned char* p = px.data() + ((size_t)r * w + x) * 4;
                    fwrite(p, 1, 3, f);
                }
            fclose(f);
            printf("[shot] wrote %s (%dx%d)\n", gShotPath.c_str(), w, h);
        }
    }
    if (gBenchFrames > 0 && self->frames >= gBenchFrames) {
        printf("[SCORE] frames=%d seconds=%.2f avgFPS=%.2f\n", self->frames, el,
               self->frames / el);
        [NSApp terminate:nil];
    }
}
@end

int main(int argc, const char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--bench-frames" && i + 1 < argc) gBenchFrames = atoi(argv[++i]);
        if (a == "--shaders" && i + 1 < argc) gShaderDir = argv[++i];
        if (a == "--screenshot" && i + 1 < argc) gShotPath = argv[++i];
        if (a == "--shot-frame" && i + 1 < argc) gShotFrame = atoi(argv[++i]);
        if (a == "--selftest") gSelftest = true;
    }
    @autoreleasepool {
        [NSApplication sharedApplication];
        NSRect frame = NSMakeRect(0, 0, 960, 600);
        NSWindow* win = [[NSWindow alloc]
            initWithContentRect:frame
                      styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                 NSWindowStyleMaskResizable)
                        backing:NSBackingStoreBuffered
                          defer:NO];
        win.title = @"AquariumBench (TGLMT Metal)";
        AquariumView* view = [[AquariumView alloc] initWithFrame:frame];
        win.contentView = view;
        [win makeKeyAndOrderFront:nil];
        [win center];
        [view startLoop];
        [NSApp run];
    }
    return 0;
}
