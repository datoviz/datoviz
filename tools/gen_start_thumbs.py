#!/usr/bin/env python3
"""Generate cropped WebP thumbnails for the Start Here page.

Reads source PNGs from data/gallery/v0.4/, applies per-image crop boxes,
and writes 240×180 WebP files to build/gallery-webp/v0.4/thumbs/.
Skips files whose source PNG is older than the existing thumbnail.

All crop boxes are exact 4:3 (verified) so PIL resize to 240×180 has zero distortion.
Content bounds were measured via pixel analysis of each source PNG.
"""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "data/gallery/v0.4"
OUT = ROOT / "build/gallery-webp/v0.4/thumbs"

THUMB_W, THUMB_H = 240, 180

# (output_stem, source_path_relative_to_SRC, crop_box)
# crop_box = (x1, y1, x2, y2) in source pixels (all sources are 1600×1200)
THUMBS: list[tuple[str, str, tuple[int, int, int, int]]] = [

    # ── Visual families (order matches tools/visuals_catalog.yaml) ────────────

    # 0D
    # pixel: 5x zoom into center patch — individual dots clearly visible
    ("v_pixel",   "visuals/visuals_pixel.png",      (680,  510,  920,  690)),
    # point: spiral galaxy content=(36,134,1504,1072) — tight 4:3
    ("v_point",   "visuals/visuals_point.png",           ( 20,   35, 1520, 1160)),
    # marker: cols 1-3 (x=152-622), rows 1-2 (y=297-624) — 6 symbols, large
    ("v_marker",  "visuals/visuals_marker.png",      (130,  270,  650,  660)),

    # 1D
    # segment: 2x zoom into center cluster
    ("v_segment", "visuals/visuals_segment.png",     (350,  250,  950,  700)),
    # path: 3 wavy lines content=(100,275,1499,932) — center zoom to 4:3
    ("v_path",    "visuals/visuals_path.png",        (303,  230, 1296,  975)),
    # vector: 4x zoom into center field
    ("v_vector",  "visuals/visuals_vector.png",      (600,  450, 1000,  750)),

    # 2D
    # image: blue texture — trim dark border
    ("v_image",   "visuals/visuals_image.png",       (100,  100, 1500, 1100)),
    # glyph: tight crop on the text
    ("v_glyph",   "visuals/visuals_glyph.png",       ( 50,  282,  850,  882)),
    # text: content=(134,183,1263,745) — left text block
    ("v_text",    "visuals/visuals_text.png",        (100,  155,  920,  770)),
    # primitive: colorful triangles content=(33,108,1535,1151) — tight 4:3
    ("v_primitive","visuals/visuals_primitive.png",  ( 50,   75, 1550, 1200)),

    # 3D
    # mesh: cube content=(362,82,1256,1078) — keep whole shape
    ("v_mesh",    "visuals/visuals_mesh.png",        (150,   20, 1350,  920)),
    # volume: sphere content=(268,1,1365,1199) — keep whole shape
    ("v_volume",  "visuals/visuals_volume.png",             (200,   50, 1400,  950)),
    # sphere: 2x zoom into central cluster
    ("v_sphere",  "visuals/visuals_sphere.png",    (500,  325, 1100,  775)),
    # splat: 2x zoom into central swirl
    ("v_splat",   "visuals/visuals_splat.png",       (527,  395, 1127,  845)),

    # ── "I want to…" grid ─────────────────────────────────────────────────────
    ("f_panzoom",  "features/features_axes_2d.png",               (200,  100, 1400, 1000)),
    ("f_arcball",  "features/features_controller_arcball.png", (200,  150, 1400, 1050)),
    ("f_colorbar", "features/features_colorbar.png",                   ( 50,  100, 1550, 1225)),
    ("f_axes",     "features/features_axis_labels.png",        (150,   60, 1350,  960)),
    # pick: selection_sphere — shows highlighted spheres in 3D scene
    ("f_pick",     "features/features_selection_sphere.png",   (200,  100, 1400, 1000)),
    # panels: 4-panel grid
    ("f_panels",   "features/features_panel_grid.png",         (100,   75, 1500, 1125)),
    ("f_realtime", "features/features_timer_animation.png",    ( 50,  300, 1450, 1125)),
    ("f_capture",  "start/start_scatter.png",                 (200,  150, 1400, 1050)),
]


def needs_update(png: Path, webp: Path, force: bool = False) -> bool:
    if force or not webp.exists():
        return True
    return png.stat().st_mtime_ns > webp.stat().st_mtime_ns


def generate(force: bool = False) -> None:
    try:
        from PIL import Image
    except ImportError:
        print("gen_start_thumbs: Pillow not available, skipping thumbnail generation")
        return

    if not SRC.exists():
        print(f"gen_start_thumbs: source PNG dir not found ({SRC}), skipping")
        return

    OUT.mkdir(parents=True, exist_ok=True)
    updated = 0
    skipped = 0

    for name, rel_src, box in THUMBS:
        png = SRC / rel_src
        webp = OUT / f"{name}.webp"

        if not png.exists():
            print(f"gen_start_thumbs: missing source {rel_src}, skipping {name}")
            continue

        if not needs_update(png, webp, force):
            skipped += 1
            continue

        img = Image.open(png).crop(box).resize((THUMB_W, THUMB_H), Image.LANCZOS)
        img.save(webp, "webp", quality=88)
        updated += 1

    print(f"gen_start_thumbs: updated={updated} skipped={skipped}")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="regenerate even when up to date")
    args = parser.parse_args()
    generate(force=args.force)
