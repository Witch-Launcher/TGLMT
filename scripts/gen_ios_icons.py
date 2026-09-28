#!/usr/bin/env python3
# gen_ios_icons.py — Sinh icon PNG cho bundle iOS (TrollStore bắt buộc có icon,
# thiếu là lỗi 181 "Failed to add app to icon cache").
# Idempotent: chỉ sinh file còn thiếu, không ghi đè file đã có.
# Dùng: python3 scripts/gen_ios_icons.py [out_dir]
import os
import sys

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "apps", "aquarium", "ios_assets")

SPECS = [  # (tên file, cạnh px) — khớp CFBundleIconFiles trong ios_Info.plist
    ("AppIcon60x60@2x.png", 120),
    ("AppIcon60x60@3x.png", 180),
    ("AppIcon76x76.png", 76),
    ("AppIcon76x76@2x.png", 152),
    ("AppIcon83.5@2x.png", 167),
    ("Icon.png", 57),
    ("Icon@2x.png", 114),
    ("Icon-60@2x.png", 120),
    ("Icon-60@3x.png", 180),
    ("Icon-76.png", 76),
    ("Icon-76@2x.png", 152),
    ("Icon-Small.png", 29),
    ("Icon-Small@2x.png", 58),
    ("Icon-Small-40@2x.png", 80),
]


def make_icon(size, path):
    from PIL import Image, ImageDraw  # noqa: PLC0415
    img = Image.new("RGB", (size, size), (2, 40, 80))
    d = ImageDraw.Draw(img)
    for y in range(size):  # gradient biển sâu -> teal
        t = y / size
        d.line([(0, y), (size, y)],
               fill=(int(2 + t * 10), int(60 + t * 80), int(120 + t * 80)))
    d.rectangle([0, int(size * 0.82), size, size], fill=(194, 178, 128))  # cát
    cx, cy = size * 0.5, size * 0.48
    rx, ry = size * 0.28, size * 0.15
    lw = max(1, size // 100)
    d.ellipse([cx - rx, cy - ry, cx + rx, cy + ry],
              fill=(255, 140, 0), outline=(200, 80, 0), width=lw)  # thân cá
    d.polygon([(cx - rx, cy), (cx - rx - size * 0.16, cy - size * 0.14),
               (cx - rx - size * 0.16, cy + size * 0.14)],
              fill=(255, 140, 0), outline=(200, 80, 0))  # đuôi
    er = max(2, size // 40)
    d.ellipse([cx + rx * 0.55 - er, cy - ry * 0.3 - er,
               cx + rx * 0.55 + er, cy - ry * 0.3 + er], fill=(255, 255, 255))
    d.ellipse([cx + rx * 0.55 - er // 2, cy - ry * 0.3 - er // 2,
               cx + rx * 0.55 + er // 2, cy - ry * 0.3 + er // 2], fill=(0, 0, 0))
    for bx, by, br in ((0.72, 0.25, 0.035), (0.80, 0.35, 0.025), (0.65, 0.20, 0.020)):
        x, y, r = int(size * bx), int(size * by), int(size * br)
        d.ellipse([x - r, y - r, x + r, y + r],
                  outline=(200, 240, 255), width=max(1, size // 200))  # bọt khí
    img.save(path)
    print(f"  wrote {path} ({size}x{size})")


def main():
    try:
        from PIL import Image  # noqa: F401
    except ImportError:
        print("THIẾU Pillow: pip install pillow  (make check-tools sẽ báo)")
        return 1
    os.makedirs(OUT, exist_ok=True)
    missing = [(n, s) for n, s in SPECS if not os.path.isfile(os.path.join(OUT, n))]
    if not missing:
        print(f"icons đủ ({len(SPECS)} file ở {OUT})")
        return 0
    for name, size in missing:
        make_icon(size, os.path.join(OUT, name))
    return 0


if __name__ == "__main__":
    sys.exit(main())
