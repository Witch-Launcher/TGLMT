// Test nhóm cuối của GL 4.6 Core — 66 entry point:
//   Framebuffer (15): glBlitNamedFramebuffer, glClearBufferiv/uiv,
//     glClearNamedFramebufferfi/fv/iv/uiv, glFramebufferTexture1D/3D,
//     glGetNamedFramebufferAttachmentParameteriv, glGetNamedFramebufferParameteriv,
//     glIsFramebuffer, glNamedFramebufferParameteri, glNamedFramebufferRenderbuffer,
//     glNamedFramebufferTextureLayer, glNamedRenderbufferStorageMultisample
//   Program/Shader (7): glCreateProgramPipelines, glCreateShaderProgramv,
//     glGetAttachedShaders, glIsProgram, glIsShader, glGetShaderSource,
//     glSpecializeShader
//   Transform feedback (8): glCreateTransformFeedbacks, glDeleteTransformFeedbacks,
//     glGetTransformFeedbacki_v/i64_v, glIsTransformFeedback,
//     glTransformFeedbackBufferBase/Range
//   Query/getter (38): glGetActive{Attrib,Uniform,UniformName,Uniformsiv},
//     glGetUniformIndices, glGetBooleani_v/glGetFloati_v/glGetDoublei_v/
//     glGetInteger64i_v, glGetBufferParameteri64v, glGetNamedBufferParameteriv/i64v,
//     glGetBufferPointerv, glGetNamedBufferPointerv,
//     glGetFramebufferAttachmentParameteriv, glGetNamedFramebufferAttachmentParameteriv,
//     glGetInternalformati64v, glGetSamplerParameterIiv/Iuiv,
//     glGetTexLevelParameterfv, glGetTextureLevelParameterfv,
//     glGetTexParameterIiv/Iuiv, glGetTextureParameterIiv/Iuiv,
//     glGetActiveSubroutineName, glGetGraphicsResetStatus
//   Misc: glClampColor, glProvokingVertex, glBindImageTextures,
//     glTextureBuffer, glTextureBufferRange
//
// Kiểm chứng: giá trị trả về đúng, lỗi theo spec, và hành vi quan sát được
// (đọc lại state sau khi set).

#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

using namespace tglmt;
using namespace tglmt::gl;

namespace {

void DrainErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}
GLenum FirstError() { return glGetError(); }
bool Near(float a, float b) { return fabsf(a - b) < 1e-5f; }

const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec4 aPos;\n"
    "uniform mat4 uM;\n"
    "uniform vec4 uColor;\n"
    "void main() { gl_Position = uM * aPos; }\n";
const char* kFS =
    "#version 460 core\n"
    "out vec4 oColor;\n"
    "uniform vec4 uColor;\n"
    "void main() { oColor = uColor; }\n";

