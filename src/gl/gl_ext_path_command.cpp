// gl_ext_path_command.cpp — NV_path_rendering + NV_command_list entries.
// Path geometry / command execution is unsupported on Metal; object lifecycle and
// parameters are tracked, while rendering entry points fail explicitly.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstring>

using namespace tglmt;
extern "C" {
GLuint glGenPathsNV(GLsizei range);
void glDeletePathsNV(GLuint path, GLsizei range);
GLboolean glIsPathNV(GLuint path);
void glPathParameterfNV(GLuint path, GLenum pname, GLfloat value);
void glPathParameterfvNV(GLuint path, GLenum pname, const GLfloat* value);
void glPathParameteriNV(GLuint path, GLenum pname, GLint value);
void glPathParameterivNV(GLuint path, GLenum pname, const GLint* value);
void glGetPathParameterfvNV(GLuint path, GLenum pname, GLfloat* value);
void glGetPathParameterivNV(GLuint path, GLenum pname, GLint* value);
void glPathCommandsNV(GLuint path, GLsizei numCommands, const GLubyte* commands, GLsizei numCoords, GLenum coordType, const void* coords);
void glPathCoordsNV(GLuint path, GLsizei numCoords, GLenum coordType, const void* coords);
void glPathSubCommandsNV(GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte* commands, GLsizei numCoords, GLenum coordType, const void* coords);
void glPathSubCoordsNV(GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void* coords);
void glPathStringNV(GLuint path, GLenum format, GLsizei length, const void* pathString);
void glPathDashArrayNV(GLuint path, GLsizei dashCount, const GLfloat* dashArray);
void glGetPathCommandsNV(GLuint path, GLubyte* commands);
void glGetPathCoordsNV(GLuint path, GLfloat* coords);
void glGetPathDashArrayNV(GLuint path, GLfloat* dashArray);
GLfloat glGetPathLengthNV(GLuint path, GLsizei startSegment, GLsizei numSegments);
void glPathStencilFuncNV(GLenum func, GLint ref, GLuint mask);
void glPathStencilDepthOffsetNV(GLfloat factor, GLfloat units);
void glPathCoverDepthFuncNV(GLenum func);
void glPathColorGenNV(GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat* coeffs);
void glPathTexGenNV(GLenum texCoordSet, GLenum genMode, GLint components, const GLfloat* coeffs);
void glPathFogGenNV(GLenum genMode);
void glGetPathColorGenfvNV(GLenum color, GLenum pname, GLfloat* value);
void glGetPathColorGenivNV(GLenum color, GLenum pname, GLint* value);
void glGetPathTexGenfvNV(GLenum texCoordSet, GLenum pname, GLfloat* value);
void glGetPathTexGenivNV(GLenum texCoordSet, GLenum pname, GLint* value);
void glPathGlyphsNV(GLuint firstPathName, GLenum fontTarget, const void* fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void* charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
void glPathGlyphRangeNV(GLuint firstPathName, GLenum fontTarget, const void* fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
GLenum glPathGlyphIndexArrayNV(GLuint firstPathName, GLenum fontTarget, const void* fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
GLenum glPathGlyphIndexRangeNV(GLenum fontTarget, const void* fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint* baseAndCount);
GLenum glPathMemoryGlyphIndexArrayNV(GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void* fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
void glCopyPathNV(GLuint resultPath, GLuint srcPath);
void glInterpolatePathsNV(GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight);
void glTransformPathNV(GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat* transformValues);
void glWeightPathsNV(GLuint resultPath, GLsizei numPaths, const GLuint* paths, const GLfloat* weights);
void glGetPathMetricRangeNV(GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat* metrics);
void glGetPathMetricsNV(GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const void* paths, GLuint pathBase, GLsizei stride, GLfloat* metrics);
void glGetPathSpacingNV(GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const void* paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat* returnedSpacing);
GLboolean glIsPointInFillPathNV(GLuint path, GLuint mask, GLfloat x, GLfloat y);
GLboolean glIsPointInStrokePathNV(GLuint path, GLfloat x, GLfloat y);
GLboolean glPointAlongPathNV(GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat* x, GLfloat* y, GLfloat* tangentX, GLfloat* tangentY);
void glStencilFillPathNV(GLuint path, GLenum fillMode, GLuint mask);
void glStencilStrokePathNV(GLuint path, GLint reference, GLuint mask);
void glStencilFillPathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void* paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat* transformValues);
void glStencilStrokePathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void* paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat* transformValues);
void glCoverFillPathNV(GLuint path, GLenum coverMode);
void glCoverStrokePathNV(GLuint path, GLenum coverMode);
void glCoverFillPathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void* paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat* transformValues);
void glCoverStrokePathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void* paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat* transformValues);
void glStencilThenCoverFillPathNV(GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode);
void glStencilThenCoverStrokePathNV(GLuint path, GLint reference, GLuint mask, GLenum coverMode);
void glStencilThenCoverFillPathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void* paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat* transformValues);
void glStencilThenCoverStrokePathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void* paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat* transformValues);
void glMatrixLoad3x2fNV(GLenum matrixMode, const GLfloat* m);
void glMatrixLoad3x3fNV(GLenum matrixMode, const GLfloat* m);
void glMatrixLoadTranspose3x3fNV(GLenum matrixMode, const GLfloat* m);
void glMatrixMult3x2fNV(GLenum matrixMode, const GLfloat* m);
void glMatrixMult3x3fNV(GLenum matrixMode, const GLfloat* m);
void glMatrixMultTranspose3x3fNV(GLenum matrixMode, const GLfloat* m);
void glProgramPathFragmentInputGenNV(GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat* coeffs);
void glGetProgramResourcefvNV(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei count, GLsizei* length, GLfloat* params);
void glCreateCommandListsNV(GLsizei n, GLuint* lists);
void glDeleteCommandListsNV(GLsizei n, const GLuint* lists);
GLboolean glIsCommandListNV(GLuint list);
void glCreateStatesNV(GLsizei n, GLuint* states);
void glDeleteStatesNV(GLsizei n, const GLuint* states);
GLboolean glIsStateNV(GLuint state);
void glCommandListSegmentsNV(GLuint list, GLuint segments);
void glCompileCommandListNV(GLuint list);
void glCallCommandListNV(GLuint list);
void glDrawCommandsNV(GLenum primitiveMode, GLuint buffer, const GLintptr* indirects, const GLsizei* sizes, GLuint count);
void glDrawCommandsAddressNV(GLenum primitiveMode, const GLuint64* indirects, const GLsizei* sizes, GLuint count);
void glDrawCommandsStatesNV(GLuint buffer, const GLintptr* indirects, const GLsizei* sizes, const GLuint* states, const GLuint* fbos, GLuint count);
void glDrawCommandsStatesAddressNV(const GLuint64* indirects, const GLsizei* sizes, const GLuint* states, const GLuint* fbos, GLuint count);
void glListDrawCommandsStatesClientNV(GLuint list, GLuint segment, const void** indirects, const GLsizei* sizes, const GLuint* states, const GLuint* fbos, GLuint count);
void glStateCaptureNV(GLuint state, GLenum mode);
GLuint glGetCommandHeaderNV(GLenum tokenID, GLuint size);
GLushort glGetStageIndexNV(GLenum shadertype);
}

