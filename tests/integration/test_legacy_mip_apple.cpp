// test_legacy_mip_apple.cpp — glTexImage2D level>0 (non-immutable) tạo mip chain GPU.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <vector>
using namespace tglmt;
using namespace tglmt::gl;
static int gFails=0;
static void Check(bool ok,const char* m){if(!ok){printf("test_legacy_mip_apple FAIL: %s\n",m);++gFails;}}
static const char* kVS=
 "#version 460 core\nlayout(location=0) in vec2 p; layout(location=0) out vec2 uv;\n"
 "void main(){uv=p*0.5+0.5; gl_Position=vec4(p,0,1);}\n";
static const char* kFS=
 "#version 460 core\nin vec2 uv; out vec4 c; uniform sampler2D t;\n"
 "void main(){c=textureLod(t,uv,1.0);}\n";
static GLuint Make(){
 GLuint v=glCreateShader(0x8B31);glShaderSource(v,1,&kVS,nullptr);glCompileShader(v);
 GLuint f=glCreateShader(0x8B30);glShaderSource(f,1,&kFS,nullptr);glCompileShader(f);
 GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);return p; }
int main(){
 Context ctx("apple"); Context::MakeCurrent(&ctx);
 if(ctx.device->isNull()){printf("test_legacy_mip_apple SKIP: no MTL device\n");return 0;}
 while(glGetError()!=GL_NO_ERROR){}
 std::vector<unsigned char> red(8*8*4), green(4*4*4);
 for(size_t i=0;i<red.size();i+=4){red[i]=255;red[i+3]=255;}
 for(size_t i=0;i<green.size();i+=4){green[i+1]=255;green[i+3]=255;}
 GLuint tex=0; glGenTextures(1,&tex); glBindTexture(0x0DE1,tex);
 glTexImage2D(0x0DE1,0,0x8058,8,8,0,0x1908,0x1401,red.data());
 glTexImage2D(0x0DE1,1,0x8058,4,4,0,0x1908,0x1401,green.data());
 glTexParameteri(0x0DE1,0x2801,0x2703); // LINEAR_MIPMAP_LINEAR
 Check(glGetError()==GL_NO_ERROR,"legacy TexImage2D mip no error");
 GLuint prog=Make();
 GLuint vao=0,vbo=0;glGenVertexArrays(1,&vao);glBindVertexArray(vao);
 float tri[6]={-1,-1,3,-1,-1,3};glGenBuffers(1,&vbo);glBindBuffer(0x8892,vbo);
 glBufferData(0x8892,sizeof(tri),tri,0x88E4);glEnableVertexAttribArray(0);
 glVertexAttribPointer(0,2,0x1406,0,8,(void*)0);
 glUseProgram(prog);glUniform1i(glGetUniformLocation(prog,"t"),0);
 GLuint rt=0,fbo=0; glGenTextures(1,&rt); glActiveTexture(0x84C1);
 glBindTexture(0x0DE1,rt);
 glTexImage2D(0x0DE1,0,0x8058,8,8,0,0x1908,0x1401,nullptr);
 glGenFramebuffers(1,&fbo); glBindFramebuffer(0x8D40,fbo);
 glFramebufferTexture2D(0x8D40,0x8CE0,0x0DE1,rt,0);
 Check(glCheckFramebufferStatus(0x8D40)==0x8CD5,"target FBO complete");
 glActiveTexture(0x84C0); glBindTexture(0x0DE1,tex);
 glViewport(0,0,8,8);glClearColor(0,0,1,1);glClear(0x4000);glDrawArrays(0x0004,0,3);
 Check(glGetError()==GL_NO_ERROR,"legacy mip draw");
 ctx.FlushPendingEncoder();ctx.CommitAndWait();
 unsigned char px[4]={};glReadPixels(4,4,1,1,0x1908,0x1401,px);
 Check(px[1]>200 && px[0]<50,"TexImage2D level1 samples green via GPU mip chain");
 glDeleteProgram(prog);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);
 glDeleteFramebuffers(1,&fbo);glDeleteTextures(1,&rt);glDeleteTextures(1,&tex);
 if(gFails==0){printf("test_legacy_mip_apple PASS\n");return 0;}return 1;
}
