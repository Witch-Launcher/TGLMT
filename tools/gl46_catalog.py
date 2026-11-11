#!/usr/bin/env python3
"""Single source of truth for the OpenGL 4.6 Core Profile command catalog.

Derived from the official Khronos registry (``docs/khronos/gl.xml``, vendored
byte-identical to https://github.com/KhronosGroup/OpenGL-Registry ``main``).

Why this module exists
----------------------
Before this module the project carried three *independent* copies of the
derivation (``tools/truth_manifest.py``, ``tools/gen_gl_headers.py``,
``tools/gen_stubs.py``).  All three shared the same two defects:

  B1  ``<require profile="compatibility">`` blocks were not honoured, so
      compatibility-profile-only entry points were counted as *core*.
  B2  ``<remove>`` blocks were applied *after* all ``<require>`` blocks instead
      of in version order, so a command removed for core in one version and
      re-required in a later version was dropped.

Consequences (measured, see ``docs/OPENGL_46_API_INVENTORY.md``):

  * 42 compatibility-only entry points were counted as core
    (the ``glVertexP*``/``glTexCoordP*``/``glMultiTexCoordP*``/``glNormalP*``/
    ``glColorP*``/``glSecondaryColorP*`` family from
    ``gl.xml`` ``GL_VERSION_3_3`` and the 12 ``glGetn*`` functions from
    ``GL_VERSION_4_5``).
  * ``glGetPointerv`` -- removed for core in GL 3.2 but *re-required* by
    ``GL_VERSION_4_3`` -- was wrongly excluded.

The project therefore advertised **698** "OpenGL 4.6 Core" APIs when the
canonical core surface is **657**.  The 698 figure happens to equal
"core union compatibility-only" and is *not* the core-profile count.

The derivation below follows Khronos' own ``xml/reg.py`` PASS 1 semantics:
a single version-ordered pass over ``GL_VERSION_1_0`` .. ``GL_VERSION_4_6``
where, for each feature in ascending version order,

  * ``<require>`` without ``profile``            -> member of every profile
  * ``<require profile="compatibility">``        -> compatibility profile only
  * ``<remove>`` without ``profile``              -> removed everywhere
  * ``<remove profile="core">``                  -> removed from core only

with "last write wins" in version order, so a later ``<require>`` restores a
command an earlier version removed.

Cross-checked against the official Khronos-generated ``glcorearb.h``: the
core set produced here is *set-equal* to the 657 prototypes in that header.
"""

import pathlib
import re
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parents[1]
GLXML = ROOT / "docs/khronos/gl.xml"

#: Canonical number of OpenGL 4.6 Core Profile entry points (Khronos registry).
CORE_FUNCTION_COUNT = 657

#: Number of entry points that exist only in the OpenGL 4.6 *Compatibility*
#: profile.  The project implements and exports them as well (they are pulled
#: in by GL_ARB_vertex_type_2_10_10_10_rev / GL_ARB_robustness), but they are
#: NOT part of the OpenGL 4.6 Core Profile and must never be reported as such.
COMPAT_ONLY_FUNCTION_COUNT = 42


def _version_key(feature_name):
    m = re.match(r"^GL_VERSION_(\d+)_(\d+)$", feature_name or "")
    if not m:
        return None
    return (int(m.group(1)), int(m.group(2)))


def _desktop_features(root):
    """All desktop GL_VERSION_x_y features, ascending by version."""
    out = []
    for feat in root.findall("feature"):
        if feat.get("api") != "gl":
            continue
        name = feat.get("name") or ""
        if not name.startswith("GL_VERSION_") or "ES" in name:
            continue
        vk = _version_key(name)
        if vk is None or vk > (4, 6):
            continue
        out.append((vk, name, feat))
    out.sort(key=lambda t: t[0])
    return out


def parse_core_commands(glxml=None):
    """Return ``{command_name: introduced_version}`` for OpenGL 4.6 **Core**.

    This is the canonical Khronos-registry derivation (see module docstring).
    """
    path = pathlib.Path(glxml) if glxml else GLXML
    root = ET.parse(str(path)).getroot()
    core = {}
    for _vk, name, feat in _desktop_features(root):
        ver = "%d.%d" % _vk
        # 1) removes first *within this feature*, then requires, so that a
        #    feature which removes-and-re-adds the same command keeps it.
        #    (Khronos reg.py applies <remove> blocks before <require> blocks of
        #    the same feature.)
        for rm in feat.findall("remove"):
            if rm.get("profile") == "compatibility":
                continue  # removed from compatibility only -> keep in core
            for cmd in rm.findall("command"):
                fn = cmd.get("name")
                if fn:
                    core.pop(fn, None)
        for req in feat.findall("require"):
            if req.get("profile") == "compatibility":
                continue  # compatibility-profile only -> NOT core
            for cmd in req.findall("command"):
                fn = cmd.get("name")
                if fn and fn not in core:
                    core[fn] = ver
    return core


def parse_compat_only_commands(glxml=None):
    """Return ``{command_name: introduced_version}`` for compat-profile-only.

    These are entry points the registry marks ``profile="compatibility"``.
    They are implemented by this project (the extension aliases need them) but
    they are **not** OpenGL 4.6 Core.
    """
    path = pathlib.Path(glxml) if glxml else GLXML
    root = ET.parse(str(path)).getroot()
    compat = {}
    for _vk, name, feat in _desktop_features(root):
        ver = "%d.%d" % _vk
        for req in feat.findall("require"):
            if req.get("profile") != "compatibility":
                continue
            for cmd in req.findall("command"):
                fn = cmd.get("name")
                if fn and fn not in compat:
                    compat[fn] = ver
    # A command that is also required without a profile restriction *is* core;
    # it is not compat-only.
    core = parse_core_commands(path)
    for fn in list(compat):
        if fn in core:
            del compat[fn]
    return compat


def parse_all_commands(glxml=None):
    """Every entry point the project declares: core + compatibility-only."""
    path = pathlib.Path(glxml) if glxml else GLXML
    core = parse_core_commands(path)
    compat = parse_compat_only_commands(path)
    merged = dict(core)
    for fn, ver in compat.items():
        merged.setdefault(fn, ver)
    return merged


def parse_command_prototypes(glxml=None):
    """Return ``{command_name: (return_type, [param_text, ...])}`` from gl.xml."""
    path = pathlib.Path(glxml) if glxml else GLXML
    root = ET.parse(str(path)).getroot()
    proto_of = {}
    for cmds in root.findall("commands"):
        for cmd in cmds.findall("command"):
            proto = cmd.find("proto")
            if proto is None:
                continue
            pname_el = proto.find("name")
            if pname_el is None or not pname_el.text:
                continue
            pname = pname_el.text.strip()
            ret = "".join(proto.itertext()).replace(pname, "").strip()
            params = []
            for p in cmd.findall("param"):
                pt = "".join(p.itertext()).strip()
                if pt:
                    params.append(pt)
            proto_of[pname] = (ret, params)
    return proto_of


if __name__ == "__main__":
    core = parse_core_commands()
    compat = parse_compat_only_commands()
    print(f"OpenGL 4.6 Core Profile commands: {len(core)} (expected {CORE_FUNCTION_COUNT})")
    print(f"Compatibility-profile-only commands: {len(compat)} (expected {COMPAT_ONLY_FUNCTION_COUNT})")
    assert len(core) == CORE_FUNCTION_COUNT, f"core catalog drifted: {len(core)}"
    assert len(compat) == COMPAT_ONLY_FUNCTION_COUNT, f"compat catalog drifted: {len(compat)}"
    print("OK")
