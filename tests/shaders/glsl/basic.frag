#version 460 core
// Fragment shader cơ bản: texture, bias, phép so sánh vector, nhiều output.
layout(location = 0) in vec2 vUV;
layout(location = 1) in vec3 vNrm;
layout(location = 2) in vec4 vColor;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormal;

uniform sampler2D uAlbedo;
uniform sampler2D uNormalMap;
uniform float uBias;

layout(std140, binding = 1) uniform Material {
    vec4 uBase;
    float uRough;
    float uMetal;
};

void main() {
    vec4 alb = texture(uAlbedo, vUV, uBias) * uBase * vColor;
    vec3 nn = texture(uNormalMap, vUV).xyz * 2.0 - 1.0;
    vec3 n = normalize(vNrm + vec3(nn.xy, 0.0));
    float spec = pow(max(dot(n, normalize(vec3(1.0, 1.0, 1.0))), 0.0),
                     mix(64.0, 4.0, uRough));
    oColor = alb + vec4(spec * uMetal);
    oNormal = vec4(n * 0.5 + 0.5, 1.0);
}
