#version 460 core
layout(location=0) in vec2 pos;
layout(location=1) in vec4 col;
layout(location=0) out vec4 vCol;
void main(){ gl_Position = vec4(pos.x, -pos.y, 0.5, 1.0); vCol = col; }
