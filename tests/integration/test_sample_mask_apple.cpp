// test_sample_mask_apple.cpp — fixed-function glSampleMaski trên MSAA.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
using namespace tglmt;
using namespace tglmt::gl;
static int gFails=0;
static void Check(bool ok,const char* m){ if(!ok){printf("test_sample_mask_apple FAIL: %s\n",m);++gFails;} }
static const char* kVS="#version 460 core\nlayout(location=0) in vec2 p; void main(){gl_Position=vec4(p,0,1);}\n";
static const char* kFS="#version 460 core\nout vec4 c; void main(){c=vec4(1,0,0,1);}\n";
static GLuint Make(){ GLuint v=glCreateShader(0x8B31);glShaderSource(v,1,&kVS,nullptr);glCompileShader(v);
 GLuint f=glCreateShader(0x8B30);glShaderSource(f,1,&kFS,nullptr);glCompileShader(f);
 GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);return p; }
int main(){
 Context ctx("apple"); Context::MakeCurrent(&ctx);
 if(ctx.device->isNull()){printf("test_sample_mask_apple SKIP: no MTL device\n");return 0;}
 while(glGetError()!=GL_NO_ERROR){}
 GLuint prog=Make(),vao=0,vbo=0;glGenVertexArrays(1,&vao);glBindVertexArray(vao);
 float tri[6]={-1,-1,3,-1,-1,3};glGenBuffers(1,&vbo);glBindBuffer(0x8892,vbo);
 glBufferData(0x8892,sizeof(tri),tri,0x88E4);glEnableVertexAttribArray(0);
 glVertexAttribPointer(0,2,0x1406,0,8,(void*)0);
 GLuint tex=0,fbo=0;glGenTextures(1,&tex);glBindTexture(0x9100,tex);
 glTexImage2DMultisample(0x9100,4,0x8058,32,32,GL_TRUE);
 glGenFramebuffers(1,&fbo);glBindFramebuffer(0x8D40,fbo);
 glFramebufferTexture2D(0x8D40,0x8CE0,0x9100,tex,0);
 Check(glCheckFramebufferStatus(0x8D40)==0x8CD5,"MSAA FBO complete");
 glViewport(0,0,32,32);glUseProgram(prog);glClearColor(0,0,1,1);glClear(0x4000);
 glSampleMaski(0,0);glDrawArrays(0x0004,0,3);
 ctx.FlushPendingEncoder();ctx.CommitAndWait();
 unsigned char px[4]={};glReadPixels(16,16,1,1,0x1908,0x1401,px);
 Check(px[2]>200 && px[0]<50,"mask 0 keeps clear blue");
 glSampleMaski(0,0xFFFFFFFF);glDrawArrays(0x0004,0,3);
 ctx.FlushPendingEncoder();ctx.CommitAndWait();
 glReadPixels(16,16,1,1,0x1908,0x1401,px);
 Check(px[0]>200 && px[2]<50,"mask all draws red");
 glDeleteProgram(prog);glDeleteFramebuffers(1,&fbo);glDeleteTextures(1,&tex);
 if(gFails==0){printf("test_sample_mask_apple PASS\n");return 0;}return 1;
}
