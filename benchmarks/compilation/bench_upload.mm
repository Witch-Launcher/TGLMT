// benchmarks/compilation/bench_upload.cpp — Đo chi phí upload texture theo
// đúng đường đi của iOS/A11.
//
// VÌ SAO CẦN FILE RIÊNG
// ---------------------
// Backend Metal của TGLMT dùng:
//   • iOS  → MTLStorageModeShared  ⇒ replaceRegion trực tiếp (zero-copy)
//   • macOS→ MTLStorageModeManaged ⇒ staging buffer + blit + command buffer
// Nên máy dev (macOS) đo ĐƯỜNG KHÁC với thiết bị đích (iOS). Benchmark này đo
// trực tiếp cả hai đường bằng API Metal thô, để con số áp dụng cho A11 có thể
// đo được trên bất kỳ máy nào (kể cả CI không có GPU Apple).
//
// Đo: 1× replaceRegion (shared), 2× replaceRegion (shared — trạng thái cũ),
// staging+blit (managed), và newTexture.
#include <Metal/Metal.h>
#include <Foundation/Foundation.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

using Clock = std::chrono::steady_clock;

static double medianOf(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}
static double p95Of(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    size_t i = (size_t)(v.size() * 0.95);
    if (i >= v.size()) i = v.size() - 1;
    return v[i];
}

template <typename F>
static void measure(const char* name, int iters, std::vector<double>& out, F&& fn) {
    for (int i = 0; i < 100; ++i) fn();
    out.clear();
    for (int i = 0; i < iters; ++i) {
        auto a = Clock::now();
        fn();
        auto b = Clock::now();
        out.push_back(std::chrono::duration<double, std::micro>(b - a).count());
    }
    printf("  \"%s\": {\"median_us\": %.4f, \"p95_us\": %.4f, \"n\": %d},\n", name,
           medianOf(out), p95Of(out), iters);
}

int main(int argc, char** argv) {
    int iters = 300;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--iters") && i + 1 < argc) iters = atoi(argv[++i]);
    }
    @autoreleasepool {
        id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
        if (!dev) {
            fprintf(stderr, "[bench] không có Metal device\n");
            return 1;
        }
        const uint32_t W = 64, H = 64;
        std::vector<uint8_t> px(W * H * 4, 0x7f);

        MTLTextureDescriptor* sd =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                width:W
                                                               height:H
                                                             mipmapped:NO];
        sd.storageMode = MTLStorageModeShared; // ← đường iOS/A11
        id<MTLTexture> sharedTex = [dev newTextureWithDescriptor:sd];

        MTLTextureDescriptor* md =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                width:W
                                                               height:H
                                                             mipmapped:NO];
        md.storageMode = MTLStorageModeManaged; // ← đường macOS
        id<MTLTexture> managedTex = [dev newTextureWithDescriptor:md];

        id<MTLCommandQueue> queue = [dev newCommandQueue];
        std::vector<double> v;

        printf("{\n");
        printf("  \"benchmark\": \"upload\",\n");
        printf("  \"device\": \"%s\",\n", [[dev name] UTF8String]);
        printf("  \"texture\": \"%ux%u RGBA8 (16 KiB)\",\n", W, H);
        printf("  \"metrics\": {\n");

        measure("shared_replaceRegion_x1_ios_path", iters, v, [&] {
            [sharedTex replaceRegion:MTLRegionMake2D(0, 0, W, H) mipmapLevel:0
                          withBytes:px.data() bytesPerRow:W * 4];
        });
        measure("shared_replaceRegion_x2_old_buggy_path", iters, v, [&] {
            [sharedTex replaceRegion:MTLRegionMake2D(0, 0, W, H) mipmapLevel:0
                          withBytes:px.data() bytesPerRow:W * 4];
            [sharedTex replaceRegion:MTLRegionMake2D(0, 0, W, H) mipmapLevel:0
                          withBytes:px.data() bytesPerRow:W * 4];
        });
        measure("managed_staging_plus_blit_macos_path", iters, v, [&] {
            id<MTLBuffer> stage = [dev newBufferWithBytes:px.data() length:px.size()
                                                    options:MTLResourceStorageModeShared];
            id<MTLCommandBuffer> cb = [queue commandBuffer];
            id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
            [blit copyFromBuffer:stage sourceOffset:0 sourceBytesPerRow:W * 4
                   sourceBytesPerImage:W * 4 * H sourceSize:MTLSizeMake(W, H, 1)
                   toTexture:managedTex destinationSlice:0 destinationLevel:0
                  destinationOrigin:MTLOriginMake(0, 0, 0)];
            [blit endEncoding];
            [cb commit];
        });
        measure("newTexture_64x64", iters, v, [&] {
            MTLTextureDescriptor* d =
                [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                    width:W height:H
                                                                 mipmapped:NO];
            id<MTLTexture> t = [dev newTextureWithDescriptor:d];
            (void)t;
        });
        measure("newBufferWithBytes_16KB", iters, v, [&] {
            id<MTLBuffer> b = [dev newBufferWithBytes:px.data() length:px.size()
                                               options:MTLResourceStorageModeShared];
            (void)b;
        });

        printf("    \"_note\": \"shared_* là đường iOS/A11; managed_* là đường macOS\"\n");
        printf("  }\n");
        printf("}\n");
    }
    return 0;
}
