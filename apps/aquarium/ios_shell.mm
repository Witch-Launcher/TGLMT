// ios_shell.mm — Shell iOS: UIWindow + UIView(CAMetalLayer) + CADisplayLink.
// Render HOÀN TOÀN qua aquarium (OpenGL 4.6 → TGLMT). Shell chỉ tạo layer,
// target, present, log FPS. Deploy: mở Xcode project do scripts/configure_ios.sh
// sinh ra, chọn Team + device, Run.
#import <UIKit/UIKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#include "aquarium.h"
#include "device_tests.h"
#include "tglmt/Context.h"
#include "tglmt/MetalInterface.h"
#include "tglmt/gl46.h"
#include <chrono>
#include <unistd.h>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

@interface AquariumView : UIView {
@public
    CAMetalLayer* mLayer;
    id<MTLDevice> device;
    tglmt::Context* ctx;
    std::shared_ptr<tglmt::metal::IRenderTarget> target;
    int vw, vh;
    tglmt::metal::PixelFormat tfmt;
    CADisplayLink* link;
    double simTime;
    double lastT;
    int frames;
    std::chrono::steady_clock::time_point t0;
    BOOL doSelftest;
    BOOL selftestDone;
    UIView* testOverlay;
    UILabel* testStatus;
    UIProgressView* testProgress;
    UITextView* testLogView;
    UIButton* testClose;
    BOOL testing;
}
- (void)startLoop;
- (void)tick;
- (void)runDeviceTests;
- (void)hideTestOverlay;
@end

// Log file Documents/aquarium.log (lấy qua Files app → gửi chẩn đoán).
// TrollStore app không có console nhìn được nên file là kênh duy nhất.
static FILE* gLogFile = nullptr;
static void InitLogFile() {
    if (gLogFile) return;
    NSArray* docs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* path = [docs[0] stringByAppendingPathComponent:@"aquarium.log"];
    gLogFile = fopen([path UTF8String], "w");
    if (gLogFile) {
        setvbuf(gLogFile, nullptr, _IONBF, 0);
        // TGLMT/Metal ghi chẩn đoán qua stderr; TrollStore không có console nên
        // đưa luôn stderr vào file để log gửi về đủ.
        dup2(fileno(gLogFile), STDERR_FILENO);
    }
}
static void LogF(const char* fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("%s\n", buf);
    if (gLogFile) {
        fprintf(gLogFile, "%s\n", buf);
        fflush(gLogFile);
    }
}

