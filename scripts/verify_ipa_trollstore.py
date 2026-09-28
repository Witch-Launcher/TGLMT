#!/usr/bin/env python3
# verify_ipa_trollstore.py — Kiểm tra IPA unsigned có đủ điều kiện cài qua TrollStore không.
# Fail-fast với message rõ ràng để `make` dừng lại trước khi copy sang iPhone.
# Check các nguyên nhân đã biết của lỗi 181 "Unable to register / Failed to add app to icon cache":
#   - thiếu icon / CFBundleIcons không resolve ra file
#   - thiếu PkgInfo / Info.plist keys bắt buộc
#   - binary sai arch/platform/minOS, bị encrypt, mất bit +x
#   - bundle lẫn __MACOSX/.DS_Store, thiếu shaders
# Dùng: python3 scripts/verify_ipa_trollstore.py <file.ipa> [--binary <path>] [--minos 16.0]
import os
import plistlib
import struct
import subprocess
import sys
import tempfile
import zipfile

REQUIRED_PLIST_KEYS = [
    "CFBundleExecutable",
    "CFBundleIdentifier",
    "CFBundleVersion",
    "CFBundleShortVersionString",
    "CFBundlePackageType",
    "CFBundleDisplayName",
    "CFBundleIcons",
    "MinimumOSVersion",
    "LSRequiresIPhoneOS",
    "UIDeviceFamily",
    "CFBundleSupportedPlatforms",
]
REQUIRED_SHADERS = [
    "fish.vert", "fish.frag",
    "water.vert", "water.frag",
    "hud.vert", "hud.frag",
    "sand.vert", "sand.frag",
    "plant.vert", "plant.frag",
    "bubble.vert", "bubble.frag",
    "bg.vert", "bg.frag",
]

errors = []
warnings = []


def err(msg):
    errors.append(msg)
    print(f"  [FAIL] {msg}")


def warn(msg):
    warnings.append(msg)
    print(f"  [WARN] {msg}")


def ok(msg):
    print(f"  [OK] {msg}")


def run(cmd):
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        return p.returncode, (p.stdout or "") + (p.stderr or "")
    except FileNotFoundError:
        return 127, f"missing tool: {cmd[0]}"
    except Exception as e:  # noqa: BLE001
        return 1, str(e)


def png_size(path):
    # Đọc IHDR của PNG, không cần PIL (chạy được ở mọi máy build).
    try:
        with open(path, "rb") as f:
            head = f.read(33)
        if head[:8] != b"\x89PNG\r\n\x1a\n":
            return None
        w, h = struct.unpack(">II", head[16:24])
        return (w, h)
    except OSError:
        return None


def macho_info(path):
    # Dùng otool -l để lấy platform/minos/sdk/cryptid (chuẩn Apple, khỏi parse Mach-O tay).
    rc, out = run(["otool", "-l", path])
    info = {"platform": None, "minos": None, "sdk": None, "cryptid": None, "has_lc_main": False}
    if rc != 0:
        return None
    for line in out.splitlines():
        s = line.strip()
        if s.startswith("platform "):
            try:
                info["platform"] = int(s.split()[1])
            except ValueError:
                pass
        elif s.startswith("minos "):
            info["minos"] = s.split()[1]
        elif s.startswith("sdk "):
            info["sdk"] = s.split()[1]
        elif s == "cmd LC_ENCRYPTION_INFO_64" or s == "cmd LC_ENCRYPTION_INFO":
            info["_want_cryptid"] = True
        elif s.startswith("cryptid ") and info.pop("_want_cryptid", False):
            try:
                info["cryptid"] = int(s.split()[1])
            except ValueError:
                pass
        elif s == "cmd LC_MAIN":
            info["has_lc_main"] = True
    return info


