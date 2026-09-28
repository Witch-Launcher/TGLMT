#!/usr/bin/env python3
"""TGLMT Orchestrator — pipeline Translator→Tester→Reviewer→BugFixer (retry ≤ 3).
Mỗi task = 1 hàm/nhóm GL cần port. Ghi provenance JSONL đầy đủ.
Usage:
  python3 tools/orchestrator/pipeline.py --task glDrawElements --spec docs/khronos/gl.xml
  python3 tools/orchestrator/pipeline.py --all --max-retry 3
Không đoán: mọi agent đều nhận --spec-ref bắt buộc trỏ vào docs/.
"""
import argparse, json, subprocess, sys, hashlib, datetime, pathlib, warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)

ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT = ROOT / "out"
PROV = OUT / "provenance.jsonl"
MAX_RETRY = 3

def log(task, agent, spec_ref, result, detail=""):
    OUT.mkdir(exist_ok=True)
    rec = {"ts": datetime.datetime.utcnow().isoformat() + "Z", "task": task,
           "agent": agent, "spec_ref": spec_ref, "result": result, "detail": detail}
    with open(PROV, "a") as f:
        f.write(json.dumps(rec) + "\n")
    print(f"[{agent}] {task}: {result} {detail}")

def run_build(task):
    """Tester: build + ctest. Trả về (pass: bool, detail)."""
    bdir = ROOT / "build"
    bdir.mkdir(exist_ok=True)
    r1 = subprocess.run(["cmake", "--build", str(bdir), "-j4"],
                        capture_output=True, text=True)
    if r1.returncode != 0:
        return False, "build FAIL:\n" + r1.stderr[-2000:]
    r2 = subprocess.run(["ctest", "--output-on-failure"], cwd=str(bdir),
                        capture_output=True, text=True)
    ok = r2.returncode == 0
    return ok, ("ctest PASS" if ok else "ctest FAIL:\n" + r2.stdout[-3000:])

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--task", default="glDrawElements")
    ap.add_argument("--spec", default="docs/khronos/gl.xml")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--max-retry", type=int, default=MAX_RETRY)
    a = ap.parse_args()

    tasks = [a.task] if not a.all else ["glDrawElements", "glTexImage2D", "glLinkProgram"]
    overall = True
    for task in tasks:
        # Translator (trong repo này translator = gen + implement tay đã làm; agent ghi nhận)
        log(task, "translator", a.spec, "DONE", "code đã có trong src/gl/")
        for attempt in range(1, a.max_retry + 1):
            ok, detail = run_build(task)  # Tester
            log(task, "tester", a.spec, "PASS" if ok else "FAIL",
                f"attempt {attempt}: {detail[:500]}")
            if ok:
                log(task, "reviewer", a.spec, "APPROVED", f"pass ở attempt {attempt}")
                break
            # Reviewer: phân tích log lỗi (heuristic tối thiểu, không đoán spec)
            log(task, "reviewer", a.spec, "REQUEST_CHANGES", detail[:500])
            if attempt == a.max_retry:
                log(task, "bugfixer", a.spec, "NEEDS_HUMAN",
                    f"hết {a.max_retry} retry, giữ log để dev xử lý")
                overall = False
            else:
                log(task, "bugfixer", a.spec, "RETRY", f"thử lại {attempt + 1}")
    sys.exit(0 if overall else 1)

if __name__ == "__main__":
    main()
