#!/usr/bin/env python3
"""Sinh bảng "state chỉ có ở Compatibility Profile" cho TGLMT.

Sinh HAI bảng:
  (1) enable cap compat-only  → glEnable/glDisable/glIsEnabled phải từ chối ở Core.
  (2) GetPName compat-only    → glGetIntegerv/glGetFloatv phải sinh INVALID_ENUM ở Core.

Cả hai dùng CHUNG logic phát hiện ALIAS giá trị enum.

VÌ SAO CẦN
----------
OpenGL 4.6 Core Profile §17.1 (Enable/Disable) liệt kê tập cap hợp lệ của
CORE. Mọi cap fixed-function (GL_LIGHTING, GL_FOG, GL_ALPHA_TEST, GL_LIGHT0..7,
GL_COLOR_MATERIAL, GL_TEXTURE_GEN_*, ...) đã bị GỠ khỏi Core Profile từ 3.2
(glspec46.core.pdf App. E.2.2 "Removed Features").

Hiện TGLMT `glEnable(GL_LIGHTING)` ở Core Profile **được chấp nhận và im lặng**
⇒ state nói dối app. Spec yêu cầu GL_INVALID_ENUM cho cap không hợp lệ
(GL 4.6 Core §17.1: "An INVALID_ENUM error is generated if cap is not one of
the listed values").

NGUỒN CHÂN LÝ
-------------
`docs/khronos/gl.xml` (Khronos OpenGL-Registry). Enum có group="EnableCap".
Dùng chung bộ phân tích version/profile với tools/gen_version_inventory.py
⇒ cap compat-only = có trong `compatibility` nhưng KHÔNG có trong `core` ở 4.6.

Đầu ra: `src/gl/gl_compat_tables_gen.cpp` — bảng C++ (tên enum + giá trị),
kèm hàm tra cứu theo tên để log/kiểm thử.

Usage:
  python3 tools/gen_compat_tables.py           # ghi file
  python3 tools/gen_compat_tables.py --check   # chỉ kiểm tra, không ghi
"""
import pathlib
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gen_version_inventory as gvi  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[1]
GLXML = ROOT / "docs/khronos/gl.xml"
OUT = ROOT / "src/gl/gl_compat_tables_gen.cpp"


def build_group(group_name):
    """Trả về sorted list[(name, value)] của enum group chỉ có ở Compatibility.

    CẢNH BÁO ALIAS: một số giá trị enum được dùng cho CẢ cap core lẫn cap
    compat-only. Ví dụ registry dùng CÙNG giá trị 0x3000 cho:
        GL_CLIP_PLANE0    (compat-only, fixed-function clip plane)
        GL_CLIP_DISTANCE0 (CORE từ GL 3.0, clip distance của shader)
    Nếu ta reject theo GIÁ TRỊ thì glEnable(GL_CLIP_DISTANCE0) ở Core Profile
    sẽ bị từ chối oan ⇒ hỏng app dùng shader thật. Vì vậy giá trị nào có
    BẤT KỲ cap core cùng giá trị thì KHÔNG được đưa vào bảng reject.
    """
    inv = gvi.build_inventory()
    core = set(inv["versions"]["4.6"]["enums"]["core"])
    compat = set(inv["versions"]["4.6"]["enums"]["compatibility"])

    tree = ET.parse(GLXML)
    root = tree.getroot()

    # Gom tất cả EnableCap theo giá trị, để phát hiện alias.
    by_value = {}
    for e in root.findall("./enums/enum"):
        groups = (e.get("group") or "").split(",")
        if group_name not in groups:
            continue
        name = e.get("name")
        raw = e.get("value")
        if not name or raw is None:
            continue
        try:
            val = int(raw, 0)
        except ValueError:
            continue
        by_value.setdefault(val, []).append(name)

    caps = []
    skipped_alias = []
    for val, names in by_value.items():
        compat_only = [n for n in names if n in compat and n not in core]
        core_names = [n for n in names if n in core]
        if not compat_only:
            continue
        if core_names:
            # Alias: giá trị hợp lệ ở Core qua tên core → không reject.
            skipped_alias.append((val, names))
            continue
        caps.append((compat_only[0], val))
    caps.sort(key=lambda kv: kv[1])
    return caps, skipped_alias


