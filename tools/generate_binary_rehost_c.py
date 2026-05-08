#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


def c_array(data: bytes, width: int = 12) -> str:
    lines: list[str] = []
    for i in range(0, len(data), width):
        chunk = data[i : i + width]
        lines.append("  " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    return "\n".join(lines)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    ap = argparse.ArgumentParser(
        description=(
            "Generate ugly but working C that embeds the known-good EXE and spawns it. "
            "Default output is a *fat* file; the repo keeps a thin launcher at "
            "aeternitas64_binary_rehost.c so this generator does not overwrite it."
        )
    )
    ap.add_argument("--exe", default=str(root / "aeternitas64.exe"))
    ap.add_argument(
        "--out",
        default=str(root / "aeternitas64_binary_rehost_fat.c"),
        help="Output path (default: fat embed beside repo root, does not clobber thin launcher).",
    )
    args = ap.parse_args()

    exe = Path(args.exe).resolve()
    out = Path(args.out).resolve()
    data = exe.read_bytes()
    sha = hashlib.sha256(data).hexdigest()

    source = f'''/*
  Generated from {exe.name}
  SHA256: {sha}

  This is not pretty source recovery. It is a compilable C rehost of the
  known-good executable so the game can be rebuilt from C even when the
  original source tree is gone. It preserves command-line args and inherited
  stdin/stdout/stderr by spawning the extracted payload.
*/
#include <errno.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#error This rehost is for Windows because the recovered payload is a PE executable.
#endif

static const unsigned char AETERNITAS64_EXE[{len(data)}] = {{
{c_array(data)}
}};

static int write_payload(const char *path) {{
  FILE *fp = fopen(path, "wb");
  if (!fp) return -1;
  if (fwrite(AETERNITAS64_EXE, 1, sizeof AETERNITAS64_EXE, fp) != sizeof AETERNITAS64_EXE) {{
    fclose(fp);
    return -2;
  }}
  if (fclose(fp) != 0) return -3;
  return 0;
}}

int main(int argc, char **argv) {{
  char tmp_dir_buf[L_tmpnam + 64];
  char *tmp_name = NULL;
  char **child_argv = NULL;
  int rc;
  int i;

  tmp_name = tmpnam(tmp_dir_buf);
  if (!tmp_name || !tmp_name[0]) {{
    fputs("aeternitas64_rehost: tmpnam failed\\n", stderr);
    return 125;
  }}

  /* Force a Windows executable suffix. */
  {{
    size_t n = strlen(tmp_name);
    if (n + 5 >= sizeof tmp_dir_buf) {{
      fputs("aeternitas64_rehost: temp path too long\\n", stderr);
      return 125;
    }}
    memcpy(tmp_dir_buf + n, ".exe", 5);
  }}

  rc = write_payload(tmp_dir_buf);
  if (rc != 0) {{
    fprintf(stderr, "aeternitas64_rehost: failed to write payload (%d)\\n", rc);
    return 126;
  }}

  child_argv = (char **)calloc((size_t)argc + 1, sizeof(char *));
  if (!child_argv) {{
    remove(tmp_dir_buf);
    fputs("aeternitas64_rehost: out of memory\\n", stderr);
    return 125;
  }}
  child_argv[0] = tmp_dir_buf;
  for (i = 1; i < argc; i++) child_argv[i] = argv[i];
  child_argv[argc] = NULL;

  rc = _spawnv(_P_WAIT, tmp_dir_buf, (const char * const *)child_argv);
  if (rc == -1) {{
    fprintf(stderr, "aeternitas64_rehost: spawn failed: %s\\n", strerror(errno));
    rc = 127;
  }}

  free(child_argv);
  remove(tmp_dir_buf);
  return rc;
}}
'''
    out.write_text(source, encoding="utf-8")
    print(f"Wrote {out}")
    print(f"Payload bytes: {len(data)}")
    print(f"Payload sha256: {sha}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