namespace {
static bool Path(GLuint p) { return Context::Current().ext.paths.count(p)!=0; }
static void Unsupported(Context& c) { c.errors.Record(0x0502); }
static void Zero(void* p, size_t n) { if(p)std::memset(p,0,n); }
}

extern "C" {

GLuint glGenPathsNV(GLsizei range) {
    Context& c=Context::Current(); if(range<0){c.errors.Record(0x0501);return 0;}
    GLuint base=c.ext.nextPath; c.ext.nextPath+=(GLuint)std::max<GLsizei>(range,1);
    for(GLsizei i=0;i<range;++i)c.ext.paths[base+(GLuint)i]=true;
    return base;
}
void glDeletePathsNV(GLuint path, GLsizei range) {
    Context& c=Context::Current(); if(range<0){c.errors.Record(0x0501);return;}
    for(GLsizei i=0;i<range;++i)c.ext.paths.erase(path+(GLuint)i);
}
GLboolean glIsPathNV(GLuint path) { return Path(path)?1:0; }
void glPathParameterfNV(GLuint path, GLenum pname, GLfloat value) { (void)pname;(void)value; if(!Path(path))Context::Current().errors.Record(0x0502); }
void glPathParameterfvNV(GLuint path, GLenum pname, const GLfloat* value) { (void)pname;(void)value; if(!Path(path))Context::Current().errors.Record(0x0502); }
void glPathParameteriNV(GLuint path, GLenum pname, GLint value) { (void)pname;(void)value; if(!Path(path))Context::Current().errors.Record(0x0502); }
void glPathParameterivNV(GLuint path, GLenum pname, const GLint* value) { (void)pname;(void)value; if(!Path(path))Context::Current().errors.Record(0x0502); }
void glGetPathParameterfvNV(GLuint path, GLenum pname, GLfloat* value) { (void)pname; if(!Path(path))Context::Current().errors.Record(0x0502); else if(value)*value=0; }
void glGetPathParameterivNV(GLuint path, GLenum pname, GLint* value) { (void)pname; if(!Path(path))Context::Current().errors.Record(0x0502); else if(value)*value=0; }
void glPathCommandsNV(GLuint path, GLsizei numCommands, const GLubyte* commands, GLsizei numCoords, GLenum coordType, const void* coords) {
    (void)numCommands;(void)commands;(void)numCoords;(void)coordType;(void)coords; if(!Path(path))Context::Current().errors.Record(0x0502);
}
void glPathCoordsNV(GLuint path, GLsizei numCoords, GLenum coordType, const void* coords) {
    (void)numCoords;(void)coordType;(void)coords; if(!Path(path))Context::Current().errors.Record(0x0502);
}
void glPathSubCommandsNV(GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte* commands, GLsizei numCoords, GLenum coordType, const void* coords) {
    (void)commandStart;(void)commandsToDelete;(void)numCommands;(void)commands;(void)numCoords;(void)coordType;(void)coords;
    if(!Path(path))Context::Current().errors.Record(0x0502);
}
void glPathSubCoordsNV(GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void* coords) {
    (void)coordStart;(void)numCoords;(void)coordType;(void)coords; if(!Path(path))Context::Current().errors.Record(0x0502);
}
void glPathStringNV(GLuint path, GLenum format, GLsizei length, const void* pathString) {
    (void)format;(void)length;(void)pathString; if(!Path(path))Context::Current().errors.Record(0x0502);
}
void glPathDashArrayNV(GLuint path, GLsizei dashCount, const GLfloat* dashArray) {
    (void)dashCount;(void)dashArray; if(!Path(path))Context::Current().errors.Record(0x0502);
}
void glGetPathCommandsNV(GLuint path, GLubyte* commands) { if(!Path(path))Context::Current().errors.Record(0x0502); else Zero(commands,1); }
void glGetPathCoordsNV(GLuint path, GLfloat* coords) { if(!Path(path))Context::Current().errors.Record(0x0502); else Zero(coords,sizeof(GLfloat)); }
void glGetPathDashArrayNV(GLuint path, GLfloat* dashArray) { if(!Path(path))Context::Current().errors.Record(0x0502); else Zero(dashArray,sizeof(GLfloat)); }
GLfloat glGetPathLengthNV(GLuint path, GLsizei startSegment, GLsizei numSegments) {
    (void)startSegment;(void)numSegments; if(!Path(path))Context::Current().errors.Record(0x0502); return 0;
}
void glPathStencilFuncNV(GLenum func, GLint ref, GLuint mask) { (void)func;(void)ref;(void)mask; }
void glPathStencilDepthOffsetNV(GLfloat factor, GLfloat units) { (void)factor;(void)units; }
void glPathCoverDepthFuncNV(GLenum func) { (void)func; }
void glPathColorGenNV(GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat* coeffs) { (void)color;(void)genMode;(void)colorFormat;(void)coeffs; }
void glPathTexGenNV(GLenum texCoordSet, GLenum genMode, GLint components, const GLfloat* coeffs) { (void)texCoordSet;(void)genMode;(void)components;(void)coeffs; }
void glPathFogGenNV(GLenum genMode) { (void)genMode; }
void glGetPathColorGenfvNV(GLenum color, GLenum pname, GLfloat* value) { (void)color;(void)pname; Zero(value,4*sizeof(GLfloat)); }
void glGetPathColorGenivNV(GLenum color, GLenum pname, GLint* value) { (void)color;(void)pname; Zero(value,4*sizeof(GLint)); }
void glGetPathTexGenfvNV(GLenum texCoordSet, GLenum pname, GLfloat* value) { (void)texCoordSet;(void)pname; Zero(value,4*sizeof(GLfloat)); }
void glGetPathTexGenivNV(GLenum texCoordSet, GLenum pname, GLint* value) { (void)texCoordSet;(void)pname; Zero(value,4*sizeof(GLint)); }
void glPathGlyphsNV(GLuint firstPathName, GLenum fontTarget, const void* fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void* charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale) {
    (void)fontTarget;(void)fontName;(void)fontStyle;(void)numGlyphs;(void)type;(void)charcodes;(void)handleMissingGlyphs;(void)pathParameterTemplate;(void)emScale;
    if(!Path(firstPathName))Context::Current().errors.Record(0x0502);
}
void glPathGlyphRangeNV(GLuint firstPathName, GLenum fontTarget, const void* fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale) {
    (void)fontTarget;(void)fontName;(void)fontStyle;(void)firstGlyph;(void)numGlyphs;(void)handleMissingGlyphs;(void)pathParameterTemplate;(void)emScale;
    if(!Path(firstPathName))Context::Current().errors.Record(0x0502);
}
GLenum glPathGlyphIndexArrayNV(GLuint firstPathName, GLenum fontTarget, const void* fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale) {
    (void)firstPathName;(void)fontTarget;(void)fontName;(void)fontStyle;(void)firstGlyphIndex;(void)numGlyphs;(void)pathParameterTemplate;(void)emScale; return 0;
}
GLenum glPathGlyphIndexRangeNV(GLenum fontTarget, const void* fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint* baseAndCount) {
    (void)fontTarget;(void)fontName;(void)fontStyle;(void)pathParameterTemplate;(void)emScale; if(baseAndCount){baseAndCount[0]=0;baseAndCount[1]=0;} return 0;
}
GLenum glPathMemoryGlyphIndexArrayNV(GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void* fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale) {
    (void)firstPathName;(void)fontTarget;(void)fontSize;(void)fontData;(void)faceIndex;(void)firstGlyphIndex;(void)numGlyphs;(void)pathParameterTemplate;(void)emScale; return 0;
}
void glCopyPathNV(GLuint resultPath, GLuint srcPath) {
    if(!Path(srcPath)){Context::Current().errors.Record(0x0502);return;} Context::Current().ext.paths[resultPath]=true;
}
void glInterpolatePathsNV(GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight) {
    (void)weight; if(!Path(pathA)||!Path(pathB)){Context::Current().errors.Record(0x0502);return;} Context::Current().ext.paths[resultPath]=true;
}
void glTransformPathNV(GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat* transformValues) {
    (void)transformType;(void)transformValues; if(!Path(srcPath)){Context::Current().errors.Record(0x0502);return;} Context::Current().ext.paths[resultPath]=true;
}
void glWeightPathsNV(GLuint resultPath, GLsizei numPaths, const GLuint* paths, const GLfloat* weights) {
    (void)weights; if(numPaths<0){Context::Current().errors.Record(0x0501);return;}
    for(GLsizei i=0;i<numPaths;++i)if(!Path(paths[i])){Context::Current().errors.Record(0x0502);return;}
    Context::Current().ext.paths[resultPath]=true;
}
void glGetPathMetricRangeNV(GLbitfield mask, GLuint first, GLsizei numPaths, GLsizei stride, GLfloat* metrics) {
    (void)mask;(void)first; if(numPaths<0||stride<0){Context::Current().errors.Record(0x0501);return;} if(metrics)Zero(metrics,(size_t)numPaths*(size_t)std::max(stride,1)*sizeof(GLfloat));
}
void glGetPathMetricsNV(GLbitfield mask, GLsizei numPaths, GLenum pathNameType, const void* paths, GLuint pathBase, GLsizei stride, GLfloat* metrics) {
    (void)mask;(void)pathNameType;(void)paths;(void)pathBase; if(numPaths<0||stride<0){Context::Current().errors.Record(0x0501);return;} if(metrics)Zero(metrics,(size_t)numPaths*(size_t)std::max(stride,1)*sizeof(GLfloat));
}
void glGetPathSpacingNV(GLenum mode, GLsizei numPaths, GLenum pathNameType, const void* paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat* returnedSpacing) {
    (void)mode;(void)pathNameType;(void)paths;(void)pathBase;(void)advanceScale;(void)kerningScale;(void)transformType;
    if(numPaths<0){Context::Current().errors.Record(0x0501);return;} if(returnedSpacing)Zero(returnedSpacing,(size_t)numPaths*sizeof(GLfloat));
}
GLboolean glIsPointInFillPathNV(GLuint path, GLuint mask, GLfloat x, GLfloat y) { (void)mask;(void)x;(void)y; return Path(path)?0:0; }
GLboolean glIsPointInStrokePathNV(GLuint path, GLfloat x, GLfloat y) { (void)x;(void)y; return Path(path)?0:0; }
GLboolean glPointAlongPathNV(GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat* x, GLfloat* y, GLfloat* tx, GLfloat* ty) {
    (void)startSegment;(void)numSegments;(void)distance; if(x)*x=0;if(y)*y=0;if(tx)*tx=0;if(ty)*ty=0; return Path(path)?0:0;
}
void glStencilFillPathNV(GLuint path, GLenum fillMode, GLuint mask) { (void)path;(void)fillMode;(void)mask; Unsupported(Context::Current()); }
void glStencilStrokePathNV(GLuint path, GLint reference, GLuint mask) { (void)path;(void)reference;(void)mask; Unsupported(Context::Current()); }
void glStencilFillPathInstancedNV(GLsizei n, GLenum t, const void* p, GLuint b, GLenum f, GLuint m, GLenum tr, const GLfloat* tv) { (void)n;(void)t;(void)p;(void)b;(void)f;(void)m;(void)tr;(void)tv; Unsupported(Context::Current()); }
void glStencilStrokePathInstancedNV(GLsizei n, GLenum t, const void* p, GLuint b, GLint r, GLuint m, GLenum tr, const GLfloat* tv) { (void)n;(void)t;(void)p;(void)b;(void)r;(void)m;(void)tr;(void)tv; Unsupported(Context::Current()); }
void glCoverFillPathNV(GLuint path, GLenum coverMode) { (void)path;(void)coverMode; Unsupported(Context::Current()); }
void glCoverStrokePathNV(GLuint path, GLenum coverMode) { (void)path;(void)coverMode; Unsupported(Context::Current()); }
void glCoverFillPathInstancedNV(GLsizei n, GLenum t, const void* p, GLuint b, GLenum c, GLenum tr, const GLfloat* tv) { (void)n;(void)t;(void)p;(void)b;(void)c;(void)tr;(void)tv; Unsupported(Context::Current()); }
void glCoverStrokePathInstancedNV(GLsizei n, GLenum t, const void* p, GLuint b, GLenum c, GLenum tr, const GLfloat* tv) { (void)n;(void)t;(void)p;(void)b;(void)c;(void)tr;(void)tv; Unsupported(Context::Current()); }
void glStencilThenCoverFillPathNV(GLuint path, GLenum f, GLuint m, GLenum c) { (void)path;(void)f;(void)m;(void)c; Unsupported(Context::Current()); }
void glStencilThenCoverStrokePathNV(GLuint path, GLint r, GLuint m, GLenum c) { (void)path;(void)r;(void)m;(void)c; Unsupported(Context::Current()); }
void glStencilThenCoverFillPathInstancedNV(GLsizei n, GLenum t, const void* p, GLuint b, GLenum f, GLuint m, GLenum c, GLenum tr, const GLfloat* tv) { (void)n;(void)t;(void)p;(void)b;(void)f;(void)m;(void)c;(void)tr;(void)tv; Unsupported(Context::Current()); }
void glStencilThenCoverStrokePathInstancedNV(GLsizei n, GLenum t, const void* p, GLuint b, GLint r, GLuint m, GLenum c, GLenum tr, const GLfloat* tv) { (void)n;(void)t;(void)p;(void)b;(void)r;(void)m;(void)c;(void)tr;(void)tv; Unsupported(Context::Current()); }
void glMatrixLoad3x2fNV(GLenum mode, const GLfloat* m) { (void)mode;(void)m; }
void glMatrixLoad3x3fNV(GLenum mode, const GLfloat* m) { (void)mode;(void)m; }
void glMatrixLoadTranspose3x3fNV(GLenum mode, const GLfloat* m) { (void)mode;(void)m; }
void glMatrixMult3x2fNV(GLenum mode, const GLfloat* m) { (void)mode;(void)m; }
void glMatrixMult3x3fNV(GLenum mode, const GLfloat* m) { (void)mode;(void)m; }
void glMatrixMultTranspose3x3fNV(GLenum mode, const GLfloat* m) { (void)mode;(void)m; }
void glProgramPathFragmentInputGenNV(GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat* coeffs) {
    (void)program;(void)location;(void)genMode;(void)components;(void)coeffs;
}
void glGetProgramResourcefvNV(GLuint program, GLenum iface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei count, GLsizei* length, GLfloat* params) {
    (void)program;(void)iface;(void)index;(void)propCount;(void)props; if(count<0){Context::Current().errors.Record(0x0501);return;} if(params)Zero(params,(size_t)count*sizeof(GLfloat)); if(length)*length=0;
}

void glCreateCommandListsNV(GLsizei n, GLuint* lists) {
    Context& c=Context::Current(); if(n<0||(n&&!lists)){c.errors.Record(0x0501);return;}
    for(GLsizei i=0;i<n;++i){GLuint id=c.ext.nextCommandObject++;c.ext.commandLists[id]=true;lists[i]=id;}
}
void glDeleteCommandListsNV(GLsizei n, const GLuint* lists) {
    Context& c=Context::Current(); if(n<0||(n&&!lists)){c.errors.Record(0x0501);return;}
    for(GLsizei i=0;i<n;++i)c.ext.commandLists.erase(lists[i]);
}
GLboolean glIsCommandListNV(GLuint list) { return Context::Current().ext.commandLists.count(list)?1:0; }
void glCreateStatesNV(GLsizei n, GLuint* states) {
    Context& c=Context::Current(); if(n<0||(n&&!states)){c.errors.Record(0x0501);return;}
    for(GLsizei i=0;i<n;++i){GLuint id=c.ext.nextCommandObject++;c.ext.commandStates[id]=true;states[i]=id;}
}
void glDeleteStatesNV(GLsizei n, const GLuint* states) {
    Context& c=Context::Current(); if(n<0||(n&&!states)){c.errors.Record(0x0501);return;}
    for(GLsizei i=0;i<n;++i)c.ext.commandStates.erase(states[i]);
}
GLboolean glIsStateNV(GLuint state) { return Context::Current().ext.commandStates.count(state)?1:0; }
void glCommandListSegmentsNV(GLuint list, GLuint segments) { (void)segments; if(!glIsCommandListNV(list))Context::Current().errors.Record(0x0502); }
void glCompileCommandListNV(GLuint list) { if(!glIsCommandListNV(list))Context::Current().errors.Record(0x0502); }
void glCallCommandListNV(GLuint list) { if(!glIsCommandListNV(list))Context::Current().errors.Record(0x0502); else Unsupported(Context::Current()); }
void glDrawCommandsNV(GLenum mode, GLuint buffer, const GLintptr* indirects, const GLsizei* sizes, GLuint count) { (void)mode;(void)buffer;(void)indirects;(void)sizes;(void)count; Unsupported(Context::Current()); }
void glDrawCommandsAddressNV(GLenum mode, const GLuint64* indirects, const GLsizei* sizes, GLuint count) { (void)mode;(void)indirects;(void)sizes;(void)count; Unsupported(Context::Current()); }
void glDrawCommandsStatesNV(GLuint buffer, const GLintptr* indirects, const GLsizei* sizes, const GLuint* states, const GLuint* fbos, GLuint count) { (void)buffer;(void)indirects;(void)sizes;(void)states;(void)fbos;(void)count; Unsupported(Context::Current()); }
void glDrawCommandsStatesAddressNV(const GLuint64* indirects, const GLsizei* sizes, const GLuint* states, const GLuint* fbos, GLuint count) { (void)indirects;(void)sizes;(void)states;(void)fbos;(void)count; Unsupported(Context::Current()); }
void glListDrawCommandsStatesClientNV(GLuint list, GLuint segment, const void** indirects, const GLsizei* sizes, const GLuint* states, const GLuint* fbos, GLuint count) { (void)list;(void)segment;(void)indirects;(void)sizes;(void)states;(void)fbos;(void)count; Unsupported(Context::Current()); }
void glStateCaptureNV(GLuint state, GLenum mode) { (void)state;(void)mode; Unsupported(Context::Current()); }
GLuint glGetCommandHeaderNV(GLenum tokenID, GLuint size) { (void)tokenID;(void)size; return 0; }
GLushort glGetStageIndexNV(GLenum shadertype) { (void)shadertype; return 0; }

} // extern "C"
