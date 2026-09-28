#pragma once
// GLFWShim — cầu nối tối thiểu để gắn TGLMT vào launcher Minecraft Java (iOS).
//
// Vanilla (LWJGL/GLFW/Blaze3D) cần:
//   CreateWindow + MakeContextCurrent + SwapBuffers + SwapInterval + GetProcAddress.
// Phase 1 (vanilla boot, A11): ĐƠN context + serialize đa luồng bằng mutex toàn cục.
//   - Main thread (render) + chunk uploader thread DÙNG CHUNG 1 Context, gọi xen kẽ
//     qua TGLMT_MakeCurrent (thread_local) + khóa ngoài của launcher.
//   - KHÔNG concurrent draws (đúng GL: 1 context / 1 thread tại 1 thời điểm).
//   - Share-lists thật (song song chunk upload + render) là P1 (Sodium persistent-map).
//
// Cách dùng launcher:
//   TGLMT_Window* w = TGLMT_CreateWindow(960, 600, "apple", true /*depth*/);
//   TGLMT_MakeCurrent(w); ... gl::... ; TGLMT_SwapBuffers(w, metalLayer);
//   void* p = TGLMT_GetProcAddress("glDrawElements"); // cho LWJGL dlsym
#include <cstdint>

namespace tglmt {

struct TGLMT_Window {
    uint32_t w = 0, h = 0;
    int swapInterval = 1;
    void* userPtr = nullptr;
};

class GLFWShim {
public:
    static GLFWShim& Instance();
    // backend: "apple" (GPU A11+) hoặc "null" (CI). depth=true cho 3D.
    TGLMT_Window* CreateWindow(uint32_t w, uint32_t h, const char* backend = "apple",
                               bool depth = true);
    void DestroyWindow(TGLMT_Window* w);
    // MakeCurrent window trên thread gọi (thread_local Context). Trả false khi w null/chưa init.
    bool MakeCurrent(TGLMT_Window* w);
    static void ClearCurrent();
    // SwapBuffers: present default target lên CAMetalLayer* (iOS) hoặc headless khi null.
    // Trả false khi present lỗi (format lệch/size 0). SwapInterval 0 = không vsync (benchmark).
    bool SwapBuffers(TGLMT_Window* w, void* metalLayer);
    void SwapInterval(int interval);
    // Cho LWJGL: tra địa chỉ hàm gl* theo tên ("glDrawElements", "glGenTextures", ...).
    // Trả nullptr khi tên lạ. Bảng 698 core + 3 bonus, sinh từ gl.xml (không đoán).
    void* GetProcAddress(const char* name);
    bool HasRealGPU() const;
    uint64_t DrawsEncoded() const;
    const char* BackendName() const;
    // Resize target render (xoay màn hình): dựng lại target ở BeginFrame kế tiếp.
    void Resize(TGLMT_Window* w, uint32_t width, uint32_t height);
};

// C API cho JNI/ObjC launcher (tránh name-mangling).
extern "C" {
TGLMT_Window* TGLMT_CreateWindow(uint32_t w, uint32_t h, const char* backend, int depth);
void TGLMT_DestroyWindow(TGLMT_Window* w);
int TGLMT_MakeCurrent(TGLMT_Window* w);
void TGLMT_ClearCurrent(void);
int TGLMT_SwapBuffers(TGLMT_Window* w, void* metalLayer);
void TGLMT_SwapInterval(int interval);
void* TGLMT_GetProcAddress(const char* name);
int TGLMT_HasRealGPU(void);
void TGLMT_ResizeWindow(TGLMT_Window* w, uint32_t width, uint32_t height);
}

} // namespace tglmt
