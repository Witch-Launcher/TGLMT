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
#include <atomic>
#include <chrono>

namespace tglmt {

struct BufferObject {
    GLuint id = 0;
    GLenum target = 0;
    // Shadow CPU. Apple backend: chỉ giữ khi device Null / BufferData chưa upload
    // / buffer dùng làm PBO — mọi thứ khác đọc thẳng GPU memory chung (Shared).
    // Null backend: shadow LÀ tài nguyên duy nhất (giữ nguyên như cũ).
    std::vector<uint8_t> data;
    size_t size = 0;                // kích thước logích — đúng cả khi data rỗng
    GLenum usage = 0x88E4;          // STATIC_DRAW
    GLbitfield storageFlags = 0;
    bool mapped = false;
    size_t mapOffset = 0, mapLength = 0;
    GLbitfield mapAccess = 0;
    // Số lần nội dung THẬT SỰ đổi (gl*BufferData/Storage hoặc flush staging).
    // Id gán lúc tạo là duy nhất trong context (Context::bufGenNext) nên id buffer
    // tái dùng không bao giờ trùng gen → cache glTexBuffer không bao giờ stale.
    uint64_t contentGen = 0;
    std::shared_ptr<metal::IBuffer> gpu; // Apple backend
    // Pool buffer phụ cho in-flight write: khi GPU còn đang đọc `gpu` (đã commit,
    // chưa execute) mà app ghi lại buffer này, ta xoay sang 1 slot pool khác thay
    // vì newBuffer mỗi lần (Minecraft tái dùng 1 vertex buffer/VertexFormat cho
    // MỌI batch trong frame → không pool thì alloc + full-copy mỗi draw).
    // Slot đã dùng ở "đợi" nào được đánh dấu; slot an toàn khi GPU đã execute
    // xong tới đợi đó (xem Context::cbCommitted/cbCompleted).
    struct GpuSlot {
        std::shared_ptr<metal::IBuffer> buf;
        uint64_t usedGen = 0;
    };
    std::vector<GpuSlot> pool;
    size_t poolNext = 0;
    uint64_t lastReadGen = 0; // giữ cho log/diag
};

// ---- Truy xuất buffer (P1: bỏ nhân bản CPU) ----
// Mọi gl* đọc buffer PHẢI qua 2 helper này. Đọc `.data` trực tiếp sẽ vỡ khi
// shadow rỗng (Apple backend đã bỏ bản sao CPU vì GPU buffer là Shared memory).
inline size_t BufferLen(const BufferObject& b) {
    size_t n = b.size;
    if (b.data.size() > n) n = b.data.size();     // PBO pack có thể grow data
    if (!n && b.gpu) n = b.gpu->length();
    return n;
}
inline const uint8_t* BufferBytes(const BufferObject& b) {
    if (!b.data.empty()) return b.data.data();           // shadow còn → nguồn chuẩn
    if (b.gpu && b.gpu->contents()) return (const uint8_t*)b.gpu->contents();
    return nullptr;
}

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
    // Mip levels >0 (vanilla 26.x alloc + upload MỌI level qua TexImage/SubImage
    // per-level: GlDevice.createTexture + MipmapGenerator). GPU hiện giữ base
    // level duy nhất (non-mipmapped) nên các level này chỉ sống trong shadow để
    // GetTexImage(level>0)/CPU fallback đúng + sẵn sàng cho mipmapped sau này.
    struct MipLevel {
        GLsizei w = 0, h = 0;
        std::vector<uint8_t> pixels; // tight RGBA8 4B (w*h*4), rỗng = chưa upload
    };
    std::map<GLint, MipLevel> mipData; // level (>=1) -> shadow
    // Cubemap (panorama menu): 6 faces shadow riêng. GPU hiện dùng face 0 làm
    // placeholder 2D (cube Metal + sample vec3 là P1, xem limits.md).
    bool isCube = false;
    std::vector<uint8_t> faces[6];
    // Diag: TỪNG LÀ color attachment của FBO (GuiItemAtlas bake etc) — nội dung
    // đến từ render, CPU shadow luôn rỗng → probe iconatlas# đọc GPU trực tiếp.
    bool wasRT = false;
    std::unordered_map<GLenum, GLint> params; // MIN_FILTER/WRAP_S/...
    // EXT_texture_swizzle: GL_TEXTURE_SWIZZLE_RGBA decode tại glTexParm.
    // Code = MTLTextureSwizzle (0=Zero,1=One,2=R,3=G,4=B,5=A); mặc định
    // identity {R,G,B,A}. Bind lúc draw → ITexture::swizzledView (gl_apple_draw).
    uint8_t swz[4] = {2, 3, 4, 5};
    std::shared_ptr<metal::ITexture> gpu;
    // Như BufferObject::lastReadGen — encoder đã commit nhưng GPU chưa execute
    // (lastReadGen == Context::waitGen) → ghi replaceRegion sẽ đổi nội dung
    // mà draw cũ đang sample (in-flight write).
    uint64_t lastReadGen = 0;
    // glTexBuffer: cache (buffer, contentGen) đã upload. Trước đây MC gọi
    // glTexBuffer MỖI FRAME → alloc tmp 2.8MB + newTextureWithBytes lại mỗi frame
    // (~2–6ms + churn RAM). Bỏ qua khi buffer + ifmt + contentGen không đổi.
    GLuint buftexBuf = 0;
    uint64_t buftexGen = 0;
    // glInvalidateFramebuffer (spec §18.4): nội dung attachment trở nên KHÔNG XÁC
    // ĐỊNH. TBDR (A11) chỉ được trả DONT_CARE khi load ở pass kế tiếp — đó là
    // thắng lợi băng thông thật, không phải no-op. Cờ được TIÊU THỤ khi encoder
    // dùng attachment này được tạo (nội dung mới do chính pass đó sinh ra).
    bool contentUndefined = false;
};

