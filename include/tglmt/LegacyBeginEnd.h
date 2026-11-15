#pragma once
// LegacyBeginEnd — lệnh BỊ CẤM trong glBegin/glEnd (Compatibility Profile).
//
// Spec OpenGL 2.1 §2.6.3 "GL Commands within Begin/End" (glspec21.pdf
// PDF p.19–20) — văn bản giữ nguyên trong OpenGL 4.2/4.6 Compatibility:
//
//   "The only GL commands that are allowed within any Begin/End pairs are the
//    commands for specifying vertex coordinates, vertex colors, normal
//    coordinates, texture coordinates, generic vertex attributes, and fog
//    coordinates (Vertex, Color, SecondaryColor, Index, Normal, TexCoord and
//    MultiTexCoord, VertexAttrib, FogCoord), the ArrayElement command, the
//    EvalCoord and EvalPoint commands, commands for specifying lighting
//    material parameters (Material commands), display list invocation commands
//    (CallList and CallLists), and the EdgeFlag command.
//    Executing any other GL command between the execution of Begin and the
//    corresponding execution of End results in the error INVALID_OPERATION."
//
// ⇒ Mọi entry point khác gọi trong Begin/End phải sinh GL_INVALID_OPERATION
//   và KHÔNG được thực thi.
namespace tglmt {

class Context;

// true nếu lệnh `name` (vd "glLightfv") bị cấm trong glBegin/glEnd.
bool LegacyCommandForbiddenInBeginEnd(const char* name);

// Ghi GL_INVALID_OPERATION nếu lệnh `name` bị cấm trong glBegin/glEnd.
// Trả true nếu lệnh bị chặn ⇒ caller phải return ngay (không thực thi).
bool LegacyCheckBeginEndAllowed(Context& c, const char* name);

}  // namespace tglmt
