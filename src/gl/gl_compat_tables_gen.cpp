// AUTO-GENERATED bởi tools/gen_compat_tables.py — KHÔNG SỬA TAY.
// Nguồn: docs/khronos/gl.xml (EnableCap + GetPName groups), phân tích
// version/profile của tools/gen_version_inventory.py (reg.py PASS 1).
//
// State trong 2 bảng này KHÔNG tồn tại ở Core Profile
// (GL 4.6 Core spec App. E.2.2 "Removed Features").
//   • enable cap → glEnable/glDisable/glIsEnabled: GL_INVALID_ENUM
//   • GetPName   → glGetIntegerv/glGetFloatv:  GL_INVALID_ENUM
//
// GIÁ TRỊ CÓ ALIAS (vd 0x3000 = GL_CLIP_PLANE0 compat +
// GL_CLIP_DISTANCE0 core) bị LOẠI khỏi cả 2 bảng: giá trị đó hợp lệ
// ở Core qua tên core.
#include "tglmt/gl46_types.h"
#include "tglmt/gl_legacy_types.h"
#include <cstring>
namespace tglmt {
namespace compat_caps {
struct Entry { const char* name; unsigned int value; };
static const Entry kTable[] = {
    {"GL_POINT_SMOOTH", 0x0B10u},
    {"GL_LINE_STIPPLE", 0x0B24u},
    {"GL_POLYGON_STIPPLE", 0x0B42u},
    {"GL_LIGHTING", 0x0B50u},
    {"GL_COLOR_MATERIAL", 0x0B57u},
    {"GL_FOG", 0x0B60u},
    {"GL_NORMALIZE", 0x0BA1u},
    {"GL_ALPHA_TEST", 0x0BC0u},
    {"GL_INDEX_LOGIC_OP", 0x0BF1u},
    {"GL_TEXTURE_GEN_S", 0x0C60u},
    {"GL_TEXTURE_GEN_T", 0x0C61u},
    {"GL_TEXTURE_GEN_R", 0x0C62u},
    {"GL_TEXTURE_GEN_Q", 0x0C63u},
    {"GL_AUTO_NORMAL", 0x0D80u},
    {"GL_MAP1_COLOR_4", 0x0D90u},
    {"GL_MAP1_INDEX", 0x0D91u},
    {"GL_MAP1_NORMAL", 0x0D92u},
    {"GL_MAP1_TEXTURE_COORD_1", 0x0D93u},
    {"GL_MAP1_TEXTURE_COORD_2", 0x0D94u},
    {"GL_MAP1_TEXTURE_COORD_3", 0x0D95u},
    {"GL_MAP1_TEXTURE_COORD_4", 0x0D96u},
    {"GL_MAP1_VERTEX_3", 0x0D97u},
    {"GL_MAP1_VERTEX_4", 0x0D98u},
    {"GL_MAP2_COLOR_4", 0x0DB0u},
    {"GL_MAP2_INDEX", 0x0DB1u},
    {"GL_MAP2_NORMAL", 0x0DB2u},
    {"GL_MAP2_TEXTURE_COORD_1", 0x0DB3u},
    {"GL_MAP2_TEXTURE_COORD_2", 0x0DB4u},
    {"GL_MAP2_TEXTURE_COORD_3", 0x0DB5u},
    {"GL_MAP2_TEXTURE_COORD_4", 0x0DB6u},
    {"GL_MAP2_VERTEX_3", 0x0DB7u},
    {"GL_MAP2_VERTEX_4", 0x0DB8u},
    {"GL_LIGHT0", 0x4000u},
    {"GL_LIGHT1", 0x4001u},
    {"GL_LIGHT2", 0x4002u},
    {"GL_LIGHT3", 0x4003u},
    {"GL_LIGHT4", 0x4004u},
    {"GL_LIGHT5", 0x4005u},
    {"GL_LIGHT6", 0x4006u},
    {"GL_LIGHT7", 0x4007u},
    {"GL_NORMAL_ARRAY", 0x8075u},
    {"GL_COLOR_ARRAY", 0x8076u},
    {"GL_INDEX_ARRAY", 0x8077u},
    {"GL_TEXTURE_COORD_ARRAY", 0x8078u},
    {"GL_EDGE_FLAG_ARRAY", 0x8079u},
    {"GL_COLOR_TABLE", 0x80D0u},
    {"GL_POST_CONVOLUTION_COLOR_TABLE", 0x80D1u},
    {"GL_POST_COLOR_MATRIX_COLOR_TABLE", 0x80D2u},
};
static const int kCount = 48;
} // namespace compat_caps

namespace compat_pnames {
struct Entry { const char* name; unsigned int value; };
static const Entry kTable[] = {
    {"GL_CURRENT_COLOR", 0x0B00u},
    {"GL_CURRENT_INDEX", 0x0B01u},
    {"GL_CURRENT_NORMAL", 0x0B02u},
    {"GL_CURRENT_TEXTURE_COORDS", 0x0B03u},
    {"GL_CURRENT_RASTER_COLOR", 0x0B04u},
    {"GL_CURRENT_RASTER_INDEX", 0x0B05u},
    {"GL_CURRENT_RASTER_TEXTURE_COORDS", 0x0B06u},
    {"GL_CURRENT_RASTER_POSITION", 0x0B07u},
    {"GL_CURRENT_RASTER_POSITION_VALID", 0x0B08u},
    {"GL_CURRENT_RASTER_DISTANCE", 0x0B09u},
    {"GL_POINT_SMOOTH", 0x0B10u},
    {"GL_LINE_STIPPLE", 0x0B24u},
    {"GL_LINE_STIPPLE_PATTERN", 0x0B25u},
    {"GL_LINE_STIPPLE_REPEAT", 0x0B26u},
    {"GL_LIST_MODE", 0x0B30u},
    {"GL_MAX_LIST_NESTING", 0x0B31u},
    {"GL_LIST_BASE", 0x0B32u},
    {"GL_LIST_INDEX", 0x0B33u},
    {"GL_POLYGON_STIPPLE", 0x0B42u},
    {"GL_EDGE_FLAG", 0x0B43u},
    {"GL_LIGHTING", 0x0B50u},
    {"GL_LIGHT_MODEL_LOCAL_VIEWER", 0x0B51u},
    {"GL_LIGHT_MODEL_TWO_SIDE", 0x0B52u},
    {"GL_LIGHT_MODEL_AMBIENT", 0x0B53u},
    {"GL_SHADE_MODEL", 0x0B54u},
    {"GL_COLOR_MATERIAL_FACE", 0x0B55u},
    {"GL_COLOR_MATERIAL_PARAMETER", 0x0B56u},
    {"GL_COLOR_MATERIAL", 0x0B57u},
    {"GL_FOG", 0x0B60u},
    {"GL_FOG_INDEX", 0x0B61u},
    {"GL_FOG_DENSITY", 0x0B62u},
    {"GL_FOG_START", 0x0B63u},
    {"GL_FOG_END", 0x0B64u},
    {"GL_FOG_MODE", 0x0B65u},
    {"GL_FOG_COLOR", 0x0B66u},
    {"GL_ACCUM_CLEAR_VALUE", 0x0B80u},
    {"GL_MATRIX_MODE", 0x0BA0u},
    {"GL_NORMALIZE", 0x0BA1u},
    {"GL_MODELVIEW_STACK_DEPTH", 0x0BA3u},
    {"GL_PROJECTION_STACK_DEPTH", 0x0BA4u},
    {"GL_TEXTURE_STACK_DEPTH", 0x0BA5u},
    {"GL_MODELVIEW_MATRIX", 0x0BA6u},
    {"GL_PROJECTION_MATRIX", 0x0BA7u},
    {"GL_TEXTURE_MATRIX", 0x0BA8u},
    {"GL_ATTRIB_STACK_DEPTH", 0x0BB0u},
    {"GL_CLIENT_ATTRIB_STACK_DEPTH", 0x0BB1u},
    {"GL_ALPHA_TEST", 0x0BC0u},
    {"GL_ALPHA_TEST_FUNC", 0x0BC1u},
    {"GL_ALPHA_TEST_REF", 0x0BC2u},
    {"GL_INDEX_LOGIC_OP", 0x0BF1u},
    {"GL_AUX_BUFFERS", 0x0C00u},
    {"GL_INDEX_CLEAR_VALUE", 0x0C20u},
    {"GL_INDEX_WRITEMASK", 0x0C21u},
    {"GL_INDEX_MODE", 0x0C30u},
    {"GL_RGBA_MODE", 0x0C31u},
    {"GL_RENDER_MODE", 0x0C40u},
    {"GL_PERSPECTIVE_CORRECTION_HINT", 0x0C50u},
    {"GL_POINT_SMOOTH_HINT", 0x0C51u},
    {"GL_FOG_HINT", 0x0C54u},
    {"GL_TEXTURE_GEN_S", 0x0C60u},
    {"GL_TEXTURE_GEN_T", 0x0C61u},
    {"GL_TEXTURE_GEN_R", 0x0C62u},
    {"GL_TEXTURE_GEN_Q", 0x0C63u},
    {"GL_PIXEL_MAP_I_TO_I_SIZE", 0x0CB0u},
    {"GL_PIXEL_MAP_S_TO_S_SIZE", 0x0CB1u},
    {"GL_PIXEL_MAP_I_TO_R_SIZE", 0x0CB2u},
    {"GL_PIXEL_MAP_I_TO_G_SIZE", 0x0CB3u},
    {"GL_PIXEL_MAP_I_TO_B_SIZE", 0x0CB4u},
    {"GL_PIXEL_MAP_I_TO_A_SIZE", 0x0CB5u},
    {"GL_PIXEL_MAP_R_TO_R_SIZE", 0x0CB6u},
    {"GL_PIXEL_MAP_G_TO_G_SIZE", 0x0CB7u},
    {"GL_PIXEL_MAP_B_TO_B_SIZE", 0x0CB8u},
    {"GL_PIXEL_MAP_A_TO_A_SIZE", 0x0CB9u},
    {"GL_MAP_COLOR", 0x0D10u},
    {"GL_MAP_STENCIL", 0x0D11u},
    {"GL_INDEX_SHIFT", 0x0D12u},
    {"GL_INDEX_OFFSET", 0x0D13u},
    {"GL_RED_SCALE", 0x0D14u},
    {"GL_RED_BIAS", 0x0D15u},
    {"GL_ZOOM_X", 0x0D16u},
    {"GL_ZOOM_Y", 0x0D17u},
    {"GL_GREEN_SCALE", 0x0D18u},
    {"GL_GREEN_BIAS", 0x0D19u},
    {"GL_BLUE_SCALE", 0x0D1Au},
    {"GL_BLUE_BIAS", 0x0D1Bu},
    {"GL_ALPHA_SCALE", 0x0D1Cu},
    {"GL_ALPHA_BIAS", 0x0D1Du},
    {"GL_DEPTH_SCALE", 0x0D1Eu},
    {"GL_DEPTH_BIAS", 0x0D1Fu},
    {"GL_MAX_EVAL_ORDER", 0x0D30u},
    {"GL_MAX_LIGHTS", 0x0D31u},
    {"GL_MAX_PIXEL_MAP_TABLE", 0x0D34u},
    {"GL_MAX_ATTRIB_STACK_DEPTH", 0x0D35u},
    {"GL_MAX_MODELVIEW_STACK_DEPTH", 0x0D36u},
    {"GL_MAX_NAME_STACK_DEPTH", 0x0D37u},
    {"GL_MAX_PROJECTION_STACK_DEPTH", 0x0D38u},
    {"GL_MAX_TEXTURE_STACK_DEPTH", 0x0D39u},
    {"GL_MAX_CLIENT_ATTRIB_STACK_DEPTH", 0x0D3Bu},
    {"GL_INDEX_BITS", 0x0D51u},
    {"GL_RED_BITS", 0x0D52u},
    {"GL_GREEN_BITS", 0x0D53u},
    {"GL_BLUE_BITS", 0x0D54u},
    {"GL_ALPHA_BITS", 0x0D55u},
    {"GL_DEPTH_BITS", 0x0D56u},
    {"GL_STENCIL_BITS", 0x0D57u},
    {"GL_ACCUM_RED_BITS", 0x0D58u},
    {"GL_ACCUM_GREEN_BITS", 0x0D59u},
    {"GL_ACCUM_BLUE_BITS", 0x0D5Au},
    {"GL_ACCUM_ALPHA_BITS", 0x0D5Bu},
    {"GL_NAME_STACK_DEPTH", 0x0D70u},
    {"GL_AUTO_NORMAL", 0x0D80u},
    {"GL_MAP1_COLOR_4", 0x0D90u},
    {"GL_MAP1_INDEX", 0x0D91u},
    {"GL_MAP1_NORMAL", 0x0D92u},
    {"GL_MAP1_TEXTURE_COORD_1", 0x0D93u},
    {"GL_MAP1_TEXTURE_COORD_2", 0x0D94u},
    {"GL_MAP1_TEXTURE_COORD_3", 0x0D95u},
    {"GL_MAP1_TEXTURE_COORD_4", 0x0D96u},
    {"GL_MAP1_VERTEX_3", 0x0D97u},
    {"GL_MAP1_VERTEX_4", 0x0D98u},
    {"GL_MAP2_COLOR_4", 0x0DB0u},
    {"GL_MAP2_INDEX", 0x0DB1u},
    {"GL_MAP2_NORMAL", 0x0DB2u},
    {"GL_MAP2_TEXTURE_COORD_1", 0x0DB3u},
    {"GL_MAP2_TEXTURE_COORD_2", 0x0DB4u},
    {"GL_MAP2_TEXTURE_COORD_3", 0x0DB5u},
    {"GL_MAP2_TEXTURE_COORD_4", 0x0DB6u},
    {"GL_MAP2_VERTEX_3", 0x0DB7u},
    {"GL_MAP2_VERTEX_4", 0x0DB8u},
    {"GL_MAP1_GRID_DOMAIN", 0x0DD0u},
    {"GL_MAP1_GRID_SEGMENTS", 0x0DD1u},
    {"GL_MAP2_GRID_DOMAIN", 0x0DD2u},
    {"GL_MAP2_GRID_SEGMENTS", 0x0DD3u},
    {"GL_FEEDBACK_BUFFER_SIZE", 0x0DF1u},
    {"GL_FEEDBACK_BUFFER_TYPE", 0x0DF2u},
    {"GL_SELECTION_BUFFER_SIZE", 0x0DF4u},
    {"GL_AMBIENT", 0x1200u},
    {"GL_DIFFUSE", 0x1201u},
    {"GL_SPECULAR", 0x1202u},
    {"GL_POSITION", 0x1203u},
    {"GL_SPOT_DIRECTION", 0x1204u},
    {"GL_SPOT_EXPONENT", 0x1205u},
    {"GL_SPOT_CUTOFF", 0x1206u},
    {"GL_CONSTANT_ATTENUATION", 0x1207u},
    {"GL_LINEAR_ATTENUATION", 0x1208u},
    {"GL_QUADRATIC_ATTENUATION", 0x1209u},
    {"GL_EMISSION", 0x1600u},
    {"GL_SHININESS", 0x1601u},
    {"GL_AMBIENT_AND_DIFFUSE", 0x1602u},
    {"GL_COLOR_INDEXES", 0x1603u},
    {"GL_CLIP_PLANE0", 0x3000u},
    {"GL_CLIP_PLANE1", 0x3001u},
    {"GL_CLIP_PLANE2", 0x3002u},
    {"GL_CLIP_PLANE3", 0x3003u},
    {"GL_CLIP_PLANE4", 0x3004u},
    {"GL_CLIP_PLANE5", 0x3005u},
    {"GL_LIGHT0", 0x4000u},
    {"GL_LIGHT1", 0x4001u},
    {"GL_LIGHT2", 0x4002u},
    {"GL_LIGHT3", 0x4003u},
    {"GL_LIGHT4", 0x4004u},
    {"GL_LIGHT5", 0x4005u},
    {"GL_LIGHT6", 0x4006u},
    {"GL_LIGHT7", 0x4007u},
    {"GL_NORMAL_ARRAY", 0x8075u},
    {"GL_COLOR_ARRAY", 0x8076u},
    {"GL_INDEX_ARRAY", 0x8077u},
    {"GL_TEXTURE_COORD_ARRAY", 0x8078u},
    {"GL_EDGE_FLAG_ARRAY", 0x8079u},
    {"GL_VERTEX_ARRAY_SIZE", 0x807Au},
    {"GL_VERTEX_ARRAY_TYPE", 0x807Bu},
    {"GL_VERTEX_ARRAY_STRIDE", 0x807Cu},
    {"GL_NORMAL_ARRAY_TYPE", 0x807Eu},
    {"GL_NORMAL_ARRAY_STRIDE", 0x807Fu},
    {"GL_COLOR_ARRAY_SIZE", 0x8081u},
    {"GL_COLOR_ARRAY_TYPE", 0x8082u},
    {"GL_COLOR_ARRAY_STRIDE", 0x8083u},
    {"GL_INDEX_ARRAY_TYPE", 0x8085u},
    {"GL_INDEX_ARRAY_STRIDE", 0x8086u},
    {"GL_TEXTURE_COORD_ARRAY_SIZE", 0x8088u},
    {"GL_TEXTURE_COORD_ARRAY_TYPE", 0x8089u},
    {"GL_TEXTURE_COORD_ARRAY_STRIDE", 0x808Au},
    {"GL_EDGE_FLAG_ARRAY_STRIDE", 0x808Cu},
    {"GL_POINT_SIZE_MIN", 0x8126u},
    {"GL_POINT_SIZE_MAX", 0x8127u},
    {"GL_POINT_DISTANCE_ATTENUATION", 0x8129u},
    {"GL_LIGHT_MODEL_COLOR_CONTROL", 0x81F8u},
    {"GL_FOG_COORD_SRC", 0x8450u},
    {"GL_ALIASED_POINT_SIZE_RANGE", 0x846Du},
};
static const int kCount = 189;
} // namespace compat_pnames

// true nếu `cap` là enable cap chỉ tồn tại ở Compatibility Profile.
bool IsCompatOnlyEnableCap(unsigned int cap) {
    for (int i = 0; i < compat_caps::kCount; ++i)
        if (compat_caps::kTable[i].value == cap) return true;
    return false;
}

const char* CompatOnlyEnableCapName(unsigned int cap) {
    for (int i = 0; i < compat_caps::kCount; ++i)
        if (compat_caps::kTable[i].value == cap) return compat_caps::kTable[i].name;
    return nullptr;
}

int CompatOnlyEnableCapCount() { return compat_caps::kCount; }

// true nếu `pname` chỉ tồn tại ở Compatibility Profile.
bool IsCompatOnlyGetPName(unsigned int pname) {
    for (int i = 0; i < compat_pnames::kCount; ++i)
        if (compat_pnames::kTable[i].value == pname) return true;
    return false;
}

const char* CompatOnlyGetPNameName(unsigned int pname) {
    for (int i = 0; i < compat_pnames::kCount; ++i)
        if (compat_pnames::kTable[i].value == pname) return compat_pnames::kTable[i].name;
    return nullptr;
}

int CompatOnlyGetPNameCount() { return compat_pnames::kCount; }
} // namespace tglmt