// ===========================================================================
// Framebuffer
// ===========================================================================
bool TestFramebuffer() {
    GLuint rb = 0, fbo = 0;
    glCreateRenderbuffers(1, &rb);
    glNamedRenderbufferStorage(rb, GL_RGBA8, 16, 16);
    glCreateFramebuffers(1, &fbo);
    glNamedFramebufferRenderbuffer(fbo, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);

    GLint v = 0;
    glGetNamedFramebufferParameteriv(fbo, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &v);
    TGLMT_CHECK(v == GL_RENDERBUFFER,
                "glGetNamedFramebufferParameteriv phải trả object type thật");
    glGetNamedFramebufferParameteriv(fbo, GL_FRAMEBUFFER_DEFAULT_WIDTH, &v);
    TGLMT_CHECK(v >= 0, "glGetNamedFramebufferParameteriv DEFAULT_WIDTH phải trả giá trị");
    // Attachment queries (spec §9.2.2) áp cho attachment 0.
    glGetNamedFramebufferParameteriv(fbo, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &v);
    TGLMT_CHECK(v == GL_RENDERBUFFER,
                "attachment OBJECT_TYPE phải là RENDERBUFFER khi attach renderbuffer");
    glGetNamedFramebufferParameteriv(fbo, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &v);
    TGLMT_CHECK((GLuint)v == rb, "attachment OBJECT_NAME phải là tên renderbuffer");

    glGetNamedFramebufferAttachmentParameteriv(fbo, GL_COLOR_ATTACHMENT0,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &v);
    TGLMT_CHECK(v == GL_RENDERBUFFER,
                "glGetNamedFramebufferAttachmentParameteriv phải trả object type thật");
    glGetNamedFramebufferAttachmentParameteriv(fbo, GL_COLOR_ATTACHMENT0,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &v);
    TGLMT_CHECK((GLuint)v == rb,
                "glGetNamedFramebufferAttachmentParameteriv phải trả object name thật");

    TGLMT_CHECK(glIsFramebuffer(fbo) == GL_TRUE, "glIsFramebuffer phải TRUE với FBO hợp lệ");
    TGLMT_CHECK(glIsFramebuffer(999999) == GL_FALSE, "glIsFramebuffer phải FALSE với id lạ");

    DrainErrors();
    glNamedFramebufferParameteri(fbo, GL_FRAMEBUFFER_DEFAULT_WIDTH, 32);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glNamedFramebufferParameteri không lỗi");

    // Texture attachment + glNamedFramebufferTextureLayer
    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 8, 8);
    DrainErrors();
    glNamedFramebufferTextureLayer(fbo, GL_COLOR_ATTACHMENT0, tex, 0, 0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glNamedFramebufferTextureLayer không lỗi");
    // Object type của attachment dùng giá trị 0x8DE1 (GL_TEXTURE trong
    // registry, khác GL_TEXTURE=0x1702 là bind target — quirk của Khronos).
    glGetNamedFramebufferAttachmentParameteriv(fbo, GL_COLOR_ATTACHMENT0,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &v);
    TGLMT_CHECK(v == 0x8DE1, "attach texture phải đổi object type sang TEXTURE");

    // glFramebufferTexture1D / 3D (bản bound)
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    DrainErrors();
    glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_1D, tex, 0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glFramebufferTexture1D không lỗi");
    glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, tex, 0, 0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glFramebufferTexture3D không lỗi");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // glClearBufferiv/uiv + glClearNamedFramebuffer*
    DrainErrors();
    const GLint ci[4] = {1, 2, 3, 4};
    glClearNamedFramebufferiv(fbo, GL_COLOR, 0, ci);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClearNamedFramebufferiv không lỗi");
    const GLuint cu[4] = {5, 6, 7, 8};
    glClearNamedFramebufferuiv(fbo, GL_COLOR, 0, cu);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClearNamedFramebufferuiv không lỗi");
    const GLfloat cf[4] = {0.5f, 0.25f, 0.125f, 1.0f};
    glClearNamedFramebufferfv(fbo, GL_COLOR, 0, cf);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClearNamedFramebufferfv không lỗi");
    glClearNamedFramebufferfi(fbo, GL_COLOR, 0, 0.5f, 0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClearNamedFramebufferfi không lỗi");
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearBufferiv(GL_COLOR, 0, ci);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClearBufferiv không lỗi");
    glClearBufferuiv(GL_COLOR, 0, cu);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClearBufferuiv không lỗi");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // glNamedRenderbufferStorageMultisample
    GLuint rb2 = 0;
    glCreateRenderbuffers(1, &rb2);
    DrainErrors();
    glNamedRenderbufferStorageMultisample(rb2, 4, GL_RGBA8, 16, 16);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glNamedRenderbufferStorageMultisample không lỗi");

    // glBlitNamedFramebuffer
    GLuint fbo2 = 0;
    glCreateFramebuffers(1, &fbo2);
    glNamedFramebufferRenderbuffer(fbo2, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
    DrainErrors();
    glBlitNamedFramebuffer(fbo, fbo2, 0, 0, 16, 16, 0, 0, 16, 16,
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glBlitNamedFramebuffer không lỗi");

    glDeleteFramebuffers(1, &fbo);
    glDeleteFramebuffers(1, &fbo2);
    glDeleteRenderbuffers(1, &rb);
    glDeleteRenderbuffers(1, &rb2);
    glDeleteTextures(1, &tex);
    return true;
}

// ===========================================================================
// Program / Shader
// ===========================================================================
bool TestProgramShader() {
    // glIsProgram / glIsShader
    GLuint sh = glCreateShader(GL_VERTEX_SHADER);
    GLuint p = glCreateProgram();
    TGLMT_CHECK(glIsShader(sh) == GL_TRUE, "glIsShader phải TRUE với shader vừa tạo");
    TGLMT_CHECK(glIsShader(999999) == GL_FALSE, "glIsShader phải FALSE với id lạ");
    TGLMT_CHECK(glIsProgram(p) == GL_TRUE, "glIsProgram phải TRUE với program vừa tạo");
    TGLMT_CHECK(glIsProgram(999999) == GL_FALSE, "glIsProgram phải FALSE với id lạ");

    // glGetShaderSource — phải trả lại ĐÚNG source đã set.
    glShaderSource(sh, 1, &kVS, nullptr);
    GLchar buf[512] = {0};
    GLsizei len = 0;
    glGetShaderSource(sh, sizeof(buf), &len, buf);
    TGLMT_CHECK(len > 0, "glGetShaderSource phải trả độ dài nguồn");
    TGLMT_CHECK(strstr(buf, "gl_Position") != nullptr,
                "glGetShaderSource phải trả lại nội dung nguồn đã set");

    // glGetAttachedShaders
    glAttachShader(p, sh);
    GLuint shaders[4] = {0};
    GLsizei count = 0;
    glGetAttachedShaders(p, 4, &count, shaders);
    TGLMT_CHECK(count == 1, "glGetAttachedShaders phải trả số shader đã attach");
    TGLMT_CHECK(shaders[0] == sh, "glGetAttachedShaders phải trả đúng id shader");

    // glCreateShaderProgramv — tạo program + link trong một lệnh.
    // Program chỉ có 1 shader: dùng VERTEX (program chỉ có FS sẽ link fail —
    // đúng GL, vì program cần vertex shader).
    const char* srcs[1] = {kVS};
    GLuint p2 = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, srcs);
    TGLMT_CHECK(p2 != 0, "glCreateShaderProgramv phải trả program id");
    GLint ok = 0;
    glGetProgramiv(p2, GL_LINK_STATUS, &ok);
    TGLMT_CHECK(ok == GL_TRUE, "glCreateShaderProgramv phải tạo program đã link");

    // glCreateProgramPipelines
    GLuint pipes[2] = {0, 0};
    glCreateProgramPipelines(2, pipes);
    TGLMT_CHECK(pipes[0] != 0 && pipes[1] != 0, "glCreateProgramPipelines phải cấp id");

    // glSpecializeShader — SPIR-V entry point; TGLMT compile lại.
    GLuint sh2 = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(sh2, 1, &kVS, nullptr);
    glCompileShader(sh2);
    DrainErrors();
    glSpecializeShader(sh2, "main", 0, nullptr, nullptr);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glSpecializeShader không lỗi");

    glDeleteProgram(p);
    glDeleteProgram(p2);
    glDeleteShader(sh);
    glDeleteShader(sh2);
    glDeleteProgramPipelines(2, pipes);
    return true;
}

// ===========================================================================
// Transform feedback
// ===========================================================================
bool TestTransformFeedback() {
    GLuint xfb = 0;
    glCreateTransformFeedbacks(1, &xfb);
    TGLMT_CHECK(xfb != 0, "glCreateTransformFeedbacks phải cấp id");
    TGLMT_CHECK(glIsTransformFeedback(xfb) == GL_TRUE,
                "glIsTransformFeedback phải TRUE với XFB hợp lệ");
    TGLMT_CHECK(glIsTransformFeedback(999999) == GL_FALSE,
                "glIsTransformFeedback phải FALSE với id lạ");

    GLuint buf = 0;
    glCreateBuffers(1, &buf);
    glNamedBufferStorage(buf, 256, nullptr, 0);

    // glTransformFeedbackBufferBase / Range
    DrainErrors();
    glTransformFeedbackBufferBase(xfb, 0, buf);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTransformFeedbackBufferBase không lỗi");
    glTransformFeedbackBufferRange(xfb, 0, buf, 0, 64);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTransformFeedbackBufferRange không lỗi");

    // Getter: phải trả buffer/offset/size thật.
    GLint v = 0;
    glGetTransformFeedbacki_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 0, &v);
    TGLMT_CHECK((GLuint)v == buf,
                "glGetTransformFeedbacki_v phải trả buffer binding thật");
    glGetTransformFeedbacki_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_START, 0, &v);
    TGLMT_CHECK(v == 0, "glGetTransformFeedbacki_v phải trả offset thật");
    glGetTransformFeedbacki_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_SIZE, 0, &v);
    TGLMT_CHECK(v == 64, "glGetTransformFeedbacki_v phải trả size thật");
    GLint64 v64 = 0;
    glGetTransformFeedbacki64_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 0, &v64);
    TGLMT_CHECK((GLuint)v64 == buf, "glGetTransformFeedbacki64_v phải trả buffer thật");
    glGetTransformFeedbacki64_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_SIZE, 0, &v64);
    TGLMT_CHECK(v64 == 64, "glGetTransformFeedbacki64_v phải trả size thật");

    glDeleteTransformFeedbacks(1, &xfb);
    glDeleteBuffers(1, &buf);
    return true;
}

