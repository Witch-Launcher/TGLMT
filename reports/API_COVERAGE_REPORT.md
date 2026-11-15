# API_COVERAGE_REPORT.md — Coverage OpenGL 1.0–4.6 (Core + Compatibility)

> **Mọi số liệu đo bằng script**, không viết tay:
> `python3 tools/gen_coverage_report.py`
> (nguồn: `data/version_inventory.json` — sinh từ `docs/khronos/gl.xml` —
> và `src/gl/gl_stubs_gen.cpp` — danh sách lệnh còn stub).

**Nhãn môi trường:** `REFERENCE_ONLY` (backend Null) — **KHÔNG** phải
`A11_DEVICE`. Xem `reports/FULL_AUDIT_REPORT.md` §0.

## 0. Cách đo

| Khái niệm | Định nghĩa |
|---|---|
| Lệnh **có ở version V** (core) | `core_available` && `introduced_version <= V` |
| Lệnh **có ở version V** (compat) | `introduced_version <= V` && chưa bị gỡ khỏi compatibility ở version <= V |
| Lệnh **đã implement** | không còn xuất hiện trong `src/gl/gl_stubs_gen.cpp` |

## 1. Coverage theo version × profile

| Version | Core | % | Compatibility | % |
|---|---|---|---|---|
| 1.0 | 48/48 | 100.0% | 306/306 | 100.0% |
| 1.1 | 62/62 | 100.0% | 336/336 | 100.0% |
| 1.2 | 66/66 | 100.0% | 340/340 | 100.0% |
| 1.3 | 75/75 | 100.0% | 386/386 | 100.0% |
| 1.4 | 84/84 | 100.0% | 433/433 | 100.0% |
| 1.5 | 103/103 | 100.0% | 452/452 | 100.0% |
| 2.0 | 196/196 | 100.0% | 545/545 | 100.0% |
| 2.1 | 202/202 | 100.0% | 551/551 | 100.0% |
| 3.0 | 286/286 | 100.0% | 635/635 | 100.0% |
| 3.1 | 298/298 | 100.0% | 647/647 | 100.0% |
| 3.2 | 317/317 | 100.0% | 666/666 | 100.0% |
| 3.3 | 345/345 | 100.0% | 724/724 | 100.0% |
| 4.0 | 391/391 | 100.0% | 770/770 | 100.0% |
| 4.1 | 479/479 | 100.0% | 858/858 | 100.0% |
| 4.2 | 491/491 | 100.0% | 870/870 | 100.0% |
| 4.3 | 534/534 | 100.0% | 913/913 | 100.0% |
| 4.4 | 543/543 | 100.0% | 922/922 | 100.0% |
| 4.5 | 653/653 | 100.0% | 1044/1044 | 100.0% |
| 4.6 | 657/657 | 100.0% | 1048/1048 | 100.0% |

**Tổng:** core **5830/5830** (100.0%) · compatibility **12446/12446** (100.0%)

**Lệnh còn stub: 0** (danh sách rỗng ⇒ không còn stub nào)

## 2. Test

`ctest -j8` → **207/207 PASS** (đọc từ log `ctest -V`).

## 3. Phạm vi

- Báo cáo này đo **bề mặt API** (lệnh nào có và đã implement).
- Trạng thái **hành vi** (đúng spec hay chưa) nằm ở
  `reports/LEGACY_COVERAGE_REPORT.md` và `reports/FULL_AUDIT_REPORT.md`.
- Phần chưa xác minh trên **Apple A11** vẫn là `NOT_VERIFIED` — xem
  `PROGRESS.md`.
