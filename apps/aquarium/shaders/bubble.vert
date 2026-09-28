#version 460 core
// Bọt khí: POINTS, kích thước theo uSize, alpha theo vA.
layout(location = 0) in vec3 pos;
layout(location = 0) out float vA;
layout(location = 0) uniform mat4 uMVP;
layout(location = 1) uniform float uSize;
void main() {
    gl_Position = uMVP * vec4(pos, 1.0);
    gl_PointSize = uSize;
    vA = 0.5;
}
