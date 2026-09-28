#!/usr/bin/env python3
"""Sinh docs-site/content-{en,vi}.js tu docs/{en,vi}/*.md (nguon chan ly duy nhat).

Bao gom converter markdown->HTML toi thieu (khong phu thuoc ngoai):
headings (+id cho TOC), code fence, bang, list, paragraph,
inline code/bold/link. ASCII diagram nam trong code fence nen giu nguyen.
Usage: python3 tools/gen_docs_site.py
"""
import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "docs-site"

ORDER = ["getting-started", "api", "architecture", "launcher", "limits"]
PAGE_ID = {
    "getting-started": "guide.start",
    "api": "api.ref",
    "architecture": "arch.overview",
    "launcher": "launcher.integration",
    "limits": "limits.ref",
}


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def slug(s):
    s = re.sub(r"<[^>]+>", "", s).lower()
    s = re.sub(r"[^a-z0-9]+", "-", s).strip("-")
    return s or "section"


def inline_md(s):
    s = esc(s)
    s = re.sub(r"`([^`]+)`", r"<code>\1</code>", s)
    s = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", s)
    s = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r'<a href="\2">\1</a>', s)
    return s


def md_to_html(md):
    out = []
    lines = md.split("\n")
    i = 0
    in_code = False
    code_lang = ""
    code_buf = []
    in_list = None  # "ul" | "ol"
    para = []

    def flush_para():
        if para:
            out.append("<p>" + inline_md(" ".join(para)) + "</p>")
            para.clear()

    def close_list():
        nonlocal in_list
        if in_list:
            out.append("</ul>" if in_list == "ul" else "</ol>")
            in_list = None

    def is_table_row(l):
        return l.strip().startswith("|") and l.strip().endswith("|")

    def is_sep_row(l):
        cells = [c.strip() for c in l.strip().strip("|").split("|")]
        return cells and all(re.fullmatch(r":?-{2,}:?", c) for c in cells)

    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        if stripped.startswith("```"):
            if in_code:
                out.append(
                    "<pre><button class=\"copy-btn\" onclick=\"copyCode(this)\">Copy</button>"
                    "<code>" + "\n".join(esc(l) for l in code_buf) + "</code></pre>"
                )
                code_buf = []
                in_code = False
            else:
                flush_para()
                close_list()
                in_code = True
                code_lang = stripped[3:].strip()
            i += 1
            continue
        if in_code:
            code_buf.append(line.rstrip("\n"))
            i += 1
            continue
        m = re.match(r"^(#{1,4})\s+(.*)$", stripped)
        if m:
            flush_para()
            close_list()
            level = len(m.group(1))
            text = m.group(2).strip()
            out.append(f"<h{level} id=\"{slug(text)}\">{inline_md(text)}</h{level}>")
            i += 1
            continue
        if stripped == "---":
            flush_para()
            close_list()
            out.append("<hr>")
            i += 1
            continue
        if stripped.startswith("> "):
            flush_para()
            close_list()
            out.append("<blockquote><p>" + inline_md(stripped[2:]) + "</p></blockquote>")
            i += 1
            continue
        if is_table_row(line) and i + 1 < len(lines) and is_sep_row(lines[i + 1]):
            flush_para()
            close_list()
            head = [c.strip() for c in line.strip().strip("|").split("|")]
            out.append("<table><thead><tr>" + "".join(f"<th>{inline_md(c)}</th>" for c in head) + "</tr></thead><tbody>")
            i += 2
            while i < len(lines) and is_table_row(lines[i]) and not is_sep_row(lines[i]):
                cells = [c.strip() for c in lines[i].strip().strip("|").split("|")]
                out.append("<tr>" + "".join(f"<td>{inline_md(c)}</td>" for c in cells) + "</tr>")
                i += 1
            out.append("</tbody></table>")
            continue
        mlist = re.match(r"^(\s*)[-*]\s+(.*)$", line)
        mold = re.match(r"^(\s*)\d+\.\s+(.*)$", line)
        if mlist or mold:
            flush_para()
            kind = "ul" if mlist else "ol"
            text = (mlist or mold).group(2)
            if in_list != kind:
                close_list()
                out.append("<ul>" if kind == "ul" else "<ol>")
                in_list = kind
            out.append("<li>" + inline_md(text) + "</li>")
            i += 1
            continue
        if not stripped:
            flush_para()
            close_list()
            i += 1
            continue
        close_list()
        para.append(stripped)
        i += 1
    flush_para()
    close_list()
    return "\n".join(out)


def build(lang):
    sections = {}
    for name in ORDER:
        src = ROOT / "docs" / lang / (name + ".md")
        html = md_to_html(src.read_text(encoding="utf-8"))
        sections[name] = {"pages": [{"id": PAGE_ID[name], "content": html}]}
    return sections


def main():
    OUT.mkdir(exist_ok=True)
    for lang in ("en", "vi"):
        sections = build(lang)
        js = "docsData." + lang + ".sections = " + json.dumps(sections, ensure_ascii=False) + ";\n"
        (OUT / f"content-{lang}.js").write_text(js, encoding="utf-8")
        print(f"[docs-site] wrote docs-site/content-{lang}.js ({len(js)//1024} KB)")


if __name__ == "__main__":
    main()
