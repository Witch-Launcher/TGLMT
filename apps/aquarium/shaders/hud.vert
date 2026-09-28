#version 460 core
// HUD: quad NDC nướng sẵn trên CPU (chữ bitmap 3x5), màu theo vertex.
layout(location = 0) in vec2 pos;
layout(location = 1) in vec4 col;
layout(location = 0) out vec4 vCol;
void main() {
    vCol = col;
    gl_Position = vec4(pos, 0.0, 1.0);
}
