#!/usr/bin/env python3
"""Reproduce vendor warnings against checked-out sources with ASan/UBSan.

Kvazaar compares generic and AVX2 angular prediction on an AVX2 Linux host.
The msdf case checks supported image types and rejection before file truncation;
the current vendor revision deliberately fails that regression. Source overrides
allow testing temporary patches without modifying either submodule.

Exit codes: 0 passed, 77 unavailable/partially skipped, other nonzero failed.
This diagnostic is separate from the normal passing test suite.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
KV_DEFAULT = ROOT / "external/kvazaar/src/strategies/avx2/intra-avx2.c"
MSDF_DEFAULT = ROOT / "external/msdf-atlas-gen/msdf-atlas-gen/json-export.cpp"
SKIP = 77


def run(cmd: list[str], *, cwd: Path | None = None, env=None) -> subprocess.CompletedProcess[str]:
    print("$ " + shlex.join(cmd))
    merged_env = os.environ.copy()
    merged_env.update(env or {})
    merged_env["ASAN_OPTIONS"] = "halt_on_error=1:detect_leaks=1"
    merged_env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    try:
        return subprocess.run(cmd, cwd=cwd, env=merged_env, text=True, capture_output=True, timeout=60)
    except subprocess.TimeoutExpired as exc:
        stdout = exc.stdout.decode(errors="replace") if isinstance(exc.stdout, bytes) else (exc.stdout or "")
        stderr = exc.stderr.decode(errors="replace") if isinstance(exc.stderr, bytes) else (exc.stderr or "")
        return subprocess.CompletedProcess(cmd, 124, stdout, stderr + "timeout after 60 seconds\n")


def report(p: subprocess.CompletedProcess[str]) -> None:
    if p.stdout:
        print(p.stdout, end="")
    if p.stderr:
        print(p.stderr, end="", file=sys.stderr)


def compiler_info(compiler: str) -> None:
    p = run([compiler, "--version"])
    report(p)
    if p.returncode:
        raise RuntimeError(f"cannot execute {compiler}")


def provenance() -> None:
    for name in ("external/kvazaar", "external/msdf-atlas-gen"):
        p = run(["git", "-c", f"safe.directory={ROOT / name}", "-C", str(ROOT / name), "rev-parse", "HEAD"])
        report(p)
        if p.returncode:
            raise RuntimeError(f"cannot inspect {name} submodule")


def kvazaar_case(cc: str, source: Path) -> int:
    if not shutil.which(cc):
        print(f"SKIP: C compiler not found: {cc}")
        return SKIP
    with tempfile.TemporaryDirectory(prefix="datoviz-kvazaar-") as td:
        d = Path(td)
        root = ROOT / "external/kvazaar"
        generic = root / "src/strategies/generic/intra-generic.c"
        wrap = d / "kvazaar_wrap.c"
        harness = d / "harness.c"
        wrap.write_text(f'''#include "{generic}"
void harness_generic(const int_fast8_t w, const int_fast8_t mode,
                     const kvz_pixel *a, const kvz_pixel *l, kvz_pixel *out) {{
  kvz_angular_pred_generic(w, mode, a, l, out);
}}
#include "{source}"
void harness_avx2(const int_fast8_t w, const int_fast8_t mode,
                  const kvz_pixel *a, const kvz_pixel *l, kvz_pixel *out) {{
  kvz_angular_pred_avx2(w, mode, a, l, out);
}}
''')
        harness.write_text(r'''#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kvazaar.h"
void harness_generic(int_fast8_t, int_fast8_t, const kvz_pixel *, const kvz_pixel *, kvz_pixel *);
void harness_avx2(int_fast8_t, int_fast8_t, const kvz_pixel *, const kvz_pixel *, kvz_pixel *);
int main(void) {
  const int logs[] = {2, 3, 4, 5}; unsigned seed = 0x9e3779b9;
  for (unsigned trial = 0; trial < 1000; ++trial) for (unsigned wi = 0; wi < 4; ++wi) {
    int w = 1 << logs[wi];
    kvz_pixel *as = malloc(2*w+67), *ls = malloc(2*w+67), *a = NULL, *b = NULL;
    posix_memalign((void **)&a, 32, w*w); posix_memalign((void **)&b, 32, w*w);
    if (!as || !ls || !a || !b) return 2;
    kvz_pixel *above = as + 32, *left = ls + 32;
    for (int i = -1; i <= 2*w; ++i) { seed=seed*1664525u+1013904223u; above[i+1]=seed>>24; seed=seed*1664525u+1013904223u; left[i+1]=seed>>24; }
    for (int mode = 2; mode <= 34; ++mode) {
      memset(a, 0xa5, w*w); memset(b, 0x5a, w*w);
      harness_generic(logs[wi], mode, above+1, left+1, a); harness_avx2(logs[wi], mode, above+1, left+1, b);
      if (memcmp(a, b, w*w)) { fprintf(stderr, "mismatch width=%d mode=%d\n", w, mode); return 1; }
    }
    free(as); free(ls); free(a); free(b);
  }
  puts("kvazaar: 132000 generic/AVX2 cases passed"); return 0;
}
''')
        obj = d / "kvazaar.o"
        exe = d / "kvazaar-harness"
        cflags = [f"-I{root / 'src'}", "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-ffunction-sections", "-DCOMPILE_INTEL_AVX2=1", "-mavx2"]
        try:
            cpuinfo = Path("/proc/cpuinfo").read_text().lower()
        except OSError as exc:
            print(f"SKIP: cannot inspect CPU capabilities: {exc}")
            return SKIP
        if "avx2" not in cpuinfo:
            print("SKIP: host CPU has no AVX2 capability")
            return SKIP
        probe = d / "avx2-probe.c"
        probe.write_text("#include <immintrin.h>\nint main(void) { __m256i x = _mm256_setzero_si256(); return _mm256_extract_epi32(x, 0); }\n")
        p = run([cc, "-mavx2", str(probe), "-o", str(d / "avx2-probe")])
        report(p)
        if p.returncode:
            print("SKIP: compiler cannot build AVX2 probe")
            return SKIP
        p = run([cc, *cflags, "-c", str(wrap), "-o", str(obj)])
        report(p)
        if p.returncode:
            return p.returncode
        p = run([cc, f"-I{root / 'src'}", "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", str(harness), str(obj), "-Wl,--gc-sections", "-o", str(exe)])
        report(p)
        if p.returncode:
            return p.returncode
        p = run([str(exe)])
        report(p)
        if p.returncode < 0 and abs(p.returncode) == 4:
            print("SKIP: CPU rejected AVX2 execution")
            return SKIP
        return p.returncode


def msdf_case(cxx: str, source: Path) -> int:
    if not shutil.which(cxx):
        print(f"SKIP: C++ compiler not found: {cxx}")
        return SKIP
    # Keep includes anchored to the checkout even when --msdf-source points at a temporary patched copy.
    atlas = ROOT / "external/msdf-atlas-gen"
    with tempfile.TemporaryDirectory(prefix="datoviz-msdf-") as td:
        d = Path(td); wrapper = d / "msdf_harness.cpp"; exe = d / "msdf-harness"
        wrapper.write_text(f'''#include <cstring>
#include "{source}"
namespace msdfgen {{ GlyphIndex::GlyphIndex(unsigned value) : index(value) {{}} }}
namespace msdf_atlas {{
FontGeometry::GlyphRange::GlyphRange() : glyphs(), rangeStart(), rangeEnd() {{}}
size_t FontGeometry::GlyphRange::size() const {{ return 0; }} bool FontGeometry::GlyphRange::empty() const {{ return true; }}
const GlyphGeometry *FontGeometry::GlyphRange::begin() const {{ return nullptr; }} const GlyphGeometry *FontGeometry::GlyphRange::end() const {{ return nullptr; }}
const char *FontGeometry::getName() const {{ return nullptr; }} const msdfgen::FontMetrics &FontGeometry::getMetrics() const {{ static msdfgen::FontMetrics m = {{}}; return m; }}
GlyphIdentifierType FontGeometry::getPreferredIdentifierType() const {{ return GlyphIdentifierType::UNICODE_CODEPOINT; }}
FontGeometry::GlyphRange FontGeometry::getGlyphs() const {{ return GlyphRange(); }}
const std::map<std::pair<int, int>, double> &FontGeometry::getKerning() const {{ static std::map<std::pair<int, int>, double> k; return k; }}
const GlyphGeometry *FontGeometry::getGlyph(msdfgen::GlyphIndex) const {{ return nullptr; }}
}}
int main() {{
  msdf_atlas::JsonAtlasMetrics m = {{}};
  const char *names[] = {{"hardmask", "softmask", "sdf", "psdf", "msdf", "mtsdf"}};
  for (int value = 0; value <= 5; ++value) {{
    const char *supported = "{d / 'supported.json'}";
    if (!msdf_atlas::exportJSON(nullptr, 0, static_cast<msdf_atlas::ImageType>(value), m, supported, false)) return 3;
    FILE *sf = fopen(supported, "rb"); char sbuf[128] = {{}}; size_t sn = fread(sbuf, 1, sizeof(sbuf)-1, sf); fclose(sf);
    if (!strstr(sbuf, names[value])) {{ fprintf(stderr, "supported ImageType %d missing JSON name\\n", value); return 4; }}
  }}
  const char *path = "{d / 'sentinel.json'}"; FILE *f = fopen(path, "w"); fputs("SENTINEL", f); fclose(f);
  bool ok = msdf_atlas::exportJSON(nullptr, 0, static_cast<msdf_atlas::ImageType>(6), m, path, false);
  f = fopen(path, "rb"); char buf[32] = {{}}; size_t n = fread(buf, 1, sizeof(buf)-1, f); fclose(f);
  if (ok) {{ fprintf(stderr, "invalid ImageType unexpectedly accepted; file=%.*s\\n", (int)n, buf); return 1; }}
  if (n != 8 || memcmp(buf, "SENTINEL", 8)) {{ fprintf(stderr, "invalid ImageType touched output file\\n"); return 2; }}
  puts("msdf: invalid ImageType rejected before output file access"); return 0;
}}
''')
        flags = ["-DMSDFGEN_PUBLIC=", f"-I{atlas}", f"-I{atlas / 'msdfgen'}", f"-I{atlas / 'msdf-atlas-gen'}", "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        p = run([cxx, *flags, str(wrapper), "-o", str(exe)])
        report(p)
        if p.returncode:
            return p.returncode
        p = run([str(exe)])
        report(p)
        return p.returncode


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--case", choices=("kvazaar", "msdf", "all"), default="all")
    ap.add_argument("--cc", default=os.environ.get("CC", "gcc"))
    ap.add_argument("--cxx", default=os.environ.get("CXX", "g++"))
    ap.add_argument("--kvazaar-source", type=Path, default=KV_DEFAULT)
    ap.add_argument("--msdf-source", type=Path, default=MSDF_DEFAULT)
    args = ap.parse_args()
    args.kvazaar_source = args.kvazaar_source.resolve()
    args.msdf_source = args.msdf_source.resolve()
    results = []
    if args.case in ("kvazaar", "all"):
        if not shutil.which(args.cc):
            print(f"SKIP: C compiler not found: {args.cc}")
            results.append(SKIP)
        else:
            compiler_info(args.cc)
            provenance()
            results.append(kvazaar_case(args.cc, args.kvazaar_source))
    if args.case in ("msdf", "all"):
        if not shutil.which(args.cxx):
            print(f"SKIP: C++ compiler not found: {args.cxx}")
            results.append(SKIP)
        else:
            compiler_info(args.cxx)
            provenance()
            results.append(msdf_case(args.cxx, args.msdf_source))
    failure = next((r for r in results if r not in (0, SKIP)), None)
    if failure is not None:
        return failure
    return SKIP if SKIP in results else 0


if __name__ == "__main__":
    raise SystemExit(main())