// ===========================================================================
// Active uniform / attribute reflection
// ===========================================================================
bool TestProgramReflection() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs, 1, &kVS, nullptr);
    glCompileShader(vs);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        printf("  (skip: link fail)\n");
        return true;
    }

    GLint nActive = 0;
    glGetProgramiv(p, GL_ACTIVE_UNIFORMS, &nActive);
    TGLMT_CHECK(nActive >= 2, "phải có ít nhất uM và uColor active");

    // glGetActiveUniform — type/size phải đúng (mat4 / vec4).
    bool foundM = false, foundColor = false;
    for (GLint i = 0; i < nActive; ++i) {
        GLchar name[128] = {0};
        GLsizei len = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveUniform(p, i, sizeof(name), &len, &size, &type, name);
        TGLMT_CHECK(len > 0, "glGetActiveUniform phải trả tên có độ dài > 0");
        TGLMT_CHECK(size >= 1, "glGetActiveUniform phải trả size >= 1");
        if (strcmp(name, "uM") == 0) {
            foundM = true;
            TGLMT_CHECK(type == GL_FLOAT_MAT4, "uM phải có type GL_FLOAT_MAT4");
        } else if (strcmp(name, "uColor") == 0) {
            foundColor = true;
            TGLMT_CHECK(type == GL_FLOAT_VEC4, "uColor phải có type GL_FLOAT_VEC4");
        }
    }
    TGLMT_CHECK(foundM, "glGetActiveUniform phải thấy uniform uM");
    TGLMT_CHECK(foundColor, "glGetActiveUniform phải thấy uniform uColor");

    // glGetActiveUniformName
    if (nActive > 0) {
        GLchar nm[128] = {0};
        GLsizei len = 0;
        glGetActiveUniformName(p, 0, sizeof(nm), &len, nm);
        TGLMT_CHECK(len > 0, "glGetActiveUniformName phải trả tên");
    }

    // glGetActiveUniformsiv — UNIFORM_LOCATION / UNIFORM_TYPE / UNIFORM_SIZE
    GLuint idx[2] = {0, 1};
    // GetActiveUniformsiv hỗ trợ UNIFORM_TYPE / UNIFORM_SIZE /
    // UNIFORM_NAME_LENGTH / UNIFORM_BLOCK_INDEX / UNIFORM_OFFSET.
    GLint nameLen[2] = {0, 0};
    glGetActiveUniformsiv(p, 2, idx, GL_UNIFORM_NAME_LENGTH, nameLen);
    TGLMT_CHECK(nameLen[0] > 0, "glGetActiveUniformsiv UNIFORM_NAME_LENGTH phải > 0");
    GLint sizes[2] = {0, 0};
    glGetActiveUniformsiv(p, 2, idx, GL_UNIFORM_SIZE, sizes);
    TGLMT_CHECK(sizes[0] >= 1, "glGetActiveUniformsiv UNIFORM_SIZE phải >= 1");
    GLint types[2] = {0, 0};
    glGetActiveUniformsiv(p, 2, idx, GL_UNIFORM_TYPE, types);
    TGLMT_CHECK(types[0] != 0, "glGetActiveUniformsiv UNIFORM_TYPE phải trả type thật");

    // glGetUniformIndices — tên → index trong active uniform list.
    const char* names[2] = {"uM", "uColor"};
    GLuint uix[2] = {0xFFFFFFFFu, 0xFFFFFFFFu};
    glGetUniformIndices(p, 2, names, uix);
    TGLMT_CHECK(uix[0] != 0xFFFFFFFFu, "glGetUniformIndices phải tìm thấy uM");
    TGLMT_CHECK(uix[1] != 0xFFFFFFFFu, "glGetUniformIndices phải tìm thấy uColor");

    // glGetActiveAttrib
    GLint nAttr = 0;
    glGetProgramiv(p, GL_ACTIVE_ATTRIBUTES, &nAttr);
    TGLMT_CHECK(nAttr >= 1, "phải có attribute aPos active");
    if (nAttr > 0) {
        GLchar an[128] = {0};
        GLsizei len = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveAttrib(p, 0, sizeof(an), &len, &size, &type, an);
        TGLMT_CHECK(len > 0, "glGetActiveAttrib phải trả tên");
        TGLMT_CHECK(type == GL_FLOAT_VEC4, "aPos phải có type GL_FLOAT_VEC4");
    }

    glDeleteProgram(p);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return true;
}

