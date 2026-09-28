#version 460 core
layout(location = 0) in vec3 vWorld;
layout(location = 0) out vec4 fragColor;
layout(location = 0) uniform float uTime;
layout(location = 1) uniform vec3 uCamPos;
void main() {
    vec3 base = vec3(0.76, 0.7, 0.5);
    float c1 = sin(vWorld.x * 2.0 + uTime * 0.8) * sin(vWorld.z * 2.0 - uTime * 0.6);
    float caustic = pow(abs(c1), 6.0);
    float dist = length(vWorld - uCamPos);
    float fog = 1.0 - exp(-dist * 0.045);
    float depthShade = 1.0 - gl_FragCoord.z * 0.3;
    vec3 col = mix(base + caustic * 0.5, vec3(0.05, 0.2, 0.35), fog) * depthShade;
    fragColor = vec4(col, 1.0);
}
