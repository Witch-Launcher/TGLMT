#pragma once
// GL Context — một context / thread (như GLX/WGL context). Giữ ErrorTracker,
// StateTracker, ObjectRegistry, Metal device, và mọi object store (buffer data,
// texture desc, shader source, program linkage, VAO layout, FBO attachments...).
// File này là trái tim TGLMT; mọi gl* đều đi qua Context::Current().
#include "tglmt/gl46_types.h"
#include "tglmt/ErrorTracker.h"
#include "tglmt/ObjectRegistry.h"
#include "tglmt/StateTracker.h"
#include "tglmt/MetalInterface.h"
#include "tglmt/GLSLConverter.h"
#include <array>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>

namespace tglmt {

struct BufferObject {
    GLuint id = 0;
    GLenum target = 0;
    std::vector<uint8_t> data;      // shadow CPU (Null backend + GetBufferSubData)
    GLenum usage = 0x88E4;          // STATIC_DRAW
    GLbitfield storageFlags = 0;
    bool mapped = false;
    size_t mapOffset = 0, mapLength = 0;
    GLbitfield mapAccess = 0;
    std::shared_ptr<metal::IBuffer> gpu; // Apple backend
};

struct VertexAttrib {
    bool enabled = false;
    GLuint buffer = 0;              // buffer của BINDING chứa attrib này
    GLint size = 4;
    GLenum type = 0x1406;           // FLOAT
    GLboolean normalized = 0;
    GLsizei stride = 0;             // stride của BINDING (đồng bộ để draw/getter đọc)
    size_t offset = 0;              // EFFECTIVE = bindings[binding].offset + relativeOffset
    size_t relativeOffset = 0;      // VERTEX_ATTRIB_RELATIVE_OFFSET (Format/con trỏ)
    GLuint binding = 0, divisor = 0;
    bool isInt = false, isLong = false;
};

// Vertex buffer binding point (GL §10.3.1, ARB_vertex_attrib_binding — game 26.x
// dùng Separate path: Format/Binding trước, BindVertexBuffer sau).
struct VertexBindingPoint {
    GLuint buffer = 0;
    GLintptr offset = 0;
    GLsizei stride = 0;
    GLuint divisor = 0;
};

struct VertexArrayObject {
    GLuint id = 0;
    std::array<VertexAttrib, 16> attribs;
    std::array<VertexBindingPoint, 16> bindings;
    GLuint elementBuffer = 0;
};

// Đồng bộ offset effective của 1 attrib sau khi đổi binding/relative/binding-offset.
// Tách đúng spec: glBindVertexBuffer KHÔNG được ghi đè relativeOffset.
inline void VAOSyncAttribOffset(VertexArrayObject& v, GLuint ai) {
    if (ai >= v.attribs.size()) return;
    VertexAttrib& a = v.attribs[ai];
    GLintptr base = (a.binding < v.bindings.size()) ? v.bindings[a.binding].offset : 0;
    if (base < 0) base = 0;
    a.offset = (size_t)base + a.relativeOffset;
}

struct TextureObject {
    GLuint id = 0;
    GLenum target = 0;              // TEXTURE_2D/CUBE/2D_ARRAY/...
    GLsizei levels = 1;
    GLenum internalFormat = 0x8058; // RGBA8
    uint32_t w = 0, h = 0, d = 0;
    std::vector<uint8_t> pixels;    // base level shadow (đủ cho unit test readback)
    // Cubemap (panorama menu): 6 faces shadow riêng. GPU hiện dùng face 0 làm
    // placeholder 2D (cube Metal + sample vec3 là P1, xem limits.md).
    bool isCube = false;
    std::vector<uint8_t> faces[6];
    std::unordered_map<GLenum, GLint> params; // MIN_FILTER/WRAP_S/...
    std::shared_ptr<metal::ITexture> gpu;
};

struct SamplerObject {
    GLuint id = 0;
    std::unordered_map<GLenum, GLint> iparams;
    std::unordered_map<GLenum, GLfloat> fparams;
};
// Kết quả probe PROXY texture (spec §8.1: query WIDTH/HEIGHT phải trả lời,
// quá giới hạn → 0, KHÔNG lỗi). Game probe max size lúc boot.
struct ProxyTex {
    GLsizei w = 0, h = 0;
    GLenum ifmt = 0;
};

struct ShaderObject {
    GLuint id = 0;
    GLenum type = 0;                // VERTEX/FRAGMENT/COMPUTE/...
    std::string source;             // GLSL gốc
    std::string msl;                // MSL đã dịch (GLSL→MSL converter)
    bool compiled = false;
    std::string infoLog;
    GLSLConvertResult conv;         // kết quả converter (varyings/uniforms/layout)
};

struct ProgramObject {
    GLuint id = 0;
    std::vector<GLuint> shaders;
    bool linked = false;
    std::string infoLog;
    std::string vertexMSL, fragmentMSL, computeMSL;
    std::unordered_map<std::string, GLint> uniformLoc;   // name -> loc
    std::unordered_map<GLint, std::vector<uint8_t>> uniforms; // loc -> bytes
    bool separable = false;
    // Transform feedback varyings (glTransformFeedbackVaryings, gọi trước link)
    std::vector<std::string> xfbVaryings;
    GLenum xfbBufferMode = 0x8C8C; // INTERLEAVED_ATTRIBS mặc định
    bool hasTessStages = false;    // true nếu có TCS/TES (cho đường tessellation Metal)
    bool hasGeometryStage = false; // true nếu có GS (Metal không native → emulate, xem gl_tess_xfb)
    // M5b GL-driven render: uniform layout gộp 2 stage (vs trước, fs sau, align 16),
    // sampler (loc → tên), Apple libraries (chỉ khi device thật).
    struct UniformEntry {
        std::string name;
        size_t offset = 0, size = 0;
        GLint loc = -1; // điền khi glGetUniformLocation
        bool isVS = true;
    };
    std::vector<UniformEntry> uniformLayout;
    size_t vsUBSize = 0, fsUBSize = 0;
    std::shared_ptr<metal::ILibrary> appleVS, appleFS;
    std::unordered_map<std::string, GLuint> attribBind; // glBindAttribLocation (trước link)
    std::unordered_map<std::string, GLint> attribLoc;   // location sau link
    // Sampler mapping (fix unit vs MSL slot): tên sampler theo thứ tự khai báo
    // mỗi stage + unit hiện tại (từ glUniform1i, mặc định 0 đúng GL).
    std::vector<std::string> vsSamplers;
    std::vector<std::string> fsSamplers;
    std::unordered_map<std::string, GLuint> samplerUnits;
    // Loại sampler theo tên ('2' 2D, 'C' cube, 'A' array, 'S' shadow, 'B' buffer)
    // để AppleDrawGL bỏ bind khi texture target không khớp (tránh abort Metal).
    std::unordered_map<std::string, char> samplerKind;
    // Uniform blocks (UBO read-only, vanilla 1.17+ / Sodium): tên → index/binding.
    // index là thứ tự khai báo gộp vs+fs (ổn định), binding từ glUniformBlockBinding
    // (mặc định 0). Buffer thật từ Context::uniformBindPoints[binding].
    struct UniformBlock {
        std::string name;
        GLuint index = 0;
        GLuint binding = 0;
        bool isVS = true;
        // Kích thước struct thật (end offset lớn nhất, KHÔNG pad 16 cuối) để
        // phát hiện buffer thiếu (misbound) trước khi bind GPU (A11 fault OOB).
        size_t minSize = 0;
    };
    std::vector<UniformBlock> uniformBlocks;
    // Thứ tự block KHAI BÁO RIÊNG mỗi stage (khớp [[buffer(17+bi)]] mà converter
    // gán trong MSL mỗi stage). AppleDrawGL bind per-stage theo 2 list này;
    // uniformBlocks gộp chỉ còn dùng tra binding point theo tên.
    std::vector<std::string> vsBlocks;
    std::vector<std::string> fsBlocks;
};

struct FramebufferObject {
    GLuint id = 0;
    std::unordered_map<GLenum, GLuint> colorTex; // attachment -> texture id
    GLuint depthTex = 0, stencilTex = 0, depthStencilTex = 0;
    GLsizei w = 0, h = 0;
    std::vector<GLenum> drawBuffers = {0x8CE0}; // GL_COLOR_ATTACHMENT0
};

struct QueryObject { GLuint id = 0; GLenum target = 0; GLuint64 result = 0; bool active = false, ready = true; };
struct SyncObject { GLuint id = 0; bool signaled = false; };
// Buffer range cho glBindBufferBase/Range (UBO index 0x8A11, SSBO 0x90D2).
// Vanilla chunk uploader + Sodium persistent mapping dùng đường này.
struct BufferRange {
    GLuint buffer = 0;
    GLintptr offset = 0;
    GLsizeiptr size = 0; // 0 = toàn bộ buffer
};
// Transform feedback object: Metal không có TF native nên TGLMT emulate bằng
// buffer capture — XFB lưu bind points (buffer effects) + state machine + số
// đỉnh đã capture. Tính varying thật cần M5b thực thi program (ghi rõ ở gl_tess_xfb).
struct XfbObject {
    GLuint id = 0;
    bool active = false, paused = false;
    GLenum primitiveMode = 0x0004; // mode của glBeginTransformFeedback (TRIANGLES)
    std::array<GLuint, 4> buffers = {0, 0, 0, 0}; // TRANSFORM_FEEDBACK_BUFFER bindings
    GLuint capturedCount = 0;      // số đỉnh đã capture (cho glDrawTransformFeedback*)
};

class Context {
public:
    static Context& Current();      // thread_local current
    static void MakeCurrent(Context* ctx);