def build_caps():
    return build_group("EnableCap")


#: Nhóm enum BỔ SUNG ngoài "GetPName" mà glGet vẫn chấp nhận.
#:
#: Registry phân loại pname theo nhiều nhóm; "GetPName" chỉ là nhóm CHÍNH.
#: Ví dụ GL_FOG_COORD_SRC (0x8450) thuộc nhóm "FogPName", và
#: GL_FOG_COORDINATE_SOURCE (0x8450, cùng giá trị) không có nhóm nào —
#: nhưng CẢ HAI đều là pname hợp lệ của glGet trong Compatibility Profile
#: (spec 4.2C bảng state 6.16 PDF p.519: "FOG COORD SRC  Z2  GetIntegerv").
#:
#: Chỉ liệt kê nhóm mà pname của nhóm ĐƯỢC glGet đọc (xem bảng state 6.x của
#: spec 4.2C): FogPName, LightModelParameter, LightParameter, MaterialParameter,
#: ColorMaterialParameter, ListParameterName, ClipPlaneName, PixelTransferParameter,
#: PixelStoreParameter, TexEnvParameter (chỉ phần GL_COMBINE_*),
#: TextureGenParameter, RenderingMode, PolygonMode, HintMode.
#:
#: KHÔNG liệt kê TextureEnvParameter/TextureGenParameter một cách chung vì
#: GL_TEXTURE_ENV_MODE / GL_TEXTURE_GEN_MODE được truy vấn bằng
#: glGetTexEnvfv / glGetTexGenfv, KHÔNG phải glGet (xem hàm build_group và
#: self-check bên dưới).
SUPPLEMENTARY_PNAME_GROUPS = [
    "FogPName",
    "LightModelParameter",
    "LightParameter",
    "MaterialParameter",
    "ColorMaterialParameter",
    "ListParameterName",
    "PixelTransferParameter",
    "PixelStoreParameter",
]


def build_pnames():
    """Pname compat-only mà glGet chấp nhận.

    Gồm nhóm "GetPName" + các nhóm bổ sung xem SUPPLEMENTARY_PNAME_GROUPS.
    """
    inv = gvi.build_inventory()
    core = set(inv["versions"]["4.6"]["enums"]["core"])
    compat = set(inv["versions"]["4.6"]["enums"]["compatibility"])

    tree = ET.parse(GLXML)
    root = tree.getroot()
    by_value = {}
    for e in root.findall("./enums/enum"):
        groups = (e.get("group") or "").split(",")
        if "GetPName" not in groups and not any(g in groups for g in SUPPLEMENTARY_PNAME_GROUPS):
            continue
        name = e.get("name")
        raw = e.get("value")
        if not name or raw is None:
            continue
        try:
            val = int(raw, 0)
        except ValueError:
            continue
        by_value.setdefault(val, []).append(name)

    caps = []
    skipped_alias = []
    for val, names in by_value.items():
        compat_only = [n for n in names if n in compat and n not in core]
        core_names = [n for n in names if n in core]
        if not compat_only:
            continue
        if core_names:
            skipped_alias.append((val, names))
            continue
        caps.append((compat_only[0], val))
    caps.sort(key=lambda kv: kv[1])
    return caps, skipped_alias


