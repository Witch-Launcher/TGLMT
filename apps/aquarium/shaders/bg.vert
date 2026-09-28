#version 460 core
// Nền: tam giác fullscreen, gradient sâu.
layout(location = 0) in vec2 pos;
layout(location = 0) out float vY;
void main() {
    vY = pos.y;
    gl_Position = vec4(pos, 0.0, 1.0);
}
