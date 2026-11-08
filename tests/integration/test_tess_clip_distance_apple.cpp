// test_tess_clip_distance_apple.cpp — TES gl_ClipDistance[0] phải clip thật.
// Tam giác phủ viewport; clip plane x = -0.5: bên trái (x<-0.5) biến mất,
// bên phải còn đỏ. Nếu [[clip_distance]] bị bỏ qua, cả hai bên đều đỏ.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

static tglmt::GLuint Compile(tglmt::GLenum t, const char* s) {
    using namespace tglmt::gl;
    tglmt::GLuint h = glCreateShader(t);
    glShaderSource(h, 1, &s, nullptr);
    glCompileShader(h);
    tglmt::GLint ok = 0; glGetShaderiv(h, 0x8B81, &ok);
    if (!ok) { char log[2048] = {}; glGetShaderInfoLog(h, sizeof(log), nullptr, log); printf("compile: %s\n", log); return 0; }
    return h;
}
int main() {
    tglmt::Context ctx("apple"); tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_tess_clip_distance_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);
    const char* vs = "#version 460 core\nvoid main(){ gl_Position=vec4(0.0); }\n";
    const char* tcs =
        "#version 460 core\nlayout(vertices=3) out;\n"
        "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n"
        "void main(){\n"
        "  vec2 c[3]=vec2[3](vec2(-1.0,-1.0),vec2(3.0,-1.0),vec2(-1.0,3.0));\n"
        "  gl_out[gl_InvocationID].gl_Position=vec4(c[gl_InvocationID],0.0,1.0);\n"
        "  gl_TessLevelOuter[0]=2.0; gl_TessLevelOuter[1]=2.0; gl_TessLevelOuter[2]=2.0;\n"
        "  gl_TessLevelInner[0]=2.0;\n}\n";
    const char* tes =
        "#version 460 core\nlayout(triangles,equal_spacing,ccw) in;\n"
        "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
        "void main(){\n"
        "  vec3 uv=gl_TessCoord;\n"
        "  vec2 p=gl_in[0].gl_Position.xy*uv.x+gl_in[1].gl_Position.xy*uv.y+gl_in[2].gl_Position.xy*uv.z;\n"
        "  gl_ClipDistance[0]=p.x+0.5;\n"
        "  gl_Position=vec4(p,0.0,1.0);\n}\n";
    const char* fs = "#version 460 core\nout vec4 c;\nvoid main(){ c=vec4(1,0,0,1); }\n";
    auto v=Compile(0x8B31,vs), tc=Compile(0x8E88,tcs), te=Compile(0x8E87,tes), f=Compile(0x8B30,fs);
    if(!v||!tc||!te||!f) return 1;
    auto p=glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,tc); glAttachShader(p,te); glAttachShader(p,f);
    glLinkProgram(p);
    tglmt::GLint ok=0; glGetProgramiv(p,0x8B82,&ok);
    if(!ok){ char log[4096]={}; glGetProgramInfoLog(p,sizeof(log),nullptr,log); printf("link: %s\n",log); return 1; }
    tglmt::GLuint vao=0; glGenVertexArrays(1,&vao); glBindVertexArray(vao);
    glViewport(0,0,64,64); glDisable(0x0B71); glDisable(0x0B44); glDisable(0x0BE2);
    glClearColor(0,0,1,1); glClear(0x4000);
    glUseProgram(p); glPatchParameteri(0x8E72,3);
    glDrawArrays(0x000E,0,3);
    if (glGetError()!=0) { printf("FAIL draw\n"); return 1; }
    ctx.FlushPendingEncoder(); ctx.CommitAndWait();
    unsigned char l[4]={}, r[4]={};
    glReadPixels(8,32,1,1,0x1908,0x1401,l);
    glReadPixels(56,32,1,1,0x1908,0x1401,r);
    printf("left=(%u,%u,%u) right=(%u,%u,%u)\n",l[0],l[1],l[2],r[0],r[1],r[2]);
    bool okL = l[0] < 50 && l[2] > 200;
    bool okR = r[0] > 200 && r[2] < 50;
    if (!okL || !okR) { printf("FAIL clip_distance (left must blue, right red)\n"); return 1; }
    printf("test_tess_clip_distance_apple PASS\n");
    return 0;
}