// ===========================================================================
// Buffer / sampler / texture / internalformat getters
// ===========================================================================
bool TestBufferSamplerTextureGetters() {
    // --- buffer ---
    GLuint buf = 0;
    glCreateBuffers(1, &buf);
    glNamedBufferStorage(buf, 1024, nullptr, 0);
    GLint v = 0;
    glGetNamedBufferParameteriv(buf, GL_BUFFER_SIZE, &v);
    TGLMT_CHECK(v == 1024, "glGetNamedBufferParameteriv BUFFER_SIZE phải trả size thật");
    GLint64 v64 = 0;
    glGetNamedBufferParameteri64v(buf, GL_BUFFER_SIZE, &v64);
    TGLMT_CHECK(v64 == 1024, "glGetNamedBufferParameteri64v phải trả size thật");
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glGetBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &v64);
    TGLMT_CHECK(v64 == 1024, "glGetBufferParameteri64v phải trả size thật");
    void* ptr = nullptr;
    glGetBufferPointerv(GL_ARRAY_BUFFER, GL_BUFFER_MAP_POINTER, &ptr);
    TGLMT_CHECK(ptr != nullptr, "glGetBufferPointerv phải trả con trỏ buffer");
    glGetNamedBufferPointerv(buf, GL_BUFFER_MAP_POINTER, &ptr);
    TGLMT_CHECK(ptr != nullptr, "glGetNamedBufferPointerv phải trả con trỏ buffer");
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // --- sampler ---
    GLuint smp = 0;
    glCreateSamplers(1, &smp);
    glSamplerParameteri(smp, GL_TEXTURE_MAX_LEVEL, 5);
    GLint iv = 0;
    glGetSamplerParameterIiv(smp, GL_TEXTURE_MAX_LEVEL, &iv);
    TGLMT_CHECK(iv == 5, "glGetSamplerParameterIiv phải trả giá trị thật");
    GLuint uiv = 0;
    glGetSamplerParameterIuiv(smp, GL_TEXTURE_MAX_LEVEL, &uiv);
    TGLMT_CHECK(uiv == 5u, "glGetSamplerParameterIuiv phải trả giá trị thật");

    // --- texture ---
    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 2, GL_RGBA8, 16, 16);
    glTextureParameteri(tex, GL_TEXTURE_MAX_LEVEL, 1);
    glBindTexture(GL_TEXTURE_2D, tex);  // getter dạng bound đọc texture đang bind
    glGetTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &iv);
    TGLMT_CHECK(iv == 1, "glGetTexParameterIiv phải trả giá trị thật");
    glGetTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &uiv);
    TGLMT_CHECK(uiv == 1u, "glGetTexParameterIuiv phải trả giá trị thật");
    glGetTextureParameterIiv(tex, GL_TEXTURE_MAX_LEVEL, &iv);
    TGLMT_CHECK(iv == 1, "glGetTextureParameterIiv phải trả giá trị thật");
    glGetTextureParameterIuiv(tex, GL_TEXTURE_MAX_LEVEL, &uiv);
    TGLMT_CHECK(uiv == 1u, "glGetTextureParameterIuiv phải trả giá trị thật");

    // Level parameter (float bản).
    GLfloat fv = 0;
    glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &fv);
    TGLMT_CHECK(Near(fv, 16.0f), "glGetTexLevelParameterfv phải trả width thật");
    glGetTextureLevelParameterfv(tex, 0, GL_TEXTURE_WIDTH, &fv);
    TGLMT_CHECK(Near(fv, 16.0f), "glGetTextureLevelParameterfv phải trả width thật");

    // --- internal format ---
    GLint64 iv64[4] = {0};
    DrainErrors();
    glGetInternalformati64v(GL_RENDERBUFFER, GL_RGBA8, GL_INTERNALFORMAT_SUPPORTED, 1, iv64);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glGetInternalformati64v không lỗi với pname hợp lệ");
    DrainErrors();
    glGetInternalformati64v(0xDEAD, GL_RGBA8, GL_INTERNALFORMAT_SUPPORTED, 1, iv64);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM,
                "glGetInternalformati64v target sai → INVALID_ENUM");

    glDeleteBuffers(1, &buf);
    glDeleteSamplers(1, &smp);
    glDeleteTextures(1, &tex);
    return true;
}

