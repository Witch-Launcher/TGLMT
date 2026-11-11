#version 460 core
// Vertex shader cơ bản: biến đổi, nội suy, uniform block.
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNrm;
layout(location = 2) in vec2 aUV;

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec3 vNrm;
layout(location = 2) out vec4 vColor;

layout(std140, binding = 0) uniform Camera {
    mat4 uMVP;
    mat4 uModel;
    vec4 uTint;
};

void main() {
    vUV = aUV;
    vNrm = normalize(mat3(uModel) * aNrm);
    vColor = uTint;
    gl_Position = uMVP * uModel * vec4(aPos, 1.0);
}
