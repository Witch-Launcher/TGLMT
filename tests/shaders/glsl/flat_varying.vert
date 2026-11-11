#version 460 core
// Varying phải giữ nguyên giá trị (flat) — kiểm tra qualifier nội suy.
layout(location = 0) in vec2 aPos;
flat out int vId;
void main() {
    vId = 3;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
