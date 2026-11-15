// gl_context_api.cpp — public C ABI cho tạo/quản lý GL context (versioned).
//
// Xem include/tglmt/gl_context_api.h. Tất cả state OpenGL nằm trong
// tglmt::Context; ở đây chỉ là lớp vỏ C ABI (không exception/STL qua biên).
#include "tglmt/gl_context_api.h"
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/VersionGate.h"
#include <cstring>
#include <new>

using tglmt::Context;
using tglmt::GlProfile;

extern "C" {

TGLMTContext* tglmtCreateContextAttribs(const TGLMTContextAttribs* attribs) {
    TGLMTContextAttribs a;
    if (attribs) a = *attribs;
    else { a.majorVersion = 4; a.minorVersion = 6; a.profile = TGLMT_PROFILE_AUTO;
           a.flags = 0; a.backend = nullptr; }

    const char* backend = (a.backend && a.backend[0]) ? a.backend : "null";
    Context* ctx = new (std::nothrow) Context(backend);
    if (!ctx) return nullptr;

    // Profile AUTO: Core nếu version >= 3.2, Legacy nếu < 3.2.
    int prof = a.profile;
    if (prof == TGLMT_PROFILE_AUTO) {
        prof = (a.majorVersion > 3 || (a.majorVersion == 3 && a.minorVersion >= 2))
                   ? TGLMT_PROFILE_CORE
                   : TGLMT_PROFILE_LEGACY;
    }
    GlProfile p = GlProfile::Legacy;
    switch (prof) {
        case TGLMT_PROFILE_CORE: p = GlProfile::Core; break;
        case TGLMT_PROFILE_COMPATIBILITY: p = GlProfile::Compatibility; break;
        case TGLMT_PROFILE_LEGACY:
        default: p = GlProfile::Legacy; break;
    }
    if (!ctx->SetVersionProfile(a.majorVersion, a.minorVersion, p, a.flags)) {
        delete ctx;
        return nullptr;
    }
    return reinterpret_cast<TGLMTContext*>(ctx);
}

void tglmtDestroyContext(TGLMTContext* c) {
    Context* ctx = reinterpret_cast<Context*>(c);
    if (!ctx) return;
    if (Context::CurrentPtr() == ctx) Context::MakeCurrent(nullptr);
    delete ctx;
}

void tglmtMakeCurrent(TGLMTContext* c) {
    Context::MakeCurrent(reinterpret_cast<Context*>(c));
}

TGLMTContext* tglmtGetCurrentContext(void) {
    return reinterpret_cast<TGLMTContext*>(Context::CurrentPtr());
}

int tglmtGetContextAttribs(TGLMTContext* c, TGLMTContextAttribs* out) {
    Context* ctx = reinterpret_cast<Context*>(c);
    if (!ctx || !out) return 0;
    out->majorVersion = ctx->glMajor;
    out->minorVersion = ctx->glMinor;
    out->profile = (int)ctx->glProfile;
    out->flags = ctx->glContextFlags;
    out->backend = ctx->backendName.c_str();
    return 1;
}

const char* tglmtGetVersionString(void) {
    return reinterpret_cast<const char*>(tglmt::gl::glGetString(0x1F02 /*GL_VERSION*/));
}

void* tglmtGetProcAddress(const char* name) {
    if (!name) return nullptr;
    // Tìm trong bảng export C ABI (sinh ở gl_c_exports.cpp).
    int lo = 0, hi = tglmt::kTglmtProcCount - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int c = std::strcmp(name, tglmt::kTglmtProcTable[mid].name);
        if (c == 0) {
            void* fn = tglmt::kTglmtProcTable[mid].fn;
            // Gate theo (version, profile) của context hiện tại: entry point
            // không tồn tại trong context ⇒ NULL (GLX_ARB_get_proc_address /
            // WGL_ARB_extensions_string: "NULL if not supported").
            Context* ctx = Context::CurrentPtr();
            if (ctx && fn) {
                int gi = tglmt::FindVersionGateIndex(name);
                if (gi >= 0 && !tglmt::VersionGateAllows(gi, ctx->glVersionSlot,
                                                         ctx->glProfile)) {
                    return nullptr;
                }
            }
            return fn;
        }
        if (c < 0) hi = mid - 1;
        else lo = mid + 1;
    }
    return nullptr;
}

}  // extern "C"