def main():
    check = "--check" in sys.argv
    caps, cap_skipped = build_caps()
    if len(caps) < 40:
        raise SystemExit(f"[compat] FATAL: chỉ tìm {len(caps)} compat-only cap, kỳ vọng >= 40")
    # Spot-check các cap cốt lõi của fixed-function.
    names = {n for n, _ in caps}
    for must in ("GL_LIGHTING", "GL_FOG", "GL_ALPHA_TEST", "GL_COLOR_MATERIAL",
                 "GL_LIGHT0", "GL_LIGHT7", "GL_NORMALIZE", "GL_POLYGON_STIPPLE",
                 "GL_TEXTURE_GEN_S", "GL_TEXTURE_GEN_Q"):
        if must not in names:
            raise SystemExit(f"[compat] FATAL: thiếu cap bắt buộc {must}")
    # ALIAS: GL_CLIP_PLANE0 (compat) và GL_CLIP_DISTANCE0 (core) cùng 0x3000 ⇒
    # giá trị này KHÔNG được reject ở Core Profile.
    vals = {v for _, v in caps}
    if 0x3000 in vals:
        raise SystemExit("[compat] FATAL: 0x3000 (GL_CLIP_PLANE0/GL_CLIP_DISTANCE0 alias) "
                         "đã vào bảng reject — sẽ hỏng glEnable(GL_CLIP_DISTANCE0) ở Core")

    pnames, pname_skipped = build_pnames()
    if len(pnames) < 100:
        raise SystemExit(f"[compat] FATAL: chỉ tìm {len(pnames)} compat-only pname, "
                         "kỳ vọng >= 100")
    pnames_set = {n for n, _ in pnames}
    # Chỉ nhóm "GetPName" của registry mới là tập pname MÀ glGet chấp nhận.
    # GL_TEXTURE_ENV_MODE / GL_TEXTURE_GEN_MODE thuộc nhóm TextureEnvParameter /
    # TextureGenParameter ⇒ chúng được truy vấn bằng glGetTexEnvfv /
    # glGetTexGenfv, KHÔNG phải glGet ⇒ không được vào bảng reject của glGet.
    for must in ("GL_LIGHTING", "GL_FOG_MODE", "GL_FOG_COLOR", "GL_SHADE_MODEL",
                 "GL_COLOR_MATERIAL_FACE", "GL_COLOR_MATERIAL_PARAMETER",
                 "GL_LIGHT_MODEL_AMBIENT", "GL_LIGHT_MODEL_LOCAL_VIEWER",
                 "GL_LIGHT_MODEL_TWO_SIDE", "GL_MODELVIEW_MATRIX",
                 "GL_PROJECTION_MATRIX", "GL_TEXTURE_MATRIX",
                 "GL_CURRENT_COLOR", "GL_CURRENT_NORMAL",
                 "GL_TEXTURE_GEN_S", "GL_ALPHA_TEST_FUNC",
                 "GL_MATRIX_MODE", "GL_MAX_LIGHTS",
                 # Nhóm bổ sung ngoài "GetPName":
                 "GL_FOG_COORD_SRC", "GL_LIGHT_MODEL_AMBIENT",
                 "GL_LIGHT_MODEL_COLOR_CONTROL", "GL_AMBIENT",
                 "GL_LIST_MODE"):
        if must not in pnames_set:
            raise SystemExit(f"[compat] FATAL: thiếu pname bắt buộc {must}")
    # KHÔNG được reject pname mà glGet ở Core vẫn hợp lệ.
    for never in ("GL_TEXTURE_ENV_MODE", "GL_TEXTURE_GEN_MODE", "GL_TEXTURE_ENV_COLOR"):
        if never in pnames_set:
            raise SystemExit(f"[compat] FATAL: {never} không phải GetPName "
                             "(truy vấn bằng glGetTexEnvfv/glGetTexGenfv) "
                             "nhưng đã vào bảng reject của glGet")

    print(f"[compat] compat-only EnableCap: {len(caps)}")
    print(f"[compat] compat-only GetPName: {len(pnames)}")
    for label, skipped in (("cap", cap_skipped), ("pname", pname_skipped)):
        if skipped:
            print(f"[compat] skipped {len(skipped)} alias {label} value(s) "
                  "(core+compat cùng giá trị):")
            for val, nm in sorted(skipped):
                print(f"[compat]   0x{val:04X}: {', '.join(nm)}")

    if check:
        return

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with open(OUT, "w") as f:
        f.write("// AUTO-GENERATED bởi tools/gen_compat_tables.py — KHÔNG SỬA TAY.\n")
        f.write("// Nguồn: docs/khronos/gl.xml (EnableCap + GetPName groups), phân tích\n")
        f.write("// version/profile của tools/gen_version_inventory.py (reg.py PASS 1).\n")
        f.write("//\n")
        f.write("// State trong 2 bảng này KHÔNG tồn tại ở Core Profile\n")
        f.write("// (GL 4.6 Core spec App. E.2.2 \"Removed Features\").\n")
        f.write("//   • enable cap → glEnable/glDisable/glIsEnabled: GL_INVALID_ENUM\n")
        f.write("//   • GetPName   → glGetIntegerv/glGetFloatv:  GL_INVALID_ENUM\n")
        f.write("//\n")
        f.write("// GIÁ TRỊ CÓ ALIAS (vd 0x3000 = GL_CLIP_PLANE0 compat +\n")
        f.write("// GL_CLIP_DISTANCE0 core) bị LOẠI khỏi cả 2 bảng: giá trị đó hợp lệ\n")
        f.write("// ở Core qua tên core.\n")
        f.write('#include "tglmt/gl46_types.h"\n')
        f.write('#include "tglmt/gl_legacy_types.h"\n')
        f.write('#include <cstring>\n')
        f.write("namespace tglmt {\n")
        for ns, rows, label in (("compat_caps", caps, "enable cap"),
                                ("compat_pnames", pnames, "GetPName")):
            f.write(f"namespace {ns} {{\n")
            f.write("struct Entry { const char* name; unsigned int value; };\n")
            f.write("static const Entry kTable[] = {\n")
            for name, val in rows:
                f.write(f'    {{"{name}", 0x{val:04X}u}},\n')
            f.write("};\n")
            f.write(f"static const int kCount = {len(rows)};\n")
            f.write(f"}} // namespace {ns}\n\n")
        f.write("// true nếu `cap` là enable cap chỉ tồn tại ở Compatibility Profile.\n")
        f.write("bool IsCompatOnlyEnableCap(unsigned int cap) {\n")
        f.write("    for (int i = 0; i < compat_caps::kCount; ++i)\n")
        f.write("        if (compat_caps::kTable[i].value == cap) return true;\n")
        f.write("    return false;\n")
        f.write("}\n\n")
        f.write("const char* CompatOnlyEnableCapName(unsigned int cap) {\n")
        f.write("    for (int i = 0; i < compat_caps::kCount; ++i)\n")
        f.write("        if (compat_caps::kTable[i].value == cap) "
                "return compat_caps::kTable[i].name;\n")
        f.write("    return nullptr;\n")
        f.write("}\n\n")
        f.write("int CompatOnlyEnableCapCount() { return compat_caps::kCount; }\n\n")
        f.write("// true nếu `pname` chỉ tồn tại ở Compatibility Profile.\n")
        f.write("bool IsCompatOnlyGetPName(unsigned int pname) {\n")
        f.write("    for (int i = 0; i < compat_pnames::kCount; ++i)\n")
        f.write("        if (compat_pnames::kTable[i].value == pname) return true;\n")
        f.write("    return false;\n")
        f.write("}\n\n")
        f.write("const char* CompatOnlyGetPNameName(unsigned int pname) {\n")
        f.write("    for (int i = 0; i < compat_pnames::kCount; ++i)\n")
        f.write("        if (compat_pnames::kTable[i].value == pname) "
                "return compat_pnames::kTable[i].name;\n")
        f.write("    return nullptr;\n")
        f.write("}\n\n")
        f.write("int CompatOnlyGetPNameCount() { return compat_pnames::kCount; }\n")
        f.write("} // namespace tglmt\n")
    print(f"[compat] wrote {OUT} ({len(caps)} caps, {len(pnames)} pnames)")


if __name__ == "__main__":
    main()
