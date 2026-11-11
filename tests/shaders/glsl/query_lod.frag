#version 460 core
// textureQueryLod phải được dịch (trước đây → MSL không compile).
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 oColor;
uniform sampler2D uTex;
void main() {
    vec2 lod = textureQueryLod(uTex, vUV);
    oColor = vec4(lod, 0.0, 1.0);
}
