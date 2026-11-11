#version 460 core
flat in int vId;
layout(location = 0) out vec4 oColor;
void main() { oColor = vec4(float(vId), 0.0, 0.0, 1.0); }