@implementation AquariumView
+ (Class)layerClass { return [CAMetalLayer class]; }
- (instancetype)initWithFrame:(CGRect)f {
    if ((self = [super initWithFrame:f])) {
        InitLogFile();
        LogF("[init] AquariumBench v1.4+log, frame %dx%d", (int)f.size.width, (int)f.size.height);
        self->mLayer = (CAMetalLayer*)self.layer;
        self->device = MTLCreateSystemDefaultDevice();
        LogF("[init] device: %s", self->device ? [[self->device name] UTF8String] : "(nil)");
        self->mLayer.device = self->device;
        self->mLayer.pixelFormat = MTLPixelFormatRGBA8Unorm;
        self->mLayer.framebufferOnly = NO; // presentTarget blit vào drawable (TBDR cần NO)
        // iOS có thể ép drawable về format khác (vd BGRA sRGB) — đọc lại format THẬT
        // rồi tạo target khớp, tránh present bị từ chối mỗi frame (màn hình đen).
        MTLPixelFormat actual = self->mLayer.pixelFormat;
        tglmt::metal::PixelFormat tfmt = tglmt::metal::PixelFormat::RGBA8Unorm;
        if (actual == MTLPixelFormatBGRA8Unorm) tfmt = tglmt::metal::PixelFormat::BGRA8Unorm;
        else if (actual == MTLPixelFormatBGRA8Unorm_sRGB)
            tfmt = tglmt::metal::PixelFormat::BGRA8Unorm_sRGB;
        else if (actual != MTLPixelFormatRGBA8Unorm)
            printf("[shell] drawable format lạ (%lu), dùng RGBA8Unorm\n", (unsigned long)actual);
        self->tfmt = tfmt;
        self.contentScaleFactor = 1.0; // benchmark pixel 1:1 (trung thực, không retina-scale)
        CGFloat w = f.size.width, h = f.size.height;
        self->mLayer.drawableSize = CGSizeMake(w, h);
        self->vw = (int)w;
        self->vh = (int)h;
        self->ctx = new tglmt::Context("apple");
        tglmt::Context::MakeCurrent(self->ctx);
        // Resolve shader dir robust: chuẩn (name+ofType) trước, rồi các fallback.
        // (IPA thủ công cũ đặt Resources/shaders, CMake mới đặt shaders/).
        NSString* shDir = [[NSBundle mainBundle] pathForResource:@"fish"
                                                          ofType:@"vert"
                                                     inDirectory:@"shaders"];
        if (shDir)
            LogF("[init] shaders via canonical lookup: shaders/");
        if (!shDir)
            shDir = [[NSBundle mainBundle] pathForResource:@"fish.vert"
                                                    ofType:nil
                                               inDirectory:@"shaders"];
        if (!shDir)
            shDir = [[NSBundle mainBundle] pathForResource:@"fish.vert"
                                                    ofType:nil
                                               inDirectory:@"Resources/shaders"];
        std::string sdir;
        if (shDir) {
            sdir = [[shDir stringByDeletingLastPathComponent] UTF8String];
        } else {
            NSString* res = [[NSBundle mainBundle] resourcePath];
            NSString* c1 = [res stringByAppendingPathComponent:@"shaders"];
            NSString* c2 = [res stringByAppendingPathComponent:@"Resources/shaders"];
            NSFileManager* fm = [NSFileManager defaultManager];
            BOOL isDir = NO;
            if ([fm fileExistsAtPath:c1 isDirectory:&isDir] && isDir)
                sdir = [c1 UTF8String];
            else if ([fm fileExistsAtPath:c2 isDirectory:&isDir] && isDir)
                sdir = [c2 UTF8String];
            else
                sdir = [c1 UTF8String]; // để Init báo lỗi rõ ràng kèm path
        }
        LogF("[init] shaderDir=%s", sdir.c_str());
        if (!aquarium::Init(sdir, 48, 60)) {
            LogF("[init] FAIL: %s", aquarium::LastError());
            return self;
        }
        LogF("[init] programs OK (7/7)");
        // Template aqua.cfg để user bisection từ xa (toggles object + selftest).
        // Ghi 1 lần nếu chưa có; đọc mask mỗi lần mở app.
        {
            NSArray* docs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask,
                                                               YES);
            NSString* cfg = [docs[0] stringByAppendingPathComponent:@"aqua.cfg"];
            NSFileManager* fm = [NSFileManager defaultManager];
            if (![fm fileExistsAtPath:cfg]) {
                NSString* tpl = @"# aqua.cfg: mask bit0 bg,1 sand,2 plants,3 fish,4 water,5 bubbles,6 hud\n"
                                  "# selftest=1: thay frame 5 bằng tam giác đỏ kiểm tra stack GL\n"
                                  "mask=255\nselftest=0\n";
                [tpl writeToFile:cfg atomically:YES encoding:NSUTF8StringEncoding error:nil];
                LogF("[cfg] wrote template %s", [cfg UTF8String]);
            }
            NSString* txt = [NSString stringWithContentsOfFile:cfg encoding:NSUTF8StringEncoding
                                                                      error:nil];
            unsigned mask = 0xFF;
            BOOL selftest = NO;
            for (NSString* rawLine in [txt componentsSeparatedByString:@"\n"]) {
                NSString* line = [rawLine stringByTrimmingCharactersInSet:
                                            [NSCharacterSet whitespaceCharacterSet]];
                if ([line hasPrefix:@"mask="]) mask = (unsigned)[[line substringFromIndex:5] intValue];
                if ([line hasPrefix:@"selftest="])
                    selftest = [[line substringFromIndex:9] intValue] != 0;
            }
            aquarium::SetObjectMask(mask);
            // Dump MSL đã convert của 7 programs ra Documents (đối chiếu mac/iOS).
            {
                NSArray* docs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                                   NSUserDomainMask, YES);
                std::string ddir = [docs[0] UTF8String];
                for (int i = 0; i < 7; ++i) {
                    unsigned pid = aquarium::ProgramId(i);
                    if (pid && !aquarium::DumpProgramMSL(pid, ddir))
                        LogF("[msl] dump prog%d FAIL: %s", i, aquarium::LastError());
                }
                LogF("[msl] dumped 7 programs to Documents");
            }
            LogF("[cfg] mask=%u selftest=%d", mask, (int)selftest);
            self->doSelftest = selftest;
        }
        self->target = self->ctx->device->makeRenderTargetWithDepth(self->vw, self->vh,
            self->tfmt);
        if (!self->target) {
            LogF("[init] FAIL: target nil (%dx%d fmt=%d)", self->vw, self->vh, (int)self->tfmt);
            return self;
        }
        LogF("[init] target %dx%d fmt=%d OK", self->vw, self->vh, (int)self->tfmt);
        self->ctx->device->setDefaultRenderTarget(self->target);
        self->simTime = 0;
        self->lastT = CACurrentMediaTime();
        self->frames = 0;
        self->t0 = std::chrono::steady_clock::now();
        self->testing = NO;

        // Overlay test: hiện tiến trình + log ngay trên màn hình, không phụ thuộc Metal.
        self->testOverlay = [[UIView alloc] initWithFrame:self.bounds];
        self->testOverlay.autoresizingMask = UIViewAutoresizingFlexibleWidth |
                                             UIViewAutoresizingFlexibleHeight;
        self->testOverlay.backgroundColor = [UIColor colorWithWhite:0.0 alpha:0.78];
        UILabel* title = [[UILabel alloc] initWithFrame:CGRectMake(18, 26, self.bounds.size.width - 36, 28)];
        title.text = @"TGLMT device tests";
        title.textColor = [UIColor whiteColor];
        title.font = [UIFont boldSystemFontOfSize:18];
        [self->testOverlay addSubview:title];
        self->testStatus = [[UILabel alloc] initWithFrame:CGRectMake(18, 58, self.bounds.size.width - 36, 24)];
        self->testStatus.text = @"Đang chờ bắt đầu...";
        self->testStatus.textColor = [UIColor colorWithWhite:0.85 alpha:1.0];
        self->testStatus.font = [UIFont systemFontOfSize:13];
        [self->testOverlay addSubview:self->testStatus];
        self->testProgress = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
        self->testProgress.frame = CGRectMake(18, 90, self.bounds.size.width - 36, 4);
        self->testProgress.progress = 0.0f;
        [self->testOverlay addSubview:self->testProgress];
        self->testLogView = [[UITextView alloc] initWithFrame:CGRectMake(12, 108, self.bounds.size.width - 24, self.bounds.size.height - 156)];
        self->testLogView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        self->testLogView.backgroundColor = [UIColor colorWithWhite:0.0 alpha:0.35];
        self->testLogView.textColor = [UIColor colorWithWhite:0.92 alpha:1.0];
        self->testLogView.font = [UIFont fontWithName:@"Menlo" size:9.0];
        self->testLogView.editable = NO;
        self->testLogView.selectable = YES;
        self->testLogView.text = @"";
        [self->testOverlay addSubview:self->testLogView];
        self->testClose = [UIButton buttonWithType:UIButtonTypeSystem];
        self->testClose.frame = CGRectMake(self.bounds.size.width - 110, self.bounds.size.height - 40, 92, 30);
        [self->testClose setTitle:@"Đóng" forState:UIControlStateNormal];
        [self->testClose addTarget:self action:@selector(hideTestOverlay) forControlEvents:UIControlEventTouchUpInside];
        self->testClose.hidden = YES;
        [self->testOverlay addSubview:self->testClose];
        [self addSubview:self->testOverlay];
    }
    return self;
}
- (void)startLoop {
    self->link = [CADisplayLink displayLinkWithTarget:self selector:@selector(tick)];
    [self->link addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
    [self runDeviceTests];
}
- (void)tick {
    double now = CACurrentMediaTime();
    if (self->testing) { self->lastT = now; return; }
    double dt = now - self->lastT;
    self->lastT = now;
    if (dt <= 0 || dt > 0.25) dt = 1.0 / 60;
    self->simTime += dt;
    CGSize ds = self->mLayer.drawableSize;
    CGFloat sc = self.contentScaleFactor;
    CGSize bs = self.bounds.size;
    int nw = (int)(bs.width * sc), nh = (int)(bs.height * sc);
    if (nw != self->vw || nh != self->vh) {
        self->vw = nw; self->vh = nh;
        self->mLayer.drawableSize = CGSizeMake(nw, nh);
        self->target = self->ctx->device->makeRenderTargetWithDepth(nw, nh,
            self->tfmt);
        self->ctx->device->setDefaultRenderTarget(self->target);
        (void)ds;
    }
    tglmt::Context::MakeCurrent(self->ctx);
    if (self->doSelftest && !self->selftestDone && self->frames == 5) {
        int r = 0, g = 0, b = 0;
        BOOL ok = aquarium::RunSelfTest(self->vw, self->vh, &r, &g, &b);
        LogF("[selftest] %s center=(%d,%d,%d) (expect red>200)", ok ? "PASS" : "FAIL", r, g, b);
        self->selftestDone = YES;
    } else if (!aquarium::RenderFrame(self->simTime, dt, self->vw, self->vh)) {
        LogF("[frame %d] RenderFrame FAIL: %s", self->frames, aquarium::LastError());
    }
    if (!self->ctx->device->presentTarget(self->target.get(), (__bridge void*)self->mLayer)) {
        static int presentFails = 0;
        presentFails++;
        if (presentFails <= 3 || presentFails % 60 == 0)
            LogF("[frame %d] present FAIL x%d (layer drawableSize %.0fx%.0f, target %dx%d)",
                 self->frames, presentFails, self->mLayer.drawableSize.width,
                 self->mLayer.drawableSize.height, self->vw, self->vh);
    }
    self->frames++;
    if (self->frames == 30) {
        // Chụp frame RAW về đối chiếu (Documents/frame{W}x{H}.rgba, lấy qua Files app).
        int w = self->vw, h = self->vh;
        std::vector<unsigned char> px((size_t)w * h * 4);
        tglmt::Context::MakeCurrent(self->ctx);
        tglmt::gl::glReadPixels(0, 0, w, h, 0x1908, 0x1401, px.data());
        NSArray* docs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString* path = [NSString stringWithFormat:@"%@/frame%dx%d.rgba", docs[0], w, h];
        FILE* f = fopen([path UTF8String], "wb");
        if (f) {
            // Ghi kèm header text để bên nhận biết kích thước (8 byte đầu "RGBA"+w+h).
            uint32_t hdr[3] = {0x41424752, (uint32_t)w, (uint32_t)h};
            fwrite(hdr, 1, 12, f);
            fwrite(px.data(), 1, px.size(), f);
            fclose(f);
            LogF("[shot] wrote %s (%dx%d)", [path UTF8String], w, h);
        } else {
            LogF("[shot] FAIL fopen");
        }
    }
    if (self->frames % 60 == 0) {
        double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - self->t0).count();
        auto& st = self->ctx->appleStats;
        LogF("[bench] frame=%d fps=%.1f (%dx%d) GL %llu/%llu P%llu T%llu M%llu [%s]",
             self->frames, self->frames / el, self->vw, self->vh,
             (unsigned long long)st.drawsEncoded, (unsigned long long)st.drawsAttempted,
             (unsigned long long)st.noPipeline, (unsigned long long)st.noTarget,
             (unsigned long long)st.miscFail, aquarium::ProgStatsString());
    }
}