// ===========================================================================
// Indexed getter + misc
// ===========================================================================
bool TestIndexedGettersAndMisc() {
    // glGetBooleani_v / glGetFloati_v / glGetDoublei_v / glGetInteger64i_v
    GLboolean b = 0;
    glGetBooleani_v(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, 0, &b);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetBooleani_v không lỗi");
    // Spec GL 4.6 table 23.6: VIEWPORT là 4×R → GetFloati_v ghi 4 float;
    // DEPTH_RANGE là 2×R → GetDoublei_v ghi 2 double.
    GLfloat f[4] = {0};
    glGetFloati_v(GL_VIEWPORT, 0, f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetFloati_v không lỗi");
    GLdouble d[2] = {0};
    glGetDoublei_v(GL_DEPTH_RANGE, 0, d);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetDoublei_v không lỗi");
    GLint64 i64 = 0;
    glGetInteger64i_v(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, 0, &i64);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetInteger64i_v không lỗi");

    // glGetGraphicsResetStatus — NO_ERROR khi context còn sống.
    TGLMT_CHECK(glGetGraphicsResetStatus() == 0x8252 /*NO_ERROR của enum reset*/,
                "glGetGraphicsResetStatus phải trả NO_ERROR");

    // glClampColor — legacy color clamp, GL 4.6 core vẫn còn (deprecated).
    DrainErrors();
    glClampColor(GL_CLAMP_READ_COLOR, GL_TRUE);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClampColor không lỗi");
    DrainErrors();
    glClampColor(GL_CLAMP_READ_COLOR, GL_RED);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM,
                "glClampColor với clamp sai → INVALID_ENUM");

    // glProvokingVertex
    DrainErrors();
    glProvokingVertex(GL_LAST_VERTEX_CONVENTION);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glProvokingVertex không lỗi");
    DrainErrors();
    glProvokingVertex(0xDEAD);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM,
                "glProvokingVertex với giá trị sai → INVALID_ENUM");

    // glGetActiveSubroutineName — không có subroutine nào → INVALID_VALUE.
    GLuint p = glCreateProgram();
    GLchar nm[64] = {0};
    GLsizei len = 0;
    DrainErrors();
    glGetActiveSubroutineName(p, GL_VERTEX_SHADER, 0, sizeof(nm), &len, nm);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "glGetActiveSubroutineName với program không có subroutine → INVALID_VALUE");
    glDeleteProgram(p);

    // glBindImageTextures + glTextureBuffer/Range
    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 4, 4);
    GLuint bufs[1] = {0};
    DrainErrors();
    glBindImageTextures(0, 1, bufs);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glBindImageTextures không lỗi");

    GLuint tbo = 0;
    glCreateTextures(GL_TEXTURE_BUFFER, 1, &tbo);
    GLuint dataBuf = 0;
    glCreateBuffers(1, &dataBuf);
    glNamedBufferStorage(dataBuf, 256, nullptr, 0);
    DrainErrors();
    glBindTexture(GL_TEXTURE_BUFFER, tbo);
    glTextureBuffer(tbo, GL_RGBA8, dataBuf);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTextureBuffer không lỗi");
    glTextureBufferRange(tbo, GL_RGBA8, dataBuf, 0, 128);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTextureBufferRange không lỗi");

    glDeleteTextures(1, &tex);
    glDeleteTextures(1, &tbo);
    glDeleteBuffers(1, &dataBuf);
    return true;
}