def ver_tuple(v):
    try:
        return tuple(int(x) for x in str(v).split("."))
    except ValueError:
        return ()


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        print(__doc__)
        return 2
    ipa = sys.argv[1]
    max_minos = "16.0"
    if "--minos" in sys.argv:
        max_minos = sys.argv[sys.argv.index("--minos") + 1]

    print(f"== verify {ipa} ==")
    if not os.path.isfile(ipa):
        err(f"không thấy file IPA: {ipa}")
        return print_summary()

    try:
        z = zipfile.ZipFile(ipa)
    except zipfile.BadZipFile:
        err("file không phải zip hợp lệ")
        return print_summary()
    names = z.namelist()

    # 1. Không lẫn rác macOS (gây lỗi giải nén / đăng ký trên device).
    print("-- bundle cleanliness --")
    junk = [n for n in names if "__MACOSX" in n or n.endswith(".DS_Store")]
    if junk:
        err(f"IPA lẫn rác macOS ({len(junk)} file, vd {junk[0]}). Zip lại sau khi xóa.")
    else:
        ok("không có __MACOSX/.DS_Store")

    # 2. Đúng 1 .app trong Payload/.
    print("-- bundle layout --")
    apps = sorted({n.split("/")[1] for n in names if n.startswith("Payload/") and n.count("/") >= 1 and n.split("/")[1].endswith(".app")})
    if len(apps) != 1:
        err(f"Payload/ phải chứa đúng 1 *.app, thấy: {apps}")
        return print_summary()
    app = f"Payload/{apps[0]}/"
    ok(f"bundle: {apps[0]}")

    tmp = tempfile.mkdtemp(prefix="verify-ipa-")
    z.extractall(tmp)
    appdir = os.path.join(tmp, app)

    # 3. Info.plist: parse được + đủ keys bắt buộc của TrollStore/MobileInstallation.
    print("-- Info.plist --")
    plist_path = os.path.join(appdir, "Info.plist")
    if not os.path.isfile(plist_path):
        err("thiếu Info.plist trong .app (TrollStore báo 172)")
        return print_summary()
    try:
        with open(plist_path, "rb") as f:
            pl = plistlib.load(f)
    except Exception as e:  # noqa: BLE001
        err(f"Info.plist parse lỗi: {e}")
        return print_summary()
    ok("Info.plist parse OK")
    for k in REQUIRED_PLIST_KEYS:
        if k not in pl:
            err(f"Info.plist thiếu key bắt buộc: {k} (nguyên nhân phổ biến của 181)")
        else:
            ok(f"key {k} = {str(pl[k])[:60]}")
    if pl.get("CFBundlePackageType") != "APPL":
        err("CFBundlePackageType phải là APPL")
    bid = pl.get("CFBundleIdentifier", "")
    want_bid = None
    if "--bundle-id" in sys.argv:
        want_bid = sys.argv[sys.argv.index("--bundle-id") + 1]
        if want_bid and bid != want_bid:
            err(f"CFBundleIdentifier={bid} nhưng mong đợi {want_bid}. "
                "Info.plist TRONG bundle thắng build setting PRODUCT_BUNDLE_IDENTIFIER, "
                "nên phải set trực tiếp trong plist khi đóng gói.")
        elif want_bid:
            ok(f"CFBundleIdentifier khớp yêu cầu ({bid})")
    if bid.lower().startswith("com.apple."):
        err(f"CFBundleIdentifier {bid} trùng prefix hệ thống (TrollStore báo 179, nguy cơ bootloop)")
    exe = pl.get("CFBundleExecutable", "")
    minos_plist = str(pl.get("MinimumOSVersion", ""))
    if minos_plist and ver_tuple(minos_plist) > ver_tuple(max_minos):
        warn(f"MinimumOSVersion {minos_plist} cao hơn mốc {max_minos}: iPhone iOS thấp hơn sẽ từ chối đăng ký (181)")

    # 4. Icons: CFBundleIcons phải resolve ra PNG thật ở bundle root (fix lỗi 181 icon cache).
    print("-- icons (TrollStore 181) --")
    for section in ("CFBundleIcons", "CFBundleIcons~ipad"):
        d = pl.get(section)
        if not isinstance(d, dict):
            err(f"thiếu section {section} (icon cache cần)")
            continue
        primary = d.get("CFBundlePrimaryIcon", {})
        files = primary.get("CFBundleIconFiles", [])
        if not files:
            err(f"{section}: CFBundleIconFiles rỗng")
            continue
        for base in files:
            # Icon có thể là .png, .tiff (Xcode hay convert sang tiff), hoặc không
            # đuôi (Icon). Tìm theo prefix + scale @2x/@3x + đuôi trong {png,tiff,...}.
            # iOS tự resolve AppIcon60x60 -> AppIcon60x60@2x.png/@3x.png.
            suffixes = (".png", ".PNG", ".tiff", ".TIFF", ".jpg", ".JPG", "")

            def icon_match(fname):
                if not fname.startswith(base):
                    return False
                rest = fname[len(base):]
                if rest.startswith("@"):
                    # scale suffix dạng @2x/@3x đứng trước đuôi file
                    dot = rest.find(".")
                    if dot == -1:
                        return False
                    rest = rest[dot:]
                return rest in suffixes

            cands = [f for f in os.listdir(appdir) if icon_match(f)]
            if not cands:
                err(f"{section}: không tìm thấy icon nào cho base '{base}' ở bundle root")
                continue
            for c in sorted(cands):
                p = os.path.join(appdir, c)
                sz = png_size(p) if c.lower().endswith(".png") else None
                if c.lower().endswith(".png"):
                    if sz is None:
                        err(f"{section}: {c} không phải PNG hợp lệ")
                    else:
                        ok(f"{section}: {c} {sz[0]}x{sz[1]}")
                else:
                    ok(f"{section}: {c} ({os.path.getsize(p)} bytes)")

    # 5. PkgInfo.
    print("-- PkgInfo --")
    pkg = os.path.join(appdir, "PkgInfo")
    if not os.path.isfile(pkg):
        err("thiếu PkgInfo (phải chứa 'APPL????')")
    else:
        with open(pkg, "rb") as f:
            data = f.read()
        if len(data) != 8 or not data.startswith(b"APPL"):
            err(f"PkgInfo sai nội dung: {data!r} (phải là b'APPL????')")
        else:
            ok(f"PkgInfo = {data!r}")

    # 6. Binary: tồn tại, +x, arm64, platform iOS, minos hợp lý, không encrypt.
    print("-- executable --")
    exepath = os.path.join(appdir, exe) if exe else None
    if not exe or not exepath or not os.path.isfile(exepath):
        err(f"CFBundleExecutable '{exe}' không tồn tại (TrollStore báo 174)")
    else:
        ok(f"executable tồn tại: {exe}")
        # bit +x trong zip (TrollStore fix perms lại, nhưng thiếu +x là dấu hiệu đóng gói sai)
        zi = next((i for i in z.infolist() if i.filename == f"{app}{exe}"), None)
        mode = (zi.external_attr >> 16) & 0o777 if zi else 0
        if zi and not (mode & 0o111):
            err(f"executable mất bit +x trong zip (mode {oct(mode)})")
        else:
            ok(f"executable mode trong zip: {oct(mode) if zi else '?'}")
        rc, out = run(["lipo", "-info", exepath])
        if rc != 0 or "arm64" not in out:
            err(f"binary không phải arm64: {out.strip()[:120]}")
        else:
            ok(f"arch: {out.strip()[:100]}")
        mi = macho_info(exepath)
        if mi is None:
            warn("không đọc được otool -l (bỏ qua check platform/minos)")
        else:
            if mi["platform"] != 2:
                err(f"LC_BUILD_VERSION platform={mi['platform']}, phải là 2 (iOS). Build nhầm SDK macOS?")
            else:
                ok(f"platform iOS (2), minos {mi['minos']}, sdk {mi['sdk']}")
            if mi["minos"] and ver_tuple(mi["minos"]) > ver_tuple(max_minos):
                err(f"binary minos {mi['minos']} cao hơn {max_minos}: iPhone iOS thấp hơn từ chối đăng ký (181). "
                    "Build lại với -miphoneos-version-min đúng.")
            if not mi["has_lc_main"]:
                warn("thiếu LC_MAIN (entry point lạ, kiểm tra lại lệnh link)")
            if mi["cryptid"] not in (None, 0):
                err(f"binary bị encrypt (cryptid={mi['cryptid']}, TrollStore báo 180). Dùng bản decrypt.")
            else:
                ok("binary không encrypt (cryptid 0)")
            # LS so MinimumOSVersion trong plist với iOS máy. Lệch 2 nơi này là
            # nguyên nhân 181 kinh điển mà verify plist-only bỏ sót.
            if mi["minos"] and minos_plist and mi["minos"] != minos_plist:
                err(f"MinimumOSVersion={minos_plist} (plist) khác minos={mi['minos']} (binary). "
                    "LaunchServices từ chối bundle → TrollStore 181. Phải đồng bộ (build_ipa_trollstore.sh tự set).")
            elif mi["minos"] and minos_plist:
                ok(f"MinimumOSVersion khớp minos binary ({mi['minos']})")
            # SDK mới hơn OS máy vẫn chạy được, nhưng nếu plist khai OS thấp hơn
            # binary thì iPhone cũ không chạy nổi → cảnh báo rõ.
            if mi["minos"] and ver_tuple(mi["minos"]) > ver_tuple(minos_plist or mi["minos"]):
                warn(f"binary minos {mi['minos']} CAO hơn MinimumOSVersion {minos_plist}: "
                     "máy iOS thấp sẽ cài được nhưng app không chạy.")
        rc, out = run(["codesign", "-dv", exepath])
        if "code object is not signed" in out:
            ok("binary unsigned (đúng: TrollStore tự ldid trên máy)")
        else:
            warn(f"binary đã có chữ ký trước khi cài: {out.strip()[:150]} (nên để unsigned cho TrollStore ldid)")

    # 7. Shaders (runtime, không gây 181 nhưng gây màn hình đen/Init FAIL).
    print("-- shaders --")
    missing = [s for s in REQUIRED_SHADERS if not os.path.isfile(os.path.join(appdir, "shaders", s))]
    if missing:
        # fallback cũ: Resources/shaders
        missing2 = [s for s in missing if not os.path.isfile(os.path.join(appdir, "Resources", "shaders", s))]
        if missing2:
            err(f"thiếu shaders: {missing2[:4]}... (app sẽ Init FAIL)")
        else:
            warn("shaders chỉ có ở Resources/shaders (bản mới cần shaders/ ở root). ios_shell.mm đã fallback, nhưng nên đóng lại IPA.")
    else:
        ok(f"đủ {len(REQUIRED_SHADERS)} shaders ở shaders/")

    return print_summary()


def print_summary():
    print("== kết quả ==")
    if errors:
        print(f"FAIL: {len(errors)} lỗi, {len(warnings)} cảnh báo. Sửa xong chạy lại verify trước khi cài.")
        for e in errors:
            print(f"  - {e}")
        return 1
    print(f"PASS: IPA đủ điều kiện TrollStore ({len(warnings)} cảnh báo).")
    for w in warnings:
        print(f"  - {w}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
