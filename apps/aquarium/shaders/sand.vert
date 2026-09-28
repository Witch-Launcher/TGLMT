#version 460 core
// Cát: plane lớn, caustics + fog theo khoảng cách (dùng gl_FragCoord.z).
layout(location = 0) in vec3 pos;
layout(location = 0) out vec3 vWorld;
layout(location = 0) uniform mat4 uMVP;
void main() {
    vWorld = pos;
    gl_Position = uMVP * vec4(pos, 1.0);
}
