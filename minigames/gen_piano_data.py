# Regenerates mg_piano_data.c from web index.html (full VP notation per sheet).
# Set AETERNITAS_WEB_INDEX to index.html path if not at the default below.
import os
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
path = Path(
    os.environ.get(
        "AETERNITAS_WEB_INDEX",
        r"D:\light\Veritasfurtum Guides and Docs\AeternitasWebEdition\index.html",
    )
)
out_path = HERE / "mg_piano_data.c"

text = open(path, encoding="utf-8", errors="replace").read()
sheets = [
    ("barkeep_lesson", "The Barkeep's First Lesson", "easy"),
    ("lantern_waltz", "Candlelight Reel of Hollow Ridge", "medium"),
    ("last_call_etude", "Last Call at the Copper Cup", "hard"),
]

notations = []
for sid, title, diff in sheets:
    m = re.search(
        re.escape(sid) + r":\s*\{[^}]*notation:\s*\"((?:\\.|[^\"])*)\"",
        text,
        re.S,
    )
    if not m:
        raise SystemExit("no notation for " + sid)
    raw = m.group(1).encode().decode("unicode_escape")
    notations.append((sid, title, diff, raw))
    print(sid, "len", len(raw))

out = ['#include "mg_piano_data.h"\n', "#include <stddef.h>\n\n"]
out.append("static const char *const k_notation[] = {\n")
for sid, title, diff, raw in notations:
    esc = raw.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n\"\n    \"")
    out.append(f"  /* {sid} */\n  \"{esc}\",\n")
out.append("};\n\n")
out.append("static const char *const k_titles[] = {\n")
for sid, title, diff, raw in notations:
    out.append(f'  "{title}",\n')
out.append("};\n\n")
out.append("static const char *const k_ids[] = {\n")
for sid, title, diff, raw in notations:
    out.append(f'  "{sid}",\n')
out.append("};\n\n")
out.append("static const char *const k_diff[] = {\n")
for sid, title, diff, raw in notations:
    out.append(f'  "{diff}",\n')
out.append("};\n\n")
n = len(sheets)
out.append(f"int mg_piano_sheet_count(void) {{ return {n}; }}\n")
out.append("const char *mg_piano_sheet_id(int i) { return k_ids[i]; }\n")
out.append("const char *mg_piano_sheet_title(int i) { return k_titles[i]; }\n")
out.append("const char *mg_piano_sheet_diff(int i) { return k_diff[i]; }\n")
out.append("const char *mg_piano_sheet_notation(int i) { return k_notation[i]; }\n")

open(out_path, "w", encoding="utf-8").writelines(out)
print("wrote", out_path)
