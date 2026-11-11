#!/usr/bin/env python3
"""So sánh hai tập kết quả benchmark (baseline vs optimized) và xuất bảng.

Dùng:
    python3 benchmarks/compare/bench_compare.py baseline/compilation.json \
                                           optimized/compilation.json
    python3 benchmarks/compare/bench_compare.py --dir benchmarks/baseline \
                                           --dir benchmarks/optimized

Quy tắc:
  • Chỉ so các metric có ở CẢ HAI bên (thiếu ở một bên ⇒ báo "MISSING", không
    coi là hồi quy).
  • Cải thiện = giá trị MỚI nhỏ hơn (thời gian càng nhỏ càng tốt).
  • Ngưỡng hồi quy đáng báo: > 2% (có thể chỉnh bằng --regress-threshold).
"""
import argparse
import json
import pathlib
import sys


def flatten(obj, prefix=""):
    """{name: value} phẳng cho mọi metric có 'median_*'."""
    out = {}
    if isinstance(obj, dict):
        for k, v in obj.items():
            key = f"{prefix}{k}"
            if isinstance(v, dict):
                out.update(flatten(v, key + "."))
            elif isinstance(v, (int, float)):
                out[key] = v
    return out


def load(path):
    with open(path) as f:
        return json.load(f)


def metric_pairs_stat(doc, stat_prefix):
    """Như metric_pairs nhưng lấy một thống kê khác ("median_" hoặc "min_").

    `min_` là thống kê robust nhất với nhiễu hệ thống: lần chạy nhanh nhất
    là lần chạy ít bị nhiễu nhất.
    """
    out = {}

    def walk(node, path):
        if not isinstance(node, dict):
            return
        for key, val in node.items():
            if key.startswith("_"):
                continue
            if isinstance(val, dict):
                meds = [(k, v) for k, v in val.items()
                       if k.startswith(stat_prefix) and isinstance(v, (int, float))]
                if meds:
                    for stat_name, stat_val in meds:
                        unit = stat_name[len(stat_prefix):]
                        out[".".join(path + [key, unit])] = stat_val
                else:
                    walk(val, path + [key])

    for container in ("stages", "metrics"):
        sec = doc.get(container)
        if isinstance(sec, dict):
            walk(sec, [])
    return out


def metric_pairs(doc):
    """Trả về {tên đầy đủ: giá trị median} cho mọi metric đo được.

    Duyệt đệ quy: bất kỳ dict nào chứa khoá "median_*" là một metric; tên
    đầy đủ là đường dẫn tới nó.  Cùng một hàm dùng được cho cả
    benchmarks/compilation ({"stages": {stage: {metric: {median_ms: …}}}}),
    benchmarks/runtime và benchmarks/upload ({"metrics": {metric: {median_us: …}}}),
    cũng dùng cho bản đã gộp nhiều lần chạy (bench_median.py).
    """
    out = {}

    def walk(node, path):
        if not isinstance(node, dict):
            return
        for key, val in node.items():
            if key.startswith("_"):
                continue
            if isinstance(val, dict):
                med = [(k, v) for k, v in val.items()
                       if k.startswith("median_") and isinstance(v, (int, float))]
                if med:
                    for stat_name, stat_val in med:
                        unit = stat_name[len("median_"):]
                        out[".".join(path + [key, unit])] = stat_val
                else:
                    walk(val, path + [key])

    for container in ("stages", "metrics"):
        sec = doc.get(container)
        if isinstance(sec, dict):
            walk(sec, [])
    return out


def pct(old, new):
    if old == 0:
        return 0.0
    return (new - old) / old * 100.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("baseline", nargs="?", type=pathlib.Path)
    ap.add_argument("optimized", nargs="?", type=pathlib.Path)
    ap.add_argument("--regress-threshold", type=float, default=2.0)
    ap.add_argument("--control", nargs="*", default=[],
                    help="tên metric KHÔNG nên thay đổi (đường code không bị "
                         "sửa). Chúng đo nhiễu nền của phép đo.")
    args = ap.parse_args()

    if not args.baseline or not args.optimized:
        ap.error("cần <baseline.json> <optimized.json>")

    b = metric_pairs(load(args.baseline))
    o = metric_pairs(load(args.optimized))
    bmin = metric_pairs_stat(load(args.baseline), "min_")
    omin = metric_pairs_stat(load(args.optimized), "min_")

    rows = []
    for key in sorted(set(b) | set(o)):
        if key not in b:
            rows.append((key, None, o[key], None, "MISSING-BASELINE"))
        elif key not in o:
            rows.append((key, b[key], None, None, "MISSING-OPTIMIZED"))
        else:
            delta = pct(b[key], o[key])
            verdict = "SAME"
            if delta < -args.regress_threshold:
                verdict = "IMPROVED"
            elif delta > args.regress_threshold:
                verdict = "REGRESSED"
            rows.append((key, b[key], o[key], delta, verdict))

    print(f"{'metric':<46} {'baseline':>12} {'optimized':>12} {'delta%':>9}  {'min-base':>10}  verdict")
    print("-" * 118)
    regressions = 0
    improvements = 0
    for key, bv, ov, delta, verdict in rows:
        mv = ""
        if key in bmin and key in omin:
            md = pct(bmin[key], omin[key])
            mv = f"{md:+7.2f}%"
        if bv is None:
            print(f"{key:<46} {'-':>12} {ov:>12.4f} {'-':>9}  {mv:>10}  {verdict}")
        elif ov is None:
            print(f"{key:<46} {bv:>12.4f} {'-':>12} {'-':>9}  {mv:>10}  {verdict}")
        else:
            print(f"{key:<46} {bv:>12.4f} {ov:>12.4f} {delta:>8.2f}%  {mv:>10}  {verdict}")
        if verdict == "REGRESSED":
            regressions += 1
        elif verdict == "IMPROVED":
            improvements += 1
    print("-" * 100)
    print(f"improved={improvements} regressed={regressions} total={len(rows)} "
          f"(ngưỡng hồi quy {args.regress_threshold}%)")

    # Nhiễu nền: các metric CONTROL (đường code KHÔNG bị sửa) phải nằm trong
    # một dải hẹp. Nếu không, phép đo không đáng tin và mọi kết luận về
    # "cải thiện" cũng không đáng tin.
    if args.control:
        ctrl = [(k, bv, ov, d) for (k, bv, ov, d, _) in rows
                if any(c in k for c in args.control) and bv is not None and ov is not None]
        if ctrl:
            worst = max(ctrl, key=lambda r: abs(r[3]))
            print()
            print("CONTROL metrics (code path KHÔNG đổi) — dải nhiễu nền của phép đo:")
            for k, bv, ov, d in ctrl:
                print(f"  {k:<44} {bv:>12.4f} {ov:>12.4f} {d:>8.2f}%")
            print(f"  → nhiễu nền tối đa: {abs(worst[3]):.2f}%  ({worst[0]})")
            print("  → mọi metric vượt quá ngưỡng này phải được coi là chưa chứng minh.")
    return 1 if regressions else 0


if __name__ == "__main__":
    sys.exit(main())