// P1: pixel có đọc lại được từ GPU (buffer Shared) không? Nếu KHÔNG → shadow CPU
// LÀ nguồn duy nhất và phải giữ. Nếu CÓ → shadow là bản sao vô ích (nguyên nhân
// jetsam 2043MB) nên TGLMT bỏ và đọc GPU theo yêu cầu (Context::ReadTextureGPU).
// Điều kiện phải khớp với danh sách refresh của GetTexSubImpl.
inline bool TexReadableFromGPU(const TextureObject& t) {
    return t.internalFormat == 0x8058 || t.internalFormat == 0x8C43 ||
           t.internalFormat == 0x1908 || t.internalFormat == 0;
}

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
    // P6f compute: library + pipeline + localSize (từ layout(local_size) converter).
    std::shared_ptr<metal::ILibrary> appleCS;
    std::shared_ptr<metal::IComputePipeline> appleCSPipeline;
    int computeLocalSize[3] = {1, 1, 1};
    // Map block → MSL buffer slot (bindingPoint → slot) cho compute dispatch bind.
    std::vector<GLSLConvertResult::BlockSlot> computeBlockSlots;
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
    // Tag program entity_shadow (Fix #6): DT+Fog+Proj, 1 fs 2D Sampler0, không
    // vs sampler. AppleDrawGL dùng để bắt draw sample sai texture (quads bóng
    // thấy texture gà) + dump texring tại chỗ.
    bool shadowLike = false;
    // Uniform blocks (UBO read-only, vanilla 1.17+ / Sodium): tên → index/binding.
    // index là thứ tự khai báo gộp vs+fs (ổn định), binding từ glUniformBlockBinding
    // (mặc định 0). Buffer thật từ Context::uniformBindPoints[binding].
    struct UniformBlock {
        std::string name;
        GLuint index = 0;
        GLuint binding = 0;
        int layoutBinding = -1; // layout(binding=N) trong GLSL (-1 = không có)
        bool isVS = true;
        // minSize = kích thước std140 làm tròn 16 == GL UNIFORM_BLOCK_DATA_SIZE.
        size_t minSize = 0;
        // trueSize = end offset lớn nhất (KHÔNG pad cuối) — ngưỡng "buffer thiếu".
        // App bind đúng struct thật (140/56/40/12B) vẫn phải ĐƯỢC UPLOAD với pad 0
        // phía sau; dùng minSize làm ngưỡng sẽ zero-fallback oan → ma trận = 0 →
        // geometry suy biến (ô atlas trống / model vô hình).
        size_t trueSize = 0;
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
    // Mip level của từng attachment (vanilla 26.x bake animation per-mip qua
    // GlTextureView(texture, baseMipLevel, 1) + bindFrameBufferTextures(..., level)).
    // Bỏ qua level (luôn 0) làm mọi mip bake alias level 0 → smear atlas.
    std::unordered_map<GLenum, GLint> colorLevel; // attachment -> mip level (mặc định 0)
    GLuint depthTex = 0, stencilTex = 0, depthStencilTex = 0;
    GLint depthLevel = 0, stencilLevel = 0, depthStencilLevel = 0;
    GLsizei w = 0, h = 0;
    std::vector<GLenum> drawBuffers = {0x8CE0}; // GL_COLOR_ATTACHMENT0
};

