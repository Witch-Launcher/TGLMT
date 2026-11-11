#!/usr/bin/env python3
"""Gộp N lần chạy benchmark thành một kết quả (median của median).

VÌ SAO CẦN
-----------
Một số metric trong benchmark (đặc biệt `frame_100_draws` và
`managed_staging_plus_blit`) có phương sai lớn: chúng phụ thuộc lịch
trình của GPU và bộ đếm thời gian, nên một lần chạy có thể lệch 10-15 %.
Đo một lần rồi so sánh là **không đáng tin**.

Cách dùng:
    for i in 1 2 3 4 5; do ./bench_runtime > run$i.json; done
    python3 benchmarks/compare/bench_median.py run*.json -o merged.json

Mỗi metric: median của các median của N lần chạy (giữ p95 lớn nhất để
không giấu đuôi).
"""
import argparse
import json
import pathlib
import statistics
import sys


def collect(doc, prefix, out):
    """Gom mọi metric có 'median_*' thành {tên: giá trị median}."""
    def walk(node, path):
        if not isinstance(node, dict):
            return
        for key, val in node.items():
            if key.startswith("_"):
                continue
            if isinstance(val, dict):
                meds = [(k, v) for k, v in val.items()
                        if k.startswith("median_") and isinstance(v, (int, float))]
                if meds:
                    for stat_name, stat_val in meds:
                        unit = stat_name[len("median_"):]
                        out.setdefault(".".join(path + [key, unit]), []).append(stat_val)
                else:
                    walk(val, path + [key])

    for container in ("stages", "metrics"):
        sec = doc.get(container)
        if isinstance(sec, dict):
            walk(sec, prefix)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("inputs", nargs="+", type=pathlib.Path)
    ap.add_argument("-o", "--output", type=pathlib.Path)
    args = ap.parse_args()

    per_metric = {}
    for path in args.inputs:
        doc = json.loads(path.read_text())
        collect(doc, [], per_metric)

    merged = {
        "benchmark": "merged",
        "runs": len(args.inputs),
        "inputs": [str(p) for p in args.inputs],
        "metrics": {},
    }
    for name in sorted(per_metric):
        vals = per_metric[name]
        merged["metrics"][name] = {
            "median_of_medians": statistics.median(vals),
            "min": min(vals),
            "max": max(vals),
            "n_runs": len(vals),
        }
    # Với công cụ so sánh, metric phải ở dạng có "median_*".
    cmp_doc = {"benchmark": "merged", "runs": len(args.inputs), "metrics": {}}
    for name, st in merged["metrics"].items():
        unit = name.rsplit(".", 1)[-1]
        # Tên đã kết thúc bằng đơn vị ("...us") thì không thêm lần nữa.
        base = name[: -(len(unit) + 1)] if name.endswith("." + unit) else name
        cmp_doc["metrics"][base] = {
            f"median_{unit}": st["median_of_medians"],
            f"min_{unit}": st["min"],
            f"max_{unit}": st["max"],
            "n_runs": st["n_runs"],
        }

    text = json.dumps(cmp_doc, indent=2)
    if args.output:
        args.output.write_text(text + "\n")
        print(f"[bench-median] wrote {args.output} ({len(args.inputs)} runs, "
              f"{len(cmp_doc['metrics'])} metrics)")
    else:
        print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