    ErrorTracker errors;
    StateTracker state;
    ObjectRegistry registry;

    std::shared_ptr<metal::IDevice> device;
    std::string backendName = "null";

    std::unordered_map<GLuint, BufferObject> buffers;
    std::unordered_map<GLuint, VertexArrayObject> vaos;
    std::unordered_map<GLuint, TextureObject> textures;
    std::unordered_map<GLuint, SamplerObject> samplers;
    std::unordered_map<GLuint, ShaderObject> shaders;
    std::unordered_map<GLuint, ProgramObject> programs;
    std::unordered_map<GLuint, FramebufferObject> fbos;
    std::unordered_map<GLuint, QueryObject> queries;
    std::unordered_map<GLuint, SyncObject> syncs;
    std::unordered_map<GLuint, XfbObject> xfbs;
    // UBO/SSBO bind points (glBindBufferBase/Range): binding → buffer range.
    // A11: upload shadow + bind Metal buffer(17+k) mỗi draw (xem AppleDrawGL).
    std::unordered_map<GLuint, BufferRange> uniformBindPoints;
    std::unordered_map<GLuint, BufferRange> storageBindPoints;
    std::map<GLenum, ProxyTex> proxyTex; // target PROXY → spec probe gần nhất

    // clear color/depth/stencil shadow (glClearColor/... + glClear)
    float clearColor[4] = {0, 0, 0, 0};
    double clearDepth = 1.0;
    GLint clearStencil = 0;
    // M5b: draw Apple đầu tiên sau glClear dùng loadActionClear, các draw sau LOAD.
    bool applePendingClear = true; // draw đầu đời cũng clear (target mới, xác định)
    GLbitfield appleClearMask = 0xFFFFFFFFu;
    // Thống kê M5b cho HUD chẩn đoán trên máy (không cần debugger).
    struct AppleStats {
        uint64_t drawsAttempted = 0; // số lần EmitDraw gọi AppleDrawGL
        uint64_t drawsEncoded = 0;   // encode GPU thật thành công
        uint64_t noProgram = 0;      // thiếu program/link/libs
        uint64_t noTarget = 0;       // thiếu default target / wrap fail
        uint64_t noPipeline = 0;     // pipeline nil (format attrib lạ...)
        uint64_t miscFail = 0;       // còn lại (VAO, buffer, commit...)
        uint64_t glClears = 0;       // số lần glClear (đối chiếu pendingClear)
        uint64_t blits = 0;          // số lần glBlitFramebuffer (composite cuối?)
        uint64_t copyTex = 0;        // số lần glCopyTexSubImage2D (post chain?)
        uint64_t rangeWarn = 0;      // draw đọc đỉnh/index vượt buffer (TBDR fault?)
        uint64_t hazardWarn = 0;     // draw vừa render vừa sample cùng texture
        uint64_t mipBase = 0;        // sampler 1-level + minfilter mipmap → base (fix A11)
        uint64_t uboSmall = 0;       // UBO buffer thiếu so với struct → zero fallback (chống fault)
        std::map<GLuint, uint64_t> progEncoded; // program id -> số draw đã encode
    };
    AppleStats appleStats;

    // debug callback (glDebugMessageCallback)
    TGLMTDebugProc debugCb = nullptr;
    const void* debugUser = nullptr;

    void LogDebug(GLenum src, GLenum type, GLuint id, GLenum sev, const std::string& msg);

    Context(const std::string& backend = "null");
    ~Context() = default;
private:
    static thread_local Context* tCurrent_;
    static Context* sFallback_;
    static std::mutex sMu_;
};

} // namespace tglmt