struct QueryObject {
    GLuint id = 0;
    GLenum target = 0;
    GLuint64 result = 0;
    bool active = false, ready = true;
    // ARB_pipeline_statistics_query: counter software là tích luỹ từ đầu context,
    // glBeginQuery snapshot lại, glEndQuery tính delta (đúng spec §18.4.2 —
    // kết quả là số lần gọi GIỮA glBegin/glEnd, không phải tổng cả session).
    GLuint64 pipeSnapshot = 0;
    // Occlusion ANY_SAMPLES: byte offset trong occlBuf (UINT64_MAX = không phải
    // occlusion). Sau glEndQuery, GPU ghi 1 vào occlBuf[slot] nếu có fragment
    // pass depth-test; đọc sau CommitAndWait.
    uint64_t occlSlot = UINT64_MAX;
};
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
    // Id gen duy nhất cho BufferObject::contentGen (xem glTexBuffer cache).
    uint64_t bufGenNext = 1;
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
    // FBO đang bound khi glClear gọi (0xFFFFFFFF = wildcard do Renderer tự set
    // cho target mới). So với draw tiêu thụ để lộ clear áp sai target (clrmiss#).
    uint32_t appleClearFBO = 0xFFFFFFFFu;
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
        uint64_t mipLevelSkipped = 0; // draw/clear vào mip level>0 của texture 1-level (animate bake) → bỏ qua an toàn
        uint64_t mipStaged = 0;      // TexSubImage level>0 đã lưu shadow (GPU base-level không sync)
        uint64_t uboSmall = 0;       // UBO buffer thiếu so với struct → zero fallback (chống fault)
        uint64_t uboPad = 0;         // UBO thiếu đúng pad std140 (≤16B) → upload thật + pad 0
        // In-flight write: GPU còn đang đọc shared memory này khi app ghi lại →
        // phải xoay sang buffer pool slot khác (thay vì alloc MTLBuffer mới).
        uint64_t bufRotated = 0;     // số lần xoay pool slot
        uint64_t bufCopyBytes = 0;   // tổng byte memcpy khi xoay (gap + range)
        uint64_t texRotated = 0;     // số lần texture phải chờ/giữ
        uint64_t framesThrottled = 0;// số lần buộc chờ vì vượt ngân sách frame
        uint64_t cbCommittedNow = 0, cbCompletedNow = 0; // mirror counter để log
        uint64_t presentPasses = 0;  // số lần present bằng 1 render pass (flip)
        uint64_t clearRegionFails = 0; // fillRegionColor fail (KHÔNG được xoá cả atlas)
        // Chống in-flight write: ghi shadow→GPU vào buffer/texture mà encoder đã
        // commit nhưng GPU chưa execute (commitAndWait chưa chạy).
        uint64_t bufOrphan = 0;      // flush phải orphan (tạo MTLBuffer mới) thay vì ghi tại chỗ
        uint64_t bufWaitStall = 0;   // flush buffer quá lớn → commitAndWait ( stall, hiếm )
        uint64_t uboFromMap = 0;     // UBO đọc từ gpu->contents() vì buffer đang mapped
        uint64_t texWasRTFlush = 0;  // flush staging trên texture wasRT (nội dung chỉ trên GPU)
        uint64_t texWaitStall = 0;   // flush texture in-flight → commitAndWait trước khi ghi
        // IR lowering stats (chứng minh 1 GL → 0 Metal khi không đổi):
        uint64_t encodersCreated = 0; // số MTLRenderCommandEncoder đã tạo (muốn << draws)
        uint64_t encoderReused = 0;   // số draw tái dùng encoder đang mở (batching)
        uint64_t pipelineReused = 0;  // số draw giữ nguyên pipeline (skip setPipeline)
        uint64_t stateSkipped = 0;    // số set* đã bỏ qua nhờ dirty-check (viewport/cull/...)
        uint64_t uniformReused = 0;   // số draw tái dùng uniform buffer (uniforms không đổi)
        uint64_t depthReused = 0;     // số draw tái dùng depth state (skip makeDepthStencil)
        uint64_t pipelineLookups = 0; // số lần gọi bridge makeCustomPipeline (muốn giảm)
        uint64_t pipelineLookupSkipped = 0; // số draw bỏ cả bridge lookup nhờ IR key cache
        uint64_t bufferCoalesced = 0; // số glBufferSubData gộp (defer, chưa tính flush)
        uint64_t bufferFlushes = 0;   // số lần flush buffer staging lên GPU
        uint64_t texCoalesced = 0;    // số glTexSubImage gộp vào staging
        uint64_t texFlushes = 0;      // số lần flush texture staging (replaceRegion)
        // Deferred full (Phase 1-3): bằng chứng gom lệnh giảm Metal calls.
        uint64_t traceSkipped = 0;    // số draw bỏ trace-encoder thừa (Apple path)
        uint64_t diagSkipped = 0;     // số diagnostic readback/log đã bỏ (release)
        uint64_t ringAllocs = 0;      // số temp upload từ ring (0 MTLBuffer alloc)
        uint64_t ringWraps = 0;       // số lần ring xoay frame (triple-buffer)
        uint64_t tempAllocs = 0;      // số TempUpload rơi về newBuffer (ring đầy/Null)
        uint64_t flushAvoided = 0;    // số lần stage KHÔNG flush encoder nhờ conditional-flush
        uint64_t hazardSplits = 0;    // số lần split pass do feedback hazard (TBDR đúng)
        uint64_t multidrawBatched = 0;// số sub-draws trong MultiDraw* đã batch
        uint64_t psoPrewarmed = 0;    // số pipeline đã prewarm trước frame đầu
        std::map<GLuint, uint64_t> progEncoded; // program id -> số draw đã encode
        // P0 memory accounting (điều tra jetsam): tách shadow CPU (TGLMT tự giữ,
        // dùng được trên cả Null backend) khỏi tài nguyên GPU thật (Shared → cũng
        // là RAM nhưng do Metal quản lý). Shadow = ứng cử viên bị cắt ở P1.
        uint64_t memTexBase = 0;      // sum TextureObject::pixels
        uint64_t memTexMip = 0;       // sum mipData[*].pixels (level >0)
        uint64_t memTexFace = 0;      // sum faces[*] (cubemap)
        uint64_t memBufShadow = 0;    // sum BufferObject::data
        uint64_t memGpuTex = 0;       // sum GPU texture bytes
        uint64_t memGpuBuf = 0;       // sum BufferObject::gpu (bản chính)
        uint64_t memGpuBufPool = 0;   // sum BufferObject::pool slots (xoay in-flight)
        uint64_t memGpuRing = 0;      // sum Context::ringBuf (temp upload 3x4MB)
        uint64_t nTextures = 0, nBuffers = 0, nBufferStaged = 0;
        // Phân chia thời gian CPU theo pha (nanoseconds, tích luỹ) — log build=
        // tách ra drawMs/clrMs/endMs/stallMs để biết frame ~50ms đi đâu khi
        // thr=0 (GPU không nghẽn) mà FPS vẫn thấp. Không cần profiler trên máy.
        uint64_t nsDraw = 0;         // trong AppleDrawGL (encode 1 draw)
        uint64_t nsClear = 0;        // trong glClear
        uint64_t nsEnd = 0;          // trong Renderer::EndFrame (flush + present)
        uint64_t nsStall = 0;        // trong Context::CommitAndWait (chờ GPU xong)
        uint64_t nPool = 0;          // số slot pool đang có buffer (mem frame=)
        uint64_t maxBuf = 0;         // buffer lớn nhất (bytes) — ai chiếm buf=420MB
        // glBlitFramebuffer = hotspot #1 (gltop#1: 6.5–26ms mỗi frame, 1 call/frame).
        // 5 counters này nói rõ nó rơi vào nhánh nào để biết đường fix.
        uint64_t blitIdentical = 0;  // rect src == dst (không scale, không offset) → đủ điều kiện GPU
        uint64_t blitScaled = 0;     // rect có scale
        uint64_t blitLinear = 0;     // filter != NEAREST
        uint64_t blitFast = 0;       // đi được GPU blit (không readback)
        uint64_t blitFastFail = 0;   // thử GPU blit nhưng bridge từ chối (định dạng/...)
        uint64_t blitCpu = 0;        // rơi CPU fallback: readback + vòng pixel/frame
        // p7: glClear dồn clear-ONLY pass vào chung 1 command buffer.
        // clears = số glClear đi qua makeClearEncoder; clearCBs = số CB đã commit.
        // clears/clearCBs = tỉ lệ dồn (muốn ~1 CB cho nhiều clear liên tiếp).
        uint64_t clears = 0;
        uint64_t clearCBs = 0;
    };
    AppleStats appleStats;

    // ARB_pipeline_statistics_query — software counter (spec §18.4.2).
    // Metal KHÔNG có hardware counter cho số lần gọi shader; TGLMT đếm chính xác
    // những gì tính được từ state draw (vertex/primitive/VS-invocation = count×inst
    // và số primitive theo mode). Fragment-shader-invocation KHÔNG tính được →
    // extension không được quảng cáo (xem gl_error_string_debug.cpp) cho tới khi
    // đường counter GPU (P5) chạy thật. Đếm luôn, query bất cứ lúc nào cũng trả
    // đúng phần tính được.
    struct PipeStats {
        uint64_t vertsSubmitted = 0;   // GL_VERTICES_SUBMITTED_ARB
        uint64_t primsSubmitted = 0;   // GL_PRIMITIVES_SUBMITTED_ARB
        uint64_t vsInvocations = 0;    // GL_VERTEX_SHADER_INVOCATIONS_ARB
        uint64_t gsInvocations = 0;    // GL_GEOMETRY_SHADER_INVOCATIONS
        uint64_t gsPrimsWritten = 0;   // GL_GEOMETRY_SHADER_PRIMITIVES_WRITTEN_ARB
        uint64_t fsInvocations = 0;    // GL_FRAGMENT_SHADER_INVOCATIONS_ARB (0: không đo được)
        uint64_t csInvocations = 0;    // GL_COMPUTE_SHADER_INVOCATIONS
    } pipeStats;

    // Occlusion ANY_SAMPLES (EXT_occlusion_query_boolean): buffer visibility
    // do device sở hữu (IDevice::visibilityBuffer), đính trên render pass
    // descriptor. Context chỉ track query active + slot offset.
    static constexpr uint64_t kOcclNumSlots = 256;
    GLuint occlActiveQ = 0;                     // id query occlusion đang active (0 = none)
    uint64_t occlSlot = 0;                      // byte offset của slot active
    bool occlPendingRead = false;               // đã glEndQuery, chờ đọc kết quả

    // RAII đo thời gian CPU của một pha: khai báo biến ở đầu hàm, huỷ tự cộng
    // thời gian vào AppleStats::*field. Không nhánh return nào bị sót.
    struct PhaseTimer {
        AppleStats* st;
        uint64_t AppleStats::*field;
        std::chrono::steady_clock::time_point t0;
        PhaseTimer(AppleStats& s, uint64_t AppleStats::*f)
            : st(&s), field(f), t0(std::chrono::steady_clock::now()) {}
        ~PhaseTimer() {
            auto d = std::chrono::steady_clock::now() - t0;
            st->*field +=
                (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
        }
    };

    // IR lowering — deferred materialization (1 OpenGL → 0 Metal nếu chưa cần):
    // pendingEncoder giữ MTLRenderCommandEncoder mở xuyên suốt các draw liên tiếp
    // cùng target; chỉ flush (endEncoding+commitNoWait) khi target đổi / readback /
    // blit / present / frame kết thúc. pending* là shadow của Metal state đã encode,
    // dùng để dirty-check: chỉ encode thứ thực sự đổi.
    std::shared_ptr<metal::IRenderEncoder> pendingEncoder;
    std::shared_ptr<metal::IRenderTarget> pendingTarget;
    std::shared_ptr<metal::IRenderPipeline> pendingPipeline;
    bool pendingHasDepth = false;
    GLuint pendingDrawFBO = 0xFFFFFFFFu; // FBO id của pass đang mở (0=default)
    GLuint pendingColorTex = 0xFFFFFFFFu; // color texture id khi FBO!=0
    bool pendingViewportValid = false;
    metal::Viewport pendingViewport{0,0,0,0,0,1};
    bool pendingCullValid = false;
    bool pendingCullEnabled = false;
    uint32_t pendingCullMode = 0, pendingFrontFace = 0;
    // GL_DEPTH_CLAMP (ARB_depth_clamp) → MTLDepthClipModeClamp/Clip.
    bool pendingDepthClampValid = false;
    bool pendingDepthClamp = false;
    bool pendingBlendValid = false;
    float pendingBlend[4] = {0,0,0,0};
    bool pendingDepthValid = false;
    uint32_t pendingDepthFunc = 0;
    bool pendingDepthMask = true;
    std::shared_ptr<metal::IDepthStencilState> pendingDepthState;
    bool pendingFillValid = false;
    bool pendingFillLines = false;
    // glPolygonOffset / glPolygonOffsetClamp (ARB_polygon_offset_clamp) → Metal
    // setDepthBias:slopeScale:clamp:. Đúng spec: bias = units × r (r = minimum
    // resolvable difference), slopeScale = factor, clamp = clamp (0 = không clamp,
    // theo convention của Vulkan/Metal).
    bool pendingPolyOffValid = false;
    float pendingPolyBias = 0.0f, pendingPolySlope = 0.0f, pendingPolyClamp = 0.0f;
    bool pendingScissorValid = false;
    bool pendingScissorEnabled = false;
    metal::ScissorRect pendingScissor{0,0,0,0};
    // Uniform staging cache: program → bytes lần cuối đã upload (tránh newBuffer mỗi draw
    // khi uniforms không đổi). Chỉ dùng cho vs/fsUB 16-slot; UBO per-draw vẫn upload
    // vì buffer source có thể đổi qua glBufferSubData.
    struct UniformCache {
        std::vector<uint8_t> vsBytes, fsBytes;
        std::shared_ptr<metal::IBuffer> vsBuf, fsBuf;
        size_t vsOff = 0, fsOff = 0;
        GLuint prog = 0;
        bool valid = false;
    };
    UniformCache uniformCache;
    // IR pipeline-key cache (Deferred State Translation đầy đủ):
    // key = mọi input bridge dùng để build PSO (libs+fmt+attribs+stride+depth+blend).
    // Nếu key giống pending → tái dùng pendingPipeline, BỎ CẢ bridge lookup
    // (trước đây mỗi draw vẫn build string + mutex dù đã cache PSO).
    struct PipelineKey {
        const void* vsLib = nullptr;
        const void* fsLib = nullptr;
        metal::PixelFormat fmt = metal::PixelFormat::Invalid;
        uint32_t stride = 0;
        bool depth = false;
        bool blend = false;
        metal::AttachmentBlend blend0;
        uint32_t colorWriteMask = 0xF;
        // Mảng cố định (tối đa 16 attrib theo GL): tránh vector + cấp phát lại
        // mỗi draw khi so sánh pipeline key.
        uint32_t nAttribs = 0;
        metal::CustomAttrib attribs[16] = {};
        bool operator==(const PipelineKey& o) const {
            if (vsLib != o.vsLib || fsLib != o.fsLib || fmt != o.fmt ||
                stride != o.stride || depth != o.depth || blend != o.blend ||
                colorWriteMask != o.colorWriteMask)
                return false;
            if (blend) {
                if (blend0.enabled != o.blend0.enabled || blend0.srcRGB != o.blend0.srcRGB ||
                    blend0.dstRGB != o.blend0.dstRGB || blend0.srcAlpha != o.blend0.srcAlpha ||
                    blend0.dstAlpha != o.blend0.dstAlpha || blend0.rgbOp != o.blend0.rgbOp ||
                    blend0.alphaOp != o.blend0.alphaOp)
                    return false;
            }
            if (nAttribs != o.nAttribs) return false;
            for (uint32_t i = 0; i < nAttribs; ++i) {
                const auto& a = attribs[i];
                const auto& b = o.attribs[i];
                if (a.loc != b.loc || a.size != b.size || a.type != b.type ||
                    a.normalized != b.normalized || a.offset != b.offset ||
                    a.bufferIndex != b.bufferIndex || a.stride != b.stride ||
                    a.divisor != b.divisor)
                    return false;
            }
            return true;
        }
    };
    PipelineKey pendingPipeKey;
    bool pendingPipeValid = false;
    // PSO đã resolve từ key (sống qua flush encoder — PSO tái dùng cho pass sau).
    // Khác pendingPipeline (state đã bind trong encoder đang mở, reset khi flush).
    std::shared_ptr<metal::IRenderPipeline> cachedPipe;
    // ---- Resource staging (upload coalescing): buffer + texture updates defer ----
    // Buffer: glBufferSubData* chỉ memcpy vào shadow + ghi range vào staging;
    // GPU copy + didModifyRange dồn đến FlushBufferStaging() (trước draw dùng buffer,
    // ReadPixels/Blit/finish). Các range kề/chồng được merge → 1 memcpy+didModify.
    // bytes != rỗng: payload đi theo range (dùng khi shadow CPU đã bỏ — P1). Flush
    // ghi payload đè lên GPU buffer, phần ngoài range lấy từ GPU/shadow cũ.
    struct BufRange { size_t off = 0, len = 0; std::vector<uint8_t> bytes; };
    std::unordered_map<GLuint, std::vector<BufRange>> pendingBufRanges;
    // stage + merge (upload coalescing). src != nullptr → carry payload (shadow rỗng).
    void StageBufferRange(GLuint buf, size_t off, size_t len, const uint8_t* src = nullptr);
    void FlushBufferStaging(GLuint buf); // flush 1 buffer (merge ranges)
    void FlushAllBufferStaging();        // flush tất cả (finish/readback)
    struct BufferView { const uint8_t* p = nullptr; size_t size = 0; };
    // Đồng bộ + trả nội dung buffer hiện hành (xả staging trước nếu shadow rỗng).
    BufferView SyncBuffer(GLuint buf);
    // PBO pack: trả shadow CPU sẵn sàng ghi [0, end). Shadow có thể đã bị bỏ
    // trên Apple backend (P1) → materialize từ GPU trước, rồi resize.
    std::vector<uint8_t>* MutablePbo(GLuint buf, size_t end);
    // Texture: glTexSubImage* ghi region vào staging (kèm payload bytes — P1);
    // replaceRegion dồn đến FlushTextureStaging() (trước draw sampling texture đó,
    // blit/readback). Merge khi cùng texture và regions kề nhau cùng row pitch.
    // bytes tight w*h*bpp: đi theo region nên flush KHÔNG cần shadow CPU — đó là
    // thứ cho phép bỏ bản sao base-level (P1: mỗi texture từng giữ 2 bản).
    struct TexRegion { uint32_t x = 0, y = 0, w = 0, h = 0; GLenum format = 0; GLenum type = 0;
                       std::vector<uint8_t> bytes; };
    std::unordered_map<GLuint, std::vector<TexRegion>> pendingTexRegions;
    void FlushTextureStaging(GLuint tex); // flush 1 texture (gộp regions thành bbox)
    void FlushAllTextureStaging();
    // IR: giữ temp buffers (uniform/UBO/index rewrite) sống đến flush.
    // Encoder Metal giữ con trỏ MTLBuffer; nếu shared_ptr chết trước commit,
    // ARC release có thể thu hồi trước khi GPU chạy → phải giữ ở đây.
    std::vector<std::shared_ptr<metal::IBuffer>> pendingKeep;
    void FlushPendingEncoder(); // end+commitNoWait, xóa shadow (giữ pipeline cache)
    void InvalidatePendingOnTargetChange(); // helper khi FBO đổi (flush nếu target khác)
    // ---- Deferred full (Phase 1-3): diag gate + conditional flush + ring ----
    // DiagOn: chỉ bật diagnostic nặng (fprintf/readback/scan). Đọc opt.diag —
    // launcher bật qua TGLMT_DIAG=1 (mục TGLMT ở tab Đồ hoạ), không cần rebuild.
    // Release/Minecraft thật: tắt để giữ 60fps, vẫn giữ LogDebug callback.
    bool DiagOn();
    // ---- TexBind ring (Fix #6: chẩn đoán bind divergence từ xa) ----
    // Ghi mọi glActiveTexture/glBindTexture/glBindTextureUnit/glBindTextures/
    // glGenTextures/glDeleteTextures (1 dòng/call, không alloc). Dump khi bind
    // id chưa có trong registry (bindheal#) hoặc draw shadow-like sample sai
    // texture (shadowmis#/shadowsig#) → thấy ngay call nào làm unit trỏ sai.
    struct TexBindRec { uint64_t seq; char op; GLuint unit; GLuint tex; GLenum target; };
    static constexpr int kTexRingN = 96;
    TexBindRec texRing[kTexRingN] = {};
    uint32_t texRingHead = 0, texRingCount = 0;
    uint64_t texRingSeq = 0;
    void RecordTexBind(char op, GLuint unit, GLuint tex, GLenum target) {
        TexBindRec& r = texRing[texRingHead];
        r.seq = ++texRingSeq; r.op = op; r.unit = unit; r.tex = tex; r.target = target;
        texRingHead = (texRingHead + 1) % kTexRingN;
        if (texRingCount < kTexRingN) ++texRingCount;
    }
    void DumpTexBindRing(const char* tag); // impl Context.cpp (oldest → newest)
    // Tài nguyên đã dùng trong pass đang mở (để conditional-flush: stage không
    // liên quan thì KHÔNG phá batching). Ghi nhận ở AppleDrawGL khi bind.
    // Vector + linear find thay unordered_map: số phần tử rất nhỏ (vài chục
    // buffer của 1 pass) nên unordered_map chỉ thêm malloc/free mỗi draw.
    struct UsedRec { uint32_t id = 0; };
    std::vector<UsedRec> pendingUsedBuffers;
    std::vector<UsedRec> pendingUsedTextures;
    void NoteBufferUsed(GLuint buf);
    void NoteTextureUsed(GLuint tex);
    static bool UsedHas(const std::vector<UsedRec>& v, GLuint id) {
        for (const auto& r : v) if (r.id == id) return true;
        return false;
    }
    // ---- FBO wrap cache ----
    // device->wrapAsTarget() cấp phát 1 AppleTarget mới + mutex + set lookup mỗi
    // draw (và takeDepthInit phải tra mỗi lần). Cache theo (fbo, colorTex, depthTex)
    // + gen (gen tăng khi texture/FBO bị xoá) cho phép tái dùng target đang mở.
    struct WrapEntry {
        uint64_t gen = 0;
        GLuint fbo = 0;
        const metal::ITexture* col = nullptr;
        const metal::ITexture* dep = nullptr;
        std::shared_ptr<metal::IRenderTarget> tgt;
    };
    static constexpr int kWrapCacheN = 4;
    WrapEntry wrapCache[kWrapCacheN];
    uint32_t wrapNext = 0;
    uint64_t objectGen = 1; // tăng khi texture/FBO bị xoá hoặc resize
    std::shared_ptr<metal::IRenderTarget> WrapTarget(GLuint fbo, metal::ITexture* col,
                                                     metal::ITexture* dep);
    // True nếu stage buffer/tex này bắt buộc flush encoder đang mở (đã dùng
    // trong pass). False → chỉ stage, giữ batching (đếm flushAvoided).
    bool MustFlushForBufferStage(GLuint buf);
    bool MustFlushForTextureStage(GLuint tex);
    // Ring allocator (triple-buffer, WWDC19 pattern): 3×4MB Shared buffers,
    // bump-pointer, xoay theo frame. TempUploads (uniform/UBO/index/fan-expand)
    // lấy từ đây → 0 MTLBuffer alloc trong frame. Trả (buf, offset); nullptr
    // khi chưa init/size quá lớn (caller fallback newBuffer + đếm tempAllocs).
    static constexpr size_t kRingSize = 4 * 1024 * 1024;
    // p6: tăng 3 → 5 (framesInFlight mặc định 4 + 1 slot đang ghi). Giữ 3 trong
    // khi framesInFlight=4 là HAZARD: NextFrame tái dùng slot mà GPU có thể vẫn
    // đang đọc (triple-buffer chỉ an toàn cho ≤2 frame đang bay).
    static constexpr size_t kRingFrames = 5;
    std::shared_ptr<metal::IBuffer> ringBuf[kRingFrames];
    size_t ringCursor[kRingFrames] = {0, 0, 0, 0, 0};
    // Đợi (cbCommitted) của frame gần nhất đã dùng slot ring — NextFrame chỉ
    // reset cursor khi GPU đã execute xong (không đoán theo số slot nữa).
    uint64_t ringUsedGen[kRingFrames] = {0, 0, 0, 0, 0};
    uint64_t frameSeq = 0; // tăng mỗi EndFrame/NextFrame (triple-buffer rotation)
    std::pair<metal::IBuffer*, size_t> RingAlloc(size_t n, size_t align = 256);
    void NextFrame(); // xoay ring + tăng frameSeq (gọi ở BeginFrame/EndFrame)
    // commitAndWait bọc lại để đếm "thế hệ GPU đã kịp execute". Mọi site
    // device->commitAndWait() PHẢI qua đây, nếu không waitGen không tăng và
    // FlushBufferStaging sẽ orphan thừa (đúng nhưng chậm).
    void CommitAndWait();
    uint64_t waitGen = 1; // tăng mỗi lần GPU đã execute xong mọi command đã commit
    // ---- In-flight tracking (thay cho waitGen mỗi frame) ----
    // AppleDevice tăng cbCommitted mỗi [commandBuffer commit] và cbCompleted trong
    // addCompletedHandler. Một queue Metal thực thi IN ORDER ⇒ cbCompleted ==
    // số việc đã xong; chênh lệch = số frame việc đang bay.
    // In-flight write (ghi đè shared memory GPU đang đọc) chỉ xảy ra khi
    // GpuBusy(); buffer lúc đó xoay sang pool slot khác thay vì alloc mới.
    std::atomic<uint64_t> cbCommitted{0}; // Metal command buffer đã commit
    std::atomic<uint64_t> cbCompleted{0}; // Metal command buffer đã execute xong
    bool GpuBusy() const {
        return cbCommitted.load(std::memory_order_relaxed) >
               cbCompleted.load(std::memory_order_relaxed);
    }
    // Số frame trong flight tối đa trước khi buộc chờ (bảo vệ target/buffer pool).
    // p6: MỖI frame commit ~3 CB (batch + blit NoWait + present) nên ngân sách 2
    // làm gap luôn ≥2 → ThrottleGpu chờ mỗi frame (p5: thr=9592/9600, stall
    // 23.1ms/frame = 45% dt). Đặt 4 = chừa chỗ cho 3 CB + 1 frame GPU đang chạy.
    static constexpr uint32_t kMaxFramesInFlight = 4;
    // Cấu hình runtime: launcher setenv TRƯỚC khi Context sinh ra (tab Đồ hoạ →
    // mục TGLMT). Compile-time chỉ là mặc định → không phải build lại dylib để
    // đổi. Mọi key đọc 1 lần trong LoadOptions(), tôn trọng khoảng [1..8].
    //   TGLMT_FRAMES_IN_FLIGHT 1..8  (mặc định kMaxFramesInFlight)
    //   TGLMT_BUF_POOL         1..8  (mặc định 3 — slot buffer xoay khi GPU bận)
    //   TGLMT_POOL_MB          0..1024 (mặc định 192 — trần TỔNG slot pool).
    //   TGLMT_DIAG             1     (xem DiagOn)
    struct Options {
        uint32_t framesInFlight = kMaxFramesInFlight;
        // p6: slot pool nên ≥ framesInFlight (4) để xoay không rơi vào nhánh chờ.
        size_t bufPoolSlots = 4;
        // Trần pool (MB). Slot pool từng đo tới 320MB sau spike load chunk →
        // góp phần chạm jetsam (limit 2148MB). Vượt trần → KHÔNG alloc slot mới,
        // rớt vào nhánh chờ (CommitAndWait) thay vì giữ thêm RAM. 0 = không giới hạn.
        size_t poolBudgetMB = 192;
        bool diag = false;
    };
    Options opt;
    static Options LoadOptions();
    // Chặn CPU chỉ khi vượt ngân sách frame đang bay. Trước đây SwapBuffers
    // commitAndWait() MỖI frame → CPU/GPU nối tiếp (mất pipeline, +2 frame
    // input latency → lia cam nhanh thấy model đứng lại). Giờ chỉ chặn khi cần.
    void ThrottleGpu();
    // P0: đo lại shadow CPU vs GPU resource → appleStats.mem*. Chỉ để chẩn đoán
    // (quét toàn bộ textures/buffers) — gọi trước khi log frame=, không gọi /frame.
    void RefreshMemStats();
    // P1: đọc level-0 texture từ GPU vào `out` (thoả w*h*4, row = w*4, không lật).
    // Trả false nếu không đọc được (Null / không phải RGBA8-ish / fail) → caller
    // dùng shadow. `out` sống đến hết frame và KHÔNG ghi ngược vào tx.pixels —
    // nếu ghi, shadow sẽ hồi sinh và nuốt mất đúng phần bộ nhớ P1 tiết kiệm.
    bool ReadTextureGPU(const TextureObject& tx, std::vector<uint8_t>& out);
    // PSO prewarm (Phase 4): dựng trước pipeline cho program đã link để frame
    // đầu không hitch giây. Trả true nếu đã đảm bảo pipeline tồn tại.
    bool PrewarmPipelineForProgram(GLuint prog);

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