- (void)hideTestOverlay {
    self->testOverlay.hidden = YES;
}

- (void)appendTestLine:(NSString*)line progress:(float)progress done:(BOOL)done summary:(NSString*)summary {
    if (line.length) {
        NSString* old = self->testLogView.text ?: @"";
        self->testLogView.text = [old stringByAppendingFormat:@"%@\n", line];
        NSRange end = NSMakeRange(self->testLogView.text.length, 0);
        [self->testLogView scrollRangeToVisible:end];
    }
    self->testProgress.progress = progress;
    if (done) {
        self->testStatus.text = summary.length ? summary : @"TEST XONG — gửi file log";
        self->testClose.hidden = NO;
    }
}

- (void)runDeviceTests {
    self->testing = YES;
    __weak AquariumView* weakSelf = self;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        @autoreleasepool {
            NSArray* docs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                               NSUserDomainMask, YES);
            NSString* tpath = [docs[0] stringByAppendingPathComponent:@"tglmt-device-tests.log"];
            FILE* tf = fopen([tpath UTF8String], "w");
            UIDevice* ud = [UIDevice currentDevice];
            if (tf) {
                fprintf(tf, "device=%s model=%s iOS=%s\n",
                        [[ud name] UTF8String], [[ud model] UTF8String],
                        [[ud systemVersion] UTF8String]);
                fflush(tf);
            }
            int checks = 0;
            int fails = 0;
            auto sink = [tf, weakSelf, &checks, &fails](const char* raw) {
                NSString* line = [NSString stringWithUTF8String:raw ? raw : ""];
                if ([line hasPrefix:@"PASS "] || [line hasPrefix:@"FAIL "] ||
                    [line hasPrefix:@"SKIP "]) {
                    ++checks;
                    if ([line hasPrefix:@"FAIL "]) ++fails;
                }
                if (tf) { fprintf(tf, "%s\n", raw ? raw : ""); fflush(tf); }
                BOOL done = [line hasPrefix:@"=== SUMMARY"];
                float p = done ? 1.0f : MIN(1.0f, (float)checks / 30.0f);
                dispatch_async(dispatch_get_main_queue(), ^{
                    AquariumView* me = weakSelf;
                    if (!me) return;
                    LogF("[test] %s", raw ? raw : "");
                    [me appendTestLine:line progress:p done:done summary:line];
                });
            };
            std::string summary;
            tglmt::Context* tc = new tglmt::Context("apple");
            if (!tc || tc->device->isNull()) {
                summary = "SKIP no Metal device";
                sink("SKIP no Metal device");
            } else {
                tglmt::Context::MakeCurrent(tc);
                summary = RunDeviceTests(*tc, sink);
                tglmt::Context::MakeCurrent(nullptr);
            }
            delete tc;
            if (tf) { fprintf(tf, "%s\n", summary.c_str()); fflush(tf); fclose(tf); }
            int finalFails = fails;
            dispatch_async(dispatch_get_main_queue(), ^{
                AquariumView* me = weakSelf;
                if (!me) return;
                me->testing = NO;
                me->testStatus.text = [NSString stringWithFormat:
                    @"XONG (%d fail) — gửi tglmt-device-tests.log + aquarium.log", finalFails];
                me->testProgress.progress = 1.0f;
                me->testClose.hidden = NO;
                LogF("[test] wrote %s", [tpath UTF8String]);
            });
        }
    });
}
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property(strong, nonatomic) UIWindow* window;
@end
@implementation AppDelegate
- (BOOL)application:(UIApplication*)app didFinishLaunchingWithOptions:(NSDictionary*)opts {
    (void)app; (void)opts;
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    UIViewController* vc = [[UIViewController alloc] init];
    vc.view = [[AquariumView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    self.window.rootViewController = vc;
    [self.window makeKeyAndVisible];
    [(AquariumView*)vc.view startLoop];
    return YES;
}
@end

int main(int argc, char* argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