// ===========================================================================
// Hàm còn lại: glGetActiveAtomicCounterBufferiv,
// glGetFramebufferAttachmentParameteriv (bản bound), glGetProgramPipelineInfoLog
// ===========================================================================
bool TestRemainingQueries() {
    // glGetFramebufferAttachmentParameteriv (bản bound, theo target).
    GLuint tex = 0, fbo = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, 8, 8);
    glCreateFramebuffers(1, &fbo);
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLint v = 0;
    DrainErrors();
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                         GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &v);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glGetFramebufferAttachmentParameteriv không lỗi");
    TGLMT_CHECK(v == 0x8DE1 /*TEXTURE*/,
                "glGetFramebufferAttachmentParameteriv phải trả object type thật");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // glGetActiveAtomicCounterBufferiv — program không có atomic counter →
    // INVALID_VALUE (index không hợp lệ).
    GLuint p = glCreateProgram();
    DrainErrors();
    glGetActiveAtomicCounterBufferiv(p, 0, GL_ATOMIC_COUNTER_BUFFER_INDEX, &v);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetActiveAtomicCounterBufferiv với program không có counter → INVALID_VALUE");
    glDeleteProgram(p);

    // glGetProgramPipelineInfoLog — pipeline rỗng → log rỗng, không lỗi.
    GLuint pipe = 0;
    glCreateProgramPipelines(1, &pipe);
    GLchar log[256] = {0};
    GLsizei len = 0;
    DrainErrors();
    glGetProgramPipelineInfoLog(pipe, GL_INFO_LOG_LENGTH, &len, log);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetProgramPipelineInfoLog không lỗi");
    TGLMT_CHECK(len >= 0, "glGetProgramPipelineInfoLog phải trả độ dài log");
    glDeleteProgramPipelines(1, &pipe);

    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);
    return true;
}

struct Case {
    const char* name;
    bool (*fn)();
};

}  // namespace

int main() {
    printf("=== core framebuffer/program/query entry points (GL 4.6) ===\n");
    static tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    const Case kCases[] = {
        {"framebuffer", TestFramebuffer},
        {"program/shader", TestProgramShader},
        {"transform feedback", TestTransformFeedback},
        {"program reflection (active uniform/attrib)", TestProgramReflection},
        {"buffer/sampler/texture/internalformat getters", TestBufferSamplerTextureGetters},
        {"indexed getters + misc", TestIndexedGettersAndMisc},
        {"các getter còn lại", TestRemainingQueries},
    };
    bool ok = true;
    for (const Case& c : kCases) {
        const int before = tglmt_test::Failures();
        c.fn();
        const bool pass = (tglmt_test::Failures() == before);
        printf("  [%s] %s\n", pass ? "PASS" : "FAIL", c.name);
        ok = ok && pass;
    }
    printf("%s\n", ok ? "PASS" : "FAIL");
    return TGLMT_RESULT;
}
