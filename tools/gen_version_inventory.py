#!/usr/bin/env python3
"""Generate the desktop OpenGL 1.0-4.6 version/profile inventory from the Khronos registry.

Source of truth
---------------
``docs/khronos/gl.xml`` -- the vendored Khronos OpenGL-Registry.  Nothing in
this tool is hardcoded from the registry: every command/enum name, every
count and every version boundary is derived from the XML.

Derivation semantics
--------------------
This tool implements Khronos ``xml/reg.py`` PASS 1 semantics, the same model
already used by :mod:`tools.gl46_catalog` (which this module imports and
reuses for the feature ordering and for the authoritative 4.6 cross-check).
The rules are:

* ``<feature api="gl" name="GL_VERSION_X_Y">`` blocks are processed in
  ascending version order (1.0 .. 4.6, no version skipped).
* ``<require>`` without a ``profile`` attribute  -> available in *every* profile.
* ``<require profile="core">``                  -> available in the core profile.
* ``<require profile="compatibility">``          -> available in the
  compatibility profile only.
* ``<remove>`` without a ``profile`` attribute   -> removed everywhere from
  that version onward.
* ``<remove profile="core">``                   -> removed from the core
  profile only.
* ``<remove profile="compatibility">``           -> removed from the
  compatibility profile only.
* Last write wins in version order: a later ``<require>`` restores a symbol an
  earlier ``<remove>`` dropped.  Canonical example: ``glGetPointerv`` is
  removed from core in GL 3.2 and re-required in GL 4.3, so it *is* an OpenGL
  4.6 core entry point.
* Inside a single feature block the ``<remove>`` blocks are applied before the
  ``<require>`` blocks (reg.py order).  The current registry contains no
  symbol that is both required and removed by the same feature; the tool
  asserts that invariant so a future registry drop cannot silently change the
  result.

Profile model
-------------
OpenGL 1.0 .. 3.1 carry no ``profile`` attribute at all in the registry: the
core/compatibility split only appears with GL 3.2.  Those versions are modelled
as ``legacy`` -- a single, unsplit API surface.  They are still emitted per
version, and their ``core`` and ``compatibility`` sets are identical.

Two different notions of "compatibility only" exist and are reported
separately, because conflating them is a well known source of wrong numbers:

``compat_only``
    The symbol is required by a ``<require profile="compatibility">`` block and
    is never a core entry point.  At GL 4.6 that is 42 commands (the
    ``glVertexP*`` / ``glTexCoordP*`` / ... family of GL 3.3 and the 12
    ``glGetn*`` entry points of GL 4.5).  This is the definition used by
    ``tools/gl46_catalog.py`` and is what the self-check asserts.

``compat_not_core``
    The symbol is present in the compatibility profile but absent from the core
    profile.  At GL 4.6 that is 391 commands, because the 350 fixed-function
    entry points dropped from core in GL 3.2 are still part of the
    compatibility profile.

Extensions
----------
Extension commands and enums are reported in a separate ``extensions``
section.  They are never mixed into the per-version core/compatibility
command or enum counts.

Self-check
----------
Running the tool asserts the two numbers already established in
``tools/gl46_catalog.py`` for OpenGL 4.6:

* core commands              == 657
* compatibility-only commands ==  42

If either assertion fails the tool exits non-zero with a clear message.

Usage
-----
    python3 tools/gen_version_inventory.py
        Print the per-version summary table and run the self-check.

    python3 tools/gen_version_inventory.py --out data/version_inventory.json
        Write the deterministic JSON inventory.

    python3 tools/gen_version_inventory.py --docs-from data/version_inventory.json \\
            --gl-doc docs/OPENGL_VERSION_MATRIX.md \\
            --glsl-doc docs/GLSL_VERSION_MATRIX.md
        Regenerate the two Markdown matrices *from the JSON artifact*.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import sys
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parents[1]
GLXML = ROOT / "docs/khronos/gl.xml"
DEFAULT_JSON = ROOT / "data/version_inventory.json"

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gl46_catalog  # noqa: E402  (local module, path injected above)

#: Canonical OpenGL 4.6 Core Profile entry-point count (Khronos registry).
EXPECTED_CORE_COMMANDS_46 = gl46_catalog.CORE_FUNCTION_COUNT

#: Canonical OpenGL 4.6 compatibility-profile-only entry-point count.
EXPECTED_COMPAT_ONLY_COMMANDS_46 = gl46_catalog.COMPAT_ONLY_FUNCTION_COUNT

#: Fallback first OpenGL version that carries profile attributes in the
#: registry.  The actual split point is *derived* from ``gl.xml`` (see
#: :func:`_profile_split_version`), this is only used if the registry
#: changes shape.
PROFILE_SPLIT_VERSION = (3, 2)


def _profile_split_version(features):
    """First desktop GL version whose feature block mentions a profile.

    OpenGL 1.0 .. 3.1 carry no ``profile`` attribute anywhere in
    ``gl.xml``: the core/compatibility split only exists from GL 3.2
    onwards, so everything below the split is a single "legacy" surface.
    """
    for key, _name, feature in features:
        for block in list(feature.findall("require")) + list(feature.findall("remove")):
            if block.get("profile"):
                return key
    return PROFILE_SPLIT_VERSION


# --------------------------------------------------------------------------
# Registry parsing
# --------------------------------------------------------------------------

def _desktop_features(root):
    """All desktop ``GL_VERSION_x_y`` features, ascending by version.

    Reuses ``gl46_catalog._desktop_features`` so that the feature ordering
    (and therefore the whole derivation) cannot drift between the two tools.
    """
    return gl46_catalog._desktop_features(root)


def _parse_registry(path=None):
    """Parse ``gl.xml`` and return ``(root, sha256, byte_size, path)``."""
    p = pathlib.Path(path) if path else GLXML
    raw = p.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    try:
        # keep the recorded path repo-relative so the JSON stays portable
        shown = p.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        shown = str(p.resolve())
    return ET.parse(str(p)).getroot(), digest, len(raw), shown


# --------------------------------------------------------------------------
# Inventory construction
# --------------------------------------------------------------------------

class _Symbol:
    """Per-symbol bookkeeping across the version sweep."""

    __slots__ = (
        "name",
        "kind",
        "first_required",
        "core_first",
        "compat_first",
        "core_removals",
        "core_restorations",
        "compat_removals",
        "compat_restorations",
        "registry_compat_required",
        "in_core",
        "in_compat",
    )

    def __init__(self, name, kind):
        self.name = name
        self.kind = kind
        self.first_required = None
        self.core_first = None
        self.compat_first = None
        self.core_removals = []
        self.core_restorations = []
        self.compat_removals = []
        self.compat_restorations = []
        self.registry_compat_required = False
        self.in_core = False
        self.in_compat = False

    def to_json(self):
        compat_only = self.registry_compat_required and not self.in_core
        return {
            "kind": self.kind,
            "introduced_version": self.first_required,
            "core_available": self.in_core,
            "compatibility_available": self.in_compat,
            "compat_only": compat_only,
            "compat_not_core": self.in_compat and not self.in_core,
            "core_introduced_version": self.core_first,
            "compatibility_introduced_version": self.compat_first,
            "removed_in_core_at": list(self.core_removals),
            "restored_in_core_at": list(self.core_restorations),
            "removed_in_compatibility_at": list(self.compat_removals),
            "restored_in_compatibility_at": list(self.compat_restorations),
        }


def build_inventory(glxml=None):
    """Build the complete inventory dictionary.

    Returns a plain ``dict`` that is directly JSON serialisable and whose
    ordering is fully deterministic (sorted keys / sorted lists).
    """
    root, digest, nbytes, registry_path = _parse_registry(glxml)
    features = _desktop_features(root)

    symbols = {}          # name -> _Symbol
    core_cmds = set()
    compat_cmds = set()
    core_enums = set()
    compat_enums = set()
    registry_compat_required = set()
    registry_compat_required_enums = set()

    versions = {}
    profile_model = "legacy"
    split_version = _profile_split_version(features)

    for key, feature_name, feature in features:
        ver = "%d.%d" % key

        # --- profile model -------------------------------------------------
        if key >= split_version:
            profile_model = "split"

        # --- invariant: no symbol is required *and* removed by one feature --
        req_cmds = {c.get("name") for r in feature.findall("require")
                    for c in r.findall("command") if c.get("name")}
        rem_cmds = {c.get("name") for r in feature.findall("remove")
                    for c in r.findall("command") if c.get("name")}
        req_enums = {e.get("name") for r in feature.findall("require")
                     for e in r.findall("enum") if e.get("name")}
        rem_enums = {e.get("name") for r in feature.findall("remove")
                     for e in r.findall("enum") if e.get("name")}
        assert not (req_cmds & rem_cmds), (
            "%s requires and removes the same command(s): %s"
            % (feature_name, sorted(req_cmds & rem_cmds)))
        assert not (req_enums & rem_enums), (
            "%s requires and removes the same enum(s): %s"
            % (feature_name, sorted(req_enums & rem_enums)))

        prev_core_cmds = set(core_cmds)
        prev_compat_cmds = set(compat_cmds)
        prev_core_enums = set(core_enums)
        prev_compat_enums = set(compat_enums)

        # --- <remove> blocks (reg.py applies removes before requires) ------
        for rem in feature.findall("remove"):
            profile = rem.get("profile")
            for c in rem.findall("command"):
                name = c.get("name")
                if not name:
                    continue
                sym = symbols.get(name)
                if profile in (None, "core"):
                    core_cmds.discard(name)
                    if sym is not None:
                        sym.core_removals.append(ver)
                        sym.in_core = False
                if profile in (None, "compatibility"):
                    compat_cmds.discard(name)
                    if sym is not None:
                        sym.compat_removals.append(ver)
                        sym.in_compat = False
            for e in rem.findall("enum"):
                name = e.get("name")
                if not name:
                    continue
                sym = symbols.get(name)
                if profile in (None, "core"):
                    core_enums.discard(name)
                    if sym is not None:
                        sym.core_removals.append(ver)
                        sym.in_core = False
                if profile in (None, "compatibility"):
                    compat_enums.discard(name)
                    if sym is not None:
                        sym.compat_removals.append(ver)
                        sym.in_compat = False

        # --- <require> blocks ---------------------------------------------
        new_cmds = []
        new_enums = []
        for req in feature.findall("require"):
            profile = req.get("profile")
            for c in req.findall("command"):
                name = c.get("name")
                if not name:
                    continue
                sym = symbols.get(name)
                if sym is None:
                    sym = symbols[name] = _Symbol(name, "command")
                if sym.first_required is None:
                    sym.first_required = ver
                    new_cmds.append(name)
                if profile in (None, "core"):
                    if name not in core_cmds:
                        core_cmds.add(name)
                        sym.in_core = True
                        if sym.core_first is None:
                            sym.core_first = ver
                        elif sym.core_removals:
                            # restored after an earlier <remove>
                            sym.core_restorations.append(ver)
                    # the compatibility profile is a superset of core
                    if name not in compat_cmds:
                        compat_cmds.add(name)
                        sym.in_compat = True
                        if sym.compat_first is None:
                            sym.compat_first = ver
                    else:
                        sym.in_compat = True
                if profile == "compatibility":
                    if name not in compat_cmds:
                        compat_cmds.add(name)
                        sym.in_compat = True
                        if sym.compat_first is None:
                            sym.compat_first = ver
                    sym.registry_compat_required = True
                    registry_compat_required.add(name)
            for e in req.findall("enum"):
                name = e.get("name")
                if not name:
                    continue
                sym = symbols.get(name)
                if sym is None:
                    sym = symbols[name] = _Symbol(name, "enum")
                if sym.first_required is None:
                    sym.first_required = ver
                    new_enums.append(name)
                if profile in (None, "core"):
                    if name not in core_enums:
                        core_enums.add(name)
                        sym.in_core = True
                        if sym.core_first is None:
                            sym.core_first = ver
                        elif sym.core_removals:
                            sym.core_restorations.append(ver)
                    if name not in compat_enums:
                        compat_enums.add(name)
                        sym.in_compat = True
                        if sym.compat_first is None:
                            sym.compat_first = ver
                    else:
                        sym.in_compat = True
                if profile == "compatibility":
                    if name not in compat_enums:
                        compat_enums.add(name)
                        sym.in_compat = True
                        if sym.compat_first is None:
                            sym.compat_first = ver
                    sym.registry_compat_required = True
                    registry_compat_required_enums.add(name)

        core_added_cmds = sorted(core_cmds - prev_core_cmds)
        compat_added_cmds = sorted(compat_cmds - prev_compat_cmds)
        core_removed_cmds = sorted(prev_core_cmds - core_cmds)
        compat_removed_cmds = sorted(prev_compat_cmds - compat_cmds)
        core_added_enums = sorted(core_enums - prev_core_enums)
        compat_added_enums = sorted(compat_enums - prev_compat_enums)
        core_removed_enums = sorted(prev_core_enums - core_enums)
        compat_removed_enums = sorted(prev_compat_enums - compat_enums)

        # compatibility-only (registry marked profile="compatibility") that are
        # NOT part of the core profile.
        compat_only_cmds = sorted(registry_compat_required - core_cmds)
        compat_only_enums = sorted(registry_compat_required_enums - core_enums)

        versions[ver] = {
            "version": ver,
            "version_key": [key[0], key[1]],
            "feature": feature_name,
            "profile_model": profile_model,
            "glsl_version": GLSL_BY_GL.get(ver),
            "counts": {
                "total_commands": len(core_cmds | compat_cmds),
                "core_commands": len(core_cmds),
                "compatibility_commands": len(compat_cmds),
                "compatibility_only_commands": len(compat_only_cmds),
                "compat_not_core_commands": len(compat_cmds - core_cmds),
                "core_enums": len(core_enums),
                "compatibility_enums": len(compat_enums),
                "compatibility_only_enums": len(compat_only_enums),
                "compat_not_core_enums": len(compat_enums - core_enums),
                "new_commands": len(new_cmds),
                "new_enums": len(new_enums),
                "new_core_commands": len(core_added_cmds),
                "new_compatibility_commands": len(compat_added_cmds),
                "removed_in_core_commands": len(core_removed_cmds),
                "removed_in_core_enums": len(core_removed_enums),
                "removed_in_compatibility_commands": len(compat_removed_cmds),
                "removed_in_compatibility_enums": len(compat_removed_enums),
            },
            "commands": {
                "core": sorted(core_cmds),
                "compatibility": sorted(compat_cmds),
                "new": sorted(new_cmds),
                "new_core": core_added_cmds,
                "new_compatibility": compat_added_cmds,
                "removed_in_core": core_removed_cmds,
                "removed_in_compatibility": compat_removed_cmds,
                "compatibility_only": compat_only_cmds,
            },
            "enums": {
                "core": sorted(core_enums),
                "compatibility": sorted(compat_enums),
                "new": sorted(new_enums),
                "new_core": core_added_enums,
                "new_compatibility": compat_added_enums,
                "removed_in_core": core_removed_enums,
                "removed_in_compatibility": compat_removed_enums,
                "compatibility_only": compat_only_enums,
            },
        }

    # ---- per-symbol records ---------------------------------------------
    commands = {}
    enums = {}
    for name, sym in symbols.items():
        rec = sym.to_json()
        if sym.kind == "command":
            commands[name] = rec
        else:
            enums[name] = rec

    # ---- extensions (separate section, never mixed into the counts) -----
    extensions = _parse_extensions(root, core_cmds, versions)

    # ---- self-check ------------------------------------------------------
    self_check = {
        "core_commands_4_6": len(versions["4.6"]["commands"]["core"]),
        "expected_core_commands_4_6": EXPECTED_CORE_COMMANDS_46,
        "compatibility_only_commands_4_6":
            len(versions["4.6"]["commands"]["compatibility_only"]),
        "expected_compatibility_only_commands_4_6":
            EXPECTED_COMPAT_ONLY_COMMANDS_46,
        "compat_not_core_commands_4_6":
            len(set(versions["4.6"]["commands"]["compatibility"])
                - set(versions["4.6"]["commands"]["core"])),
    }
    self_check["passed"] = (
        self_check["core_commands_4_6"] == EXPECTED_CORE_COMMANDS_46
        and self_check["compatibility_only_commands_4_6"]
        == EXPECTED_COMPAT_ONLY_COMMANDS_46)

    # ---- cross-check against tools/gl46_catalog.py -----------------------
    catalog_core = gl46_catalog.parse_core_commands(glxml)
    catalog_compat = gl46_catalog.parse_compat_only_commands(glxml)
    catalog_core_46 = set(catalog_core)
    catalog_compat_46 = set(catalog_compat)
    inventory_core_46 = set(versions["4.6"]["commands"]["core"])
    inventory_compat_46 = set(versions["4.6"]["commands"]["compatibility_only"])
    self_check["matches_gl46_catalog_core"] = (
        catalog_core_46 == inventory_core_46)
    self_check["matches_gl46_catalog_compat_only"] = (
        catalog_compat_46 == inventory_compat_46)

    inventory = {
        "schema_version": 1,
        "generator": "tools/gen_version_inventory.py",
        "source": {
            "registry_path": str(registry_path),
            "registry_sha256": digest,
            "registry_bytes": nbytes,
            "registry_kind": "Khronos OpenGL-Registry (vendored)",
        },
        "semantics": {
            "feature_order": "ascending GL_VERSION_x_y, api=gl only, <= 4.6",
            "require_no_profile": "available in every profile",
            "require_profile_core": "core profile only",
            "require_profile_compatibility": "compatibility profile only",
            "remove_no_profile": "removed everywhere from that version onward",
            "remove_profile_core": "removed from the core profile only",
            "remove_profile_compatibility":
                "removed from the compatibility profile only",
            "last_write_wins": True,
            "within_feature_order": "remove blocks applied before require blocks",
            "compatibility_is_superset_of_core": True,
            "profile_model_legacy_versions":
                "GL 1.0 - 3.1 carry no profile attribute in gl.xml and are "
                "modelled as a single unsplit surface (core == compatibility)",
            "compat_only_definition":
                "required by a `profile=\"compatibility\"` require block and "
                "never a core entry point (this is the definition used by "
                "tools/gl46_catalog.py and by the self-check)",
            "compat_not_core_definition":
                "present in the compatibility profile but absent from the core "
                "profile (includes the fixed-function entry points dropped from "
                "core in GL 3.2)",
            "extensions_are_separate": True,
        },
        "profiles": {
            "core": "OpenGL core profile (profile='core' or no profile attribute)",
            "compatibility": "OpenGL compatibility profile",
            "legacy": "GL 1.0 - 3.1: no profile attribute in the registry",
        },
        "self_check": self_check,
        "totals": {
            "versions": len(versions),
            "commands": len(commands),
            "enums": len(enums),
            "extensions": len(extensions),
        },
        "versions": versions,
        "commands": commands,
        "enums": enums,
        "extensions": extensions,
        "glsl": GLSL_DATA,
    }
    return inventory


def _parse_extensions(root, core_set, versions):
    """Parse the ``<extensions>`` section into a deterministic mapping.

    Extension commands/enums are *never* folded into the per-version
    core/compatibility counts; they live only in this section.
    """
    out = {}
    exts = root.find("extensions")
    if exts is None:
        return out
    version_order = sorted(versions, key=lambda v: tuple(versions[v]["version_key"]))
    for ext in exts.findall("extension"):
        name = ext.get("name")
        if not name:
            continue
        supported = ext.get("supported") or ""
        commands = set()
        enums = set()
        compat_only_commands = set()
        compat_only_enums = set()
        for req in ext.findall("require"):
            profile = req.get("profile")
            for c in req.findall("command"):
                if c.get("name"):
                    commands.add(c.get("name"))
                    if profile == "compatibility":
                        compat_only_commands.add(c.get("name"))
            for e in req.findall("enum"):
                if e.get("name"):
                    enums.add(e.get("name"))
                    if profile == "compatibility":
                        compat_only_enums.add(e.get("name"))
        # earliest version at which *all* extension commands are core
        promoted = None
        if commands:
            for v in version_order:
                if commands <= set(versions[v]["commands"]["core"]):
                    promoted = v
                    break
        desktop = "gl" in supported.split("|") and supported != "disabled"
        out[name] = {
            "supported": supported,
            "supported_desktop_gl": desktop,
            "command_count": len(commands),
            "enum_count": len(enums),
            "commands": sorted(commands),
            "enums": sorted(enums),
            "compatibility_only_commands": sorted(compat_only_commands),
            "compatibility_only_enums": sorted(compat_only_enums),
            "core_command_count": len(commands & core_set),
            "fully_promoted_to_core_in_version": promoted,
        }
    return dict(sorted(out.items()))


# --------------------------------------------------------------------------
# GL <-> GLSL anchor table
# --------------------------------------------------------------------------

#: The GL version that first provides each GLSL version (desktop GL).
GLSL_BY_GL = {
    "2.0": "1.10",
    "2.1": "1.20",
    "3.0": "1.30",
    "3.1": "1.40",
    "3.2": "1.50",
    "3.3": "3.30",
    "4.0": "4.00",
    "4.1": "4.10",
    "4.2": "4.20",
    "4.3": "4.30",
    "4.4": "4.40",
    "4.5": "4.50",
    "4.6": "4.60",
}

GL_BY_GLSL = {v: k for k, v in GLSL_BY_GL.items()}

#: GLSL stage names, in pipeline order.
STAGES = ["vertex", "tessellation control", "tessellation evaluation",
          "geometry", "fragment", "compute"]


def _g(ver, gl, stages, features):
    """Build one GLSL version record.

    ``features`` is a list of ``(text, provenance, reference)`` tuples where
    provenance is ``"pdf"`` (verified against the vendored GLSL 4.60 spec
    PDF) or ``"general-knowledge"`` (not verifiable from the vendored PDF).
    """
    return {
        "glsl_version": ver,
        "gl_version": gl,
        "profiles": ("none (pre-profile language)" if ver in ("1.10", "1.20",
                                                              "1.30", "1.40")
                     else "core and compatibility"),
        "stages": list(stages),
        "features": [
            {"text": t, "provenance": p, "reference": r} for (t, p, r) in features
        ],
    }


#: Per-GLSL-version data.
#:
#: Provenance markers:
#:   "pdf"  -- verified against docs/khronos/GLSLangSpec.4.60.pdf
#:   "knowledge" -- not verifiable from the vendored PDF (historical knowledge)
GLSL_DATA = {
    "1.10": _g("1.10", "2.0", ["vertex", "fragment"], [
        ("First release of the OpenGL Shading Language; replaces the GL 1.1 "
         "fixed-function vertex/fragment program model.", "knowledge", ""),
        ("Two stages only: vertex and fragment.", "knowledge", ""),
        ("Qualifiers const, uniform, attribute, varying; types float, vec2-4, "
         "mat2-4, bool, sampler2D.", "knowledge", ""),
        ("Built-ins gl_Position, gl_FragColor, gl_FragCoord, gl_PointSize, "
         "gl_Normal, gl_Color, gl_TexCoord.", "knowledge", ""),
        ("No #version directive is required; a shader without one is treated "
         "as targeting version 1.10.", "pdf", "GLSL 4.60 spec, sec. 3.3, PDF p.19"),
        ("No integer types and no geometry, tessellation, compute, image or "
         "buffer types.", "knowledge", ""),
    ]),
    "1.20": _g("1.20", "2.1", ["vertex", "fragment"], [
        ("Integer scalar and vector types (int, uint, ivec2-4, uvec2-4) and "
         "integer uniforms.", "knowledge", ""),
        ("Non-square matrix types (mat2x3, mat3x2, mat2x4, mat4x2, ...).",
         "knowledge", ""),
        ("Array constructors, for example float[3](1.0, 2.0, 3.0).",
         "knowledge", ""),
        ("Stage set unchanged: vertex and fragment only.", "knowledge", ""),
    ]),
    "1.30": _g("1.30", "3.0", ["vertex", "fragment"], [
        ("The #version directive becomes mandatory and the declared version "
         "must match the language in use.", "knowledge", ""),
        ("Bitwise operators on integers (& | ^ ~ << >>), ++/--, compound "
         "assignment, switch and do-while statements.", "knowledge", ""),
        ("Explicit-LOD and texel-fetch texture functions (textureLod, "
         "texelFetch).", "knowledge", ""),
        ("gl_FragDepth and gl_FrontFacing built-ins.", "knowledge", ""),
        ("Multiple render targets through gl_FragData[].", "knowledge", ""),
        ("Integer texture formats and their sampler types (isampler*/"
         "usampler*, ARB_texture_integer).", "knowledge", ""),
        ("Precision qualifiers lowp, mediump, highp.", "knowledge", ""),
        ("Stage set unchanged: vertex and fragment only.", "knowledge", ""),
    ]),
    "1.40": _g("1.40", "3.1", ["vertex", "fragment", "geometry"], [
        ("Geometry shader stage introduced (ARB_geometry_shader4, OpenGL 3.1).",
         "knowledge", ""),
        ("Uniform blocks with an explicit std140 layout.", "knowledge", ""),
        ("textureGather family and the textureOffset variants.", "knowledge", ""),
        ("samplerBuffer, for texture buffer objects (ARB_texture_buffer_object, "
         "OpenGL 3.1).", "knowledge", ""),
    ]),
    "1.50": _g("1.50", "3.2", ["vertex", "fragment", "geometry"], [
        ("First version that carries a profile argument: '#version 150 core' "
         "or '#version 150 compatibility'. A profile argument can only be "
         "used with version 150 or greater, and if no profile is given the "
         "default is core.", "pdf", "GLSL 4.60 spec, sec. 3.3, PDF p.19"),
        ("'in'/'out' replace 'attribute'/'varying'; the latter are deprecated "
         "but remain available in the compatibility profile.", "knowledge", ""),
        ("layout qualifiers, including layout(location=) for fragment "
         "outputs.", "knowledge", ""),
        ("Separate sampler objects are distinguished from combined "
         "sampler/image uniform slots.", "knowledge", ""),
        ("gl_FragColor and gl_FragData exist only in the compatibility "
         "profile.", "pdf", "GLSL 4.60 spec, sec. 7.1/7.2, PDF p.156"),
        ("Tessellation shaders are NOT part of this version; they arrive with "
         "GLSL 4.00.", "knowledge", ""),
    ]),
    "3.30": _g("3.30", "3.3", ["vertex", "fragment", "geometry"], [
        ("Version 330: the language version required by the OpenGL 3.3 core "
         "profile.", "knowledge", ""),
        ("'in'/'out' everywhere; 'attribute'/'varying' are no longer part of "
         "the core language.", "knowledge", ""),
        ("layout(location=) extended to vertex inputs and outputs "
         "(ARB_enhanced_layouts).", "knowledge", ""),
        ("gl_VertexID and gl_InstanceID built-ins.", "knowledge", ""),
        ("Stage set unchanged from 1.50: vertex, fragment and geometry. "
         "Tessellation shaders arrive with GLSL 4.00.", "knowledge", ""),
    ]),
    "4.00": _g("4.00", "4.0", ["vertex", "fragment", "geometry",
                              "tessellation control",
                              "tessellation evaluation"], [
        ("Tessellation control and tessellation evaluation shader stages "
         "introduced (ARB_tessellation_shader, OpenGL 4.0).", "knowledge", ""),
        ("64-bit integer types int64_t and uint64_t (ARB_gpu_shader_int64).",
         "knowledge", ""),
        ("Double-precision floating point: double, dvec2-4, dmat2-4 "
         "(ARB_gpu_shader_fp64).", "knowledge", ""),
        ("Texture LOD query functions (textureQueryLod, textureQueryLevels).",
         "knowledge", ""),
        ("Transform feedback 3.0: multiple streams and interleaved queries.",
         "knowledge", ""),
    ]),
    "4.10": _g("4.10", "4.1", ["vertex", "fragment", "geometry",
                              "tessellation control",
                              "tessellation evaluation"], [
        ("gl_ViewportIndex built-in (ARB_viewport_array).", "knowledge", ""),
        ("Separate sampler objects: 'sampler' is a distinct opaque type from "
         "the texture sampler types.", "knowledge", ""),
        ("No new shader stage.", "knowledge", ""),
    ]),
    "4.20": _g("4.20", "4.2", ["vertex", "fragment", "geometry",
                              "tessellation control",
                              "tessellation evaluation"], [
        ("Image types (image1D..imageBuffer, iimage*, uimage*) with "
         "imageLoad/imageStore/imageAtomic* (ARB_shader_image_load_store).",
         "knowledge", ""),
        ("Atomic counters (atomic_uint) with atomicCounterIncrement, "
         "atomicCounterDecrement and friends (ARB_shader_atomic_counters).",
         "knowledge", ""),
        ("Memory qualifiers coherent, volatile, restrict, readonly, "
         "writeonly on image and buffer variables.", "knowledge", ""),
        ("No new shader stage.", "knowledge", ""),
    ]),
    "4.30": _g("4.30", "4.3", ["vertex", "fragment", "geometry",
                              "tessellation control",
                              "tessellation evaluation", "compute"], [
        ("Compute shader stage introduced (OpenGL 4.3) with local_size_x/y/z, "
         "shared variables, memoryBarrier and group-shared storage.",
         "knowledge", ""),
        ("Shader storage buffer objects (layout(std430) buffer variables).",
         "knowledge", ""),
        ("Subroutines: the 'subroutine' keyword, subroutine types and "
         "subroutine uniforms.", "knowledge", ""),
        ("Program interface queries: named uniforms and uniform blocks with "
         "resource locations.", "knowledge", ""),
        ("Image load/store extended to the vertex, tessellation and geometry "
         "stages.", "knowledge", ""),
    ]),
    "4.40": _g("4.40", "4.4", ["vertex", "fragment", "geometry",
                              "tessellation control",
                              "tessellation evaluation", "compute"], [
        ("Extended layout qualifier set on uniforms and blocks (binding, "
         "offset, align, ...).", "knowledge", ""),
        ("Dynamic indexing of arrays of samplers and of uniform arrays.",
         "knowledge", ""),
        ("ARB_gpu_shader5 arithmetic and layout extensions.", "knowledge", ""),
        ("No new shader stage.", "knowledge", ""),
    ]),
    "4.50": _g("4.50", "4.5", ["vertex", "fragment", "geometry",
                              "tessellation control",
                              "tessellation evaluation", "compute"], [
        ("Cull distance built-ins (gl_CullDistance, ARB_clip_cull_distance).",
         "knowledge", ""),
        ("The 4.50 revision does not yet contain ARB_shader_atomic_counter_ops, "
         "ARB_shader_draw_parameters, ARB_shader_group_vote or ARB_gl_spirv: "
         "the 4.60 revision summary lists all four as newly incorporated.",
         "pdf", "GLSL 4.60 spec, sec. 1.1.6, PDF p.8"),
        ("No new shader stage.", "knowledge", ""),
    ]),
    "4.60": _g("4.60", "4.6", ["vertex", "fragment", "geometry",
                              "tessellation control",
                              "tessellation evaluation", "compute"], [
        ("'#version 460' is required in any shader that uses version 4.60 of "
         "the language, and __VERSION__ substitutes the decimal integer 460.",
         "pdf", "GLSL 4.60 spec, sec. 3.3, PDF p.17 and p.19"),
        ("Incorporates GL_ARB_shader_atomic_counter_ops, "
         "GL_ARB_shader_draw_parameters, GL_ARB_shader_group_vote and "
         "GL_ARB_gl_spirv.", "pdf", "GLSL 4.60 spec, sec. 1.1.6, PDF p.8"),
        ("Six stages: vertex, tessellation control, tessellation evaluation, "
         "geometry, fragment and compute.", "pdf",
         "GLSL 4.60 spec, Chapter 2, PDF p.11"),
        ("The compatibility profile adds the fixed-function fragment outputs "
         "gl_FragColor and gl_FragData and the fixed-function vertex inputs "
         "gl_Color, gl_SecondaryColor, gl_Normal, gl_Vertex and "
         "gl_MultiTexCoord0-7.", "pdf", "GLSL 4.60 spec, sec. 7.1/7.2, PDF p.156"),
        ("When compiling for OpenGL SPIR-V the macro GL_SPIRV is predefined as "
         "100.", "pdf", "GLSL 4.60 spec, PDF p.22"),
        ("Compatibility-profile features are not available when generating "
         "SPIR-V.", "pdf", "GLSL 4.60 spec, sec. 3.3, PDF p.19"),
    ]),
}


# --------------------------------------------------------------------------
# Self-check
# --------------------------------------------------------------------------

def run_self_check(inventory):
    """Raise ``SystemExit`` with a clear message when the self-check fails."""
    sc = inventory["self_check"]
    registry = inventory["source"]["registry_path"]
    problems = []
    if sc["core_commands_4_6"] != EXPECTED_CORE_COMMANDS_46:
        problems.append(
            "OpenGL 4.6 core command count is %d, expected %d"
            % (sc["core_commands_4_6"], EXPECTED_CORE_COMMANDS_46))
    if sc["compatibility_only_commands_4_6"] != EXPECTED_COMPAT_ONLY_COMMANDS_46:
        problems.append(
            "OpenGL 4.6 compatibility-only command count is %d, expected %d"
            % (sc["compatibility_only_commands_4_6"],
               EXPECTED_COMPAT_ONLY_COMMANDS_46))
    if not sc.get("matches_gl46_catalog_core", False):
        problems.append(
            "core set differs from tools/gl46_catalog.parse_core_commands()")
    if not sc.get("matches_gl46_catalog_compat_only", False):
        problems.append(
            "compatibility-only set differs from "
            "tools/gl46_catalog.parse_compat_only_commands()")
    if problems:
        sys.stderr.write("SELF-CHECK FAILED:\n")
        for p in problems:
            sys.stderr.write("  - %s\n" % p)
        sys.stderr.write(
            "The Khronos registry in %s no longer matches the numbers "
            "established in tools/gl46_catalog.py. Re-derive the catalog "
            "before regenerating the inventory.\n" % registry)
        raise SystemExit(1)


# --------------------------------------------------------------------------
# Reporting
# --------------------------------------------------------------------------

def format_summary(inventory):
    """Render the human-readable summary table."""
    lines = []
    lines.append("OpenGL desktop version inventory")
    lines.append("source: %s (sha256 %s)"
                 % (inventory["source"]["registry_path"],
                    inventory["source"]["registry_sha256"][:16]))
    lines.append("")
    header = ("GL    model    core  compat  new  rm-core   coreE  compatE  "
              "compat-only  GLSL")
    lines.append(header)
    lines.append("-" * len(header))
    for ver in sorted(inventory["versions"],
                      key=lambda v: tuple(inventory["versions"][v]["version_key"])):
        v = inventory["versions"][ver]
        c = v["counts"]
        lines.append(
            "%-5s %-8s %5d  %6d  %4d  %6d  %6d  %7d  %11d  %s"
            % (ver, v["profile_model"], c["core_commands"],
               c["compatibility_commands"], c["new_commands"],
               c["removed_in_core_commands"], c["core_enums"],
               c["compatibility_enums"], c["compatibility_only_commands"],
               v["glsl_version"] or "-"))
    lines.append("")
    sc = inventory["self_check"]
    lines.append("totals: %d versions, %d commands, %d enums, %d extensions"
                 % (inventory["totals"]["versions"],
                    inventory["totals"]["commands"],
                    inventory["totals"]["enums"],
                    inventory["totals"]["extensions"]))
    lines.append("self-check: GL 4.6 core commands %d (expected %d) | "
                 "compatibility-only commands %d (expected %d) | "
                 "catalog cross-check %s"
                 % (sc["core_commands_4_6"], sc["expected_core_commands_4_6"],
                    sc["compatibility_only_commands_4_6"],
                    sc["expected_compatibility_only_commands_4_6"],
                    "OK" if (sc["matches_gl46_catalog_core"]
                             and sc["matches_gl46_catalog_compat_only"]) else "MISMATCH"))
    lines.append("note: 'compat-only' = required with profile=\"compatibility\" "
                 "and never core (tools/gl46_catalog definition).")
    lines.append("      'compat-not-core' at 4.6 = %d = %d compatibility-only + "
                 "%d fixed-function entry points dropped from core in GL 3.2 "
                 "and not restored."
                 % (sc["compat_not_core_commands_4_6"],
                    sc["compatibility_only_commands_4_6"],
                    sc["compat_not_core_commands_4_6"]
                    - sc["compatibility_only_commands_4_6"]))
    return "\n".join(lines)


# --------------------------------------------------------------------------
# Markdown generation (from the JSON artifact)
# --------------------------------------------------------------------------

def _md_escape(text):
    """Escape a value for a Markdown table cell."""
    return str(text).replace("|", "\\|")


def render_gl_markdown(inventory):
    """Render ``docs/OPENGL_VERSION_MATRIX.md`` from the inventory."""
    sc = inventory["self_check"]
    out = []
    a = out.append
    a("# OpenGL Desktop Version Matrix (1.0 - 4.6, Core + Compatibility)")
    a("")
    a("Generated file -- do not edit by hand.")
    a("")
    a("Regenerate with:")
    a("")
    a("```")
    a("python3 tools/gen_version_inventory.py --out data/version_inventory.json")
    a("python3 tools/gen_version_inventory.py --docs-from data/version_inventory.json \\")
    a("        --gl-doc docs/OPENGL_VERSION_MATRIX.md \\")
    a("        --glsl-doc docs/GLSL_VERSION_MATRIX.md")
    a("```")
    a("")
    a("Source of truth: `%s` (vendored Khronos OpenGL-Registry, sha256 `%s`)."
      % (inventory["source"]["registry_path"],
         inventory["source"]["registry_sha256"]))
    a("")
    a("Every number below is read from `data/version_inventory.json`, which is")
    a("produced by `tools/gen_version_inventory.py`. No count in this document is")
    a("typed by hand.")
    a("")
    a("This document is a **specification inventory**. It says which entry")
    a("points and tokens the Khronos registry defines for each OpenGL version")
    a("and profile. It makes **no claim about CTS conformance and no claim")
    a("about what this implementation actually does at runtime**.")
    a("")
    a("## Derivation rules")
    a("")
    sem = inventory["semantics"]
    a("| Rule | Meaning |")
    a("| --- | --- |")
    for key in sorted(sem):
        a("| `%s` | %s |" % (_md_escape(key), _md_escape(sem[key])))
    a("")
    a("## Self-check")
    a("")
    a("| Check | Value | Expected |")
    a("| --- | --- | --- |")
    a("| GL 4.6 core commands | %d | %d |"
      % (sc["core_commands_4_6"], sc["expected_core_commands_4_6"]))
    a("| GL 4.6 compatibility-only commands | %d | %d |"
      % (sc["compatibility_only_commands_4_6"],
         sc["expected_compatibility_only_commands_4_6"]))
    a("| Matches `tools/gl46_catalog.parse_core_commands()` | %s | true |"
      % ("yes" if sc["matches_gl46_catalog_core"] else "NO"))
    a("| Matches `tools/gl46_catalog.parse_compat_only_commands()` | %s | true |"
      % ("yes" if sc["matches_gl46_catalog_compat_only"] else "NO"))
    a("")
    a("For reference, the number of entry points that are in the OpenGL 4.6")
    a("**compatibility profile but not the core profile** is **%d** -- the %d"
      % (sc["compat_not_core_commands_4_6"],
         sc["compatibility_only_commands_4_6"]))
    a("compatibility-only entry points plus the %d fixed-function entry points"
      % (sc["compat_not_core_commands_4_6"]
         - sc["compatibility_only_commands_4_6"]))
    a("that GL 3.2 removed from core and that were never restored. That is a")
    a("different question from \"which entry points does the registry mark")
    a("`profile='compatibility'`\", which is what the 42 above counts.")
    a("")
    a("## GL <-> GLSL anchor table")
    a("")
    a("| OpenGL version | GLSL version |")
    a("| --- | --- |")
    for gl in sorted(GLSL_BY_GL, key=lambda v: tuple(map(int, v.split(".")))):
        a("| %s | %s |" % (gl, GLSL_BY_GL[gl]))
    a("")
    a("OpenGL 1.0 - 1.5 have **no** GLSL: shaders are only reachable through")
    a("extensions (for example `GL_ARB_shader_objects`, `GL_ARB_vertex_shader`,")
    a("`GL_ARB_fragment_shader`) and are outside the version matrix below.")
    a("")
    a("## Per-version summary")
    a("")
    a("`core` = entry points in the OpenGL core profile at that version;")
    a("`compat` = entry points in the OpenGL compatibility profile at that")
    a("version; `compat-only` = required with `profile=\"compatibility\"` and")
    a("never core. `new` = entry points first required by that version.")
    a("`rm-core` = entry points removed from the core profile by that version.")
    a("`total` = union of the core and compatibility sets.")
    a("")
    a("| GL | model | total | core | compat | compat-only | new | rm-core | "
      "core enums | compat enums | GLSL |")
    a("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |")
    for ver in sorted(inventory["versions"],
                      key=lambda v: tuple(inventory["versions"][v]["version_key"])):
        v = inventory["versions"][ver]
        c = v["counts"]
        a("| %s | %s | %d | %d | %d | %d | %d | %d | %d | %d | %s |"
          % (ver, v["profile_model"], c["total_commands"], c["core_commands"],
             c["compatibility_commands"], c["compatibility_only_commands"],
             c["new_commands"], c["removed_in_core_commands"],
             c["core_enums"], c["compatibility_enums"],
             v["glsl_version"] or "-- (none)"))
    a("")
    a("### Reading the `core` column before GL 3.2")
    a("")
    a("The registry carries no `profile` attribute before `GL_VERSION_3_2`, so")
    a("GL 1.0 - 3.1 are a single unsplit surface: their `core` and `compat`")
    a("sets are identical by construction. The drop from %d commands at 3.1 to"
      % inventory["versions"]["3.1"]["counts"]["core_commands"])
    a("%d at 3.2 is the `GL_VERSION_3_2` `<remove profile=\"core\">` blocks"
      % inventory["versions"]["3.2"]["counts"]["core_commands"])
    a("removing the fixed-function API from the core profile -- the")
    a("compatibility profile keeps all of it.")
    a("")
    a("## Per-version detail")
    a("")
    for ver in sorted(inventory["versions"],
                      key=lambda v: tuple(inventory["versions"][v]["version_key"])):
        v = inventory["versions"][ver]
        a("### OpenGL %s (`%s`, %s profile model)"
          % (ver, v["feature"], v["profile_model"]))
        a("")
        a("- core commands: **%d**, compatibility commands: **%d**, "
          "compatibility-only commands: **%d**"
          % (v["counts"]["core_commands"], v["counts"]["compatibility_commands"],
             v["counts"]["compatibility_only_commands"]))
        a("- core enums: **%d**, compatibility enums: **%d**"
          % (v["counts"]["core_enums"], v["counts"]["compatibility_enums"]))
        a("- first required in this version: **%d** commands, **%d** enums"
          % (v["counts"]["new_commands"], v["counts"]["new_enums"]))
        a("- removed from the core profile in this version: **%d** commands, "
          "**%d** enums"
          % (v["counts"]["removed_in_core_commands"],
             v["counts"]["removed_in_core_enums"]))
        a("- GLSL that goes with it: **%s**"
          % (v["glsl_version"] or "none (pre-GLSL; extensions only)"))
        a("")
        new_c = v["commands"]["new"]
        if new_c:
            a("<details><summary>Commands first required in %s (%d)</summary>"
              % (ver, len(new_c)))
            a("")
            for n in new_c:
                a("- `%s`" % n)
            a("")
            a("</details>")
            a("")
        rm_c = v["commands"]["removed_in_core"]
        if rm_c:
            a("<details><summary>Commands removed from the core profile in %s "
              "(%d)</summary>" % (ver, len(rm_c)))
            a("")
            for n in rm_c:
                a("- `%s`" % n)
            a("")
            a("</details>")
            a("")
        co_c = v["commands"]["compatibility_only"]
        if co_c:
            a("<details><summary>Compatibility-only commands at %s (%d)</summary>"
              % (ver, len(co_c)))
            a("")
            for n in co_c:
                a("- `%s`" % n)
            a("")
            a("</details>")
            a("")
    a("## Extensions")
    a("")
    a("Extension commands and enums are **not** included in any count above.")
    a("They live in the separate `extensions` section of")
    a("`data/version_inventory.json` (%d extensions)."
      % inventory["totals"]["extensions"])
    a("")
    a("| Extension | desktop GL | commands | enums | fully core in |")
    a("| --- | --- | ---: | ---: | --- |")
    for name in sorted(inventory["extensions"]):
        e = inventory["extensions"][name]
        a("| `%s` | %s | %d | %d | %s |"
          % (name, "yes" if e["supported_desktop_gl"] else "no",
             e["command_count"], e["enum_count"],
             e["fully_promoted_to_core_in_version"] or "--"))
    a("")
    return "\n".join(out) + "\n"


def render_glsl_markdown(inventory):
    """Render ``docs/GLSL_VERSION_MATRIX.md`` from the inventory."""
    out = []
    a = out.append
    a("# GLSL Version Matrix (1.10 - 4.60)")
    a("")
    a("Generated file -- do not edit by hand.")
    a("")
    a("Regenerate with:")
    a("")
    a("```")
    a("python3 tools/gen_version_inventory.py --out data/version_inventory.json")
    a("python3 tools/gen_version_inventory.py --docs-from data/version_inventory.json \\")
    a("        --gl-doc docs/OPENGL_VERSION_MATRIX.md \\")
    a("        --glsl-doc docs/GLSL_VERSION_MATRIX.md")
    a("```")
    a("")
    a("## Provenance")
    a("")
    a("The per-version GLSL data lives in the `glsl` section of")
    a("`data/version_inventory.json`, which is written by")
    a("`tools/gen_version_inventory.py`.")
    a("")
    a("Each feature bullet carries a provenance marker:")
    a("")
    a("- **[PDF]** -- verified against the vendored specification PDF")
    a("  `docs/khronos/GLSLangSpec.4.60.pdf` (OpenGL Shading Language 4.60.8,")
    a("  2023-08-14). The section/page reference is given in parentheses.")
    a("- **[KNOWLEDGE]** -- **not** verifiable from the vendored PDF. The")
    a("  vendored PDF documents GLSL 4.60 only; its \"Changes\" section")
    a("  (sec. 1.1) reaches back to GLSL 4.50 revision 7. Everything about")
    a("  earlier GLSL versions in this document is historical knowledge and")
    a("  has **not** been checked against a primary source in this repository.")
    a("")
    a("This document is a **specification inventory**. It makes **no claim")
    a("about CTS conformance and no claim about runtime behaviour** of any")
    a("implementation.")
    a("")
    a("## GL <-> GLSL anchor table")
    a("")
    a("| GLSL version | provided by OpenGL version |")
    a("| --- | --- |")
    for g in sorted(inventory["glsl"], key=lambda s: tuple(map(int, s.split(".")))):
        a("| %s | %s |" % (g, inventory["glsl"][g]["gl_version"]))
    a("")
    a("OpenGL 1.0 - 1.5 have no GLSL version of their own; GLSL for those")
    a("releases exists only as extensions.")
    a("")
    a("## Per-version detail")
    a("")
    for g in sorted(inventory["glsl"], key=lambda s: tuple(map(int, s.split(".")))):
        rec = inventory["glsl"][g]
        a("### GLSL %s" % g)
        a("")
        a("- **Provided by OpenGL %s**" % rec["gl_version"])
        a("- **Profiles**: %s" % rec["profiles"])
        a("- **Shader stages available**: %s"
          % (", ".join(rec["stages"]) if rec["stages"] else "none"))
        a("- **Major language features**:")
        a("")
        for f in rec["features"]:
            ref = " (%s)" % f["reference"] if f["reference"] else ""
            tag = "PDF" if f["provenance"] == "pdf" else "KNOWLEDGE"
            a("  - **[%s]** %s%s" % (tag, f["text"], ref))
        a("")
    a("## Stage availability at a glance")
    a("")
    all_stages = []
    for g in sorted(inventory["glsl"], key=lambda s: tuple(map(int, s.split(".")))):
        for s in inventory["glsl"][g]["stages"]:
            if s not in all_stages:
                all_stages.append(s)
    a("| GLSL version | " + " | ".join(all_stages) + " |")
    a("| --- |" + " --- |" * len(all_stages))
    for g in sorted(inventory["glsl"], key=lambda s: tuple(map(int, s.split(".")))):
        stages = inventory["glsl"][g]["stages"]
        cells = ["yes" if s in stages else "--" for s in all_stages]
        a("| %s | %s |" % (g, " | ".join(cells)))
    a("")
    a("Stage list and the six-stage pipeline wording are **[PDF]**-verified")
    a("(`docs/khronos/GLSLangSpec.4.60.pdf`, Chapter 2, p.11): \"Currently,")
    a("these processors are the vertex, tessellation control, tessellation")
    a("evaluation, geometry, fragment, and compute processors.\"")
    a("")
    return "\n".join(out) + "\n"


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def _write_json(path, inventory):
    p = pathlib.Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    text = json.dumps(inventory, sort_keys=True, indent=2, ensure_ascii=True)
    p.write_text(text + "\n", encoding="utf-8")
    return p


def _load_json(path):
    with open(path, "r", encoding="utf-8") as fh:
        return json.load(fh)


def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Generate the desktop OpenGL 1.0-4.6 version/profile "
                    "inventory from the Khronos registry.")
    ap.add_argument("--out", metavar="PATH",
                    help="write the JSON inventory to PATH")
    ap.add_argument("--glxml", metavar="PATH",
                    help="use this registry file instead of docs/khronos/gl.xml")
    ap.add_argument("--check", metavar="PATH",
                    help="verify that PATH already holds the up-to-date JSON")
    ap.add_argument("--docs-from", metavar="PATH", dest="docs_from",
                    help="render the Markdown matrices from an existing JSON "
                         "inventory instead of rebuilding it")
    ap.add_argument("--gl-doc", metavar="PATH", dest="gl_doc",
                    help="path of the OpenGL version matrix Markdown")
    ap.add_argument("--glsl-doc", metavar="PATH", dest="glsl_doc",
                    help="path of the GLSL version matrix Markdown")
    ap.add_argument("--quiet", action="store_true",
                    help="do not print the summary table")
    args = ap.parse_args(argv)

    if args.docs_from:
        inventory = _load_json(args.docs_from)
        if args.gl_doc:
            p = pathlib.Path(args.gl_doc)
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(render_gl_markdown(inventory), encoding="utf-8")
            if not args.quiet:
                print("wrote %s" % p)
        if args.glsl_doc:
            p = pathlib.Path(args.glsl_doc)
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(render_glsl_markdown(inventory), encoding="utf-8")
            if not args.quiet:
                print("wrote %s" % p)
        if not args.quiet:
            print(format_summary(inventory))
        return 0

    inventory = build_inventory(args.glxml)
    run_self_check(inventory)

    if args.out:
        p = _write_json(args.out, inventory)
        if not args.quiet:
            print("wrote %s (%d bytes)" % (p, p.stat().st_size))

    if args.check:
        p = pathlib.Path(args.check)
        if not p.exists():
            sys.stderr.write("CHECK FAILED: %s does not exist\n" % p)
            return 1
        expected = json.dumps(inventory, sort_keys=True, indent=2,
                              ensure_ascii=True) + "\n"
        actual = p.read_text(encoding="utf-8")
        if actual != expected:
            sys.stderr.write(
                "CHECK FAILED: %s is out of date with the registry\n" % p)
            return 1
        if not args.quiet:
            print("check OK: %s is up to date" % p)

    if not args.quiet:
        print(format_summary(inventory))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
