// LegacyDisplayList.h — API ghi lệnh vào display list (Compatibility Profile).
//
// Spec: OpenGL 4.2 Compatibility Profile §5.5 "Display Lists" (PDF p.470+),
// §2.6.3 (PDF p.51) cho phép CallList/CallLists trong Begin/End.
//
// Cơ chế: khi một display list đang được COMPILE, các lệnh state-changing
// được ghi vào list thay vì thực thi. Khi list được REPLAY, các lệnh ghi lại
// được gọi lại đúng như lệnh GL gốc ⇒ "executed just as if they were given
// normally" (spec §5.5).
//
// Quy ước hàm ghi: trả TRUE khi lệnh ĐÃ được ghi và KHÔNG được thực thi
// (chế độ GL_COMPILE) ⇒ caller phải `return` ngay. Trả FALSE ⇒ lệnh thực
// thi bình thường (không ghi, hoặc ghi + thực thi ở GL_COMPILE_AND_EXECUTE).
#ifndef TGLMT_LEGACY_DISPLAY_LIST_H
#define TGLMT_LEGACY_DISPLAY_LIST_H

#include "tglmt/gl46_types.h"

namespace tglmt::gl {

bool LegacyRecordEnable(GLenum cap);
bool LegacyRecordDisable(GLenum cap);
bool LegacyRecordEnableClientState(GLenum array);
bool LegacyRecordDisableClientState(GLenum array);
bool LegacyRecordMatrixMode(GLenum mode);
bool LegacyRecordLoadMatrix(const GLfloat* m, bool transpose);
bool LegacyRecordMultMatrix(const GLfloat* m, bool transpose);
bool LegacyRecordPushMatrix();
bool LegacyRecordPopMatrix();
bool LegacyRecordLoadIdentity();
bool LegacyRecordShadeModel(GLenum mode);
bool LegacyRecordDepthMask(GLboolean flag);
bool LegacyRecordBlendFunc(GLenum src, GLenum dst);
bool LegacyRecordCullFace(GLenum mode);
bool LegacyRecordFrontFace(GLenum mode);
bool LegacyRecordPolygonOffset(GLfloat factor, GLfloat units);
bool LegacyRecordViewport(GLint x, GLint y, GLsizei w, GLsizei h);
bool LegacyRecordScissor(GLint x, GLint y, GLsizei w, GLsizei h);
bool LegacyRecordLineWidth(GLfloat w);
bool LegacyRecordPointSize(GLfloat s);
bool LegacyRecordTexEnv(GLenum target, GLenum pname, const GLfloat* values, int count);
bool LegacyRecordTexGen(GLenum coord, GLenum pname, const GLfloat* values, int count);
bool LegacyRecordLight(GLenum light, GLenum pname, const GLfloat* values, int count);
bool LegacyRecordMaterial(GLenum face, GLenum pname, const GLfloat* values, int count);

}  // namespace tglmt::gl

#endif  // TGLMT_LEGACY_DISPLAY_LIST_H
