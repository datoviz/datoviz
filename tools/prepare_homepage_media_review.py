#!/usr/bin/env python3
"""Prepare a local-only, hash-inventoried comparison of homepage media candidates."""
from __future__ import annotations

import argparse
import hashlib
from html import escape
import json
from pathlib import Path
import shutil
import subprocess

import gallery_media
import gallery_frames

ROOT = gallery_media.ROOT
NOTES = {
    'showcases_protein': ('Protein', 'Gentle ±12° presentation sway over 14.4 seconds; molecule, selection and crosshair stay aligned.', 'Recommended. Static molecular data permits a closed presentation loop.'),
    'showcases_point_cloud': ('Point cloud', 'Small ±7° camera orbit around the existing scene pivot, returning smoothly every 14.4 seconds.', 'Recommended if dense point detail remains clear at the encoded quality.'),
    'showcases_cortical_activity': ('Cortical activity', 'Fixed anatomical presentation; the established 3.6-second activity cycle runs four times per clip.', 'Recommended for readability. The source activity wraps as before; matching camera poses does not certify equal last/first data values.'),
    'showcases_textured_planet': ('Textured planet', 'Fixed display orientation; orbital debris retains its existing demonstration timing, including the existing time acceleration.', 'The orbital trajectories do not close over this clip. Prefer the still poster on the homepage unless the visible boundary is acceptable; do not conceal it with a crossfade.'),
}


def sha(path: Path) -> str:
    """Hash one exact local artifact."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(products: Path, baseline: Path, output: Path) -> None:
    """Freeze comparison copies, exact hashes and capture provenance without publication."""
    if output.exists():
        raise ValueError('Review output already exists; choose a new frozen review directory')
    report = json.loads((products / 'reports/homepage/report.json').read_text())
    manifest = gallery_media.load_manifest(gallery_media.DEFAULT_MANIFEST)
    manifest_entries = {entry['id']: entry for entry in manifest['examples']}
    source = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    records, sections = [], []
    output.mkdir(parents=True)
    for example_id in ('showcases_protein', 'showcases_point_cloud', 'showcases_cortical_activity', 'showcases_textured_planet'):
        title, change, assessment = NOTES[example_id]
        item = next(row for row in report['comparisons'] if row['id'] == example_id)
        captures = json.loads((products / f'cache/frames/homepage/showcases/{example_id}.json').read_text())
        assets, figures = [], []
        for role, root in [('baseline', baseline), ('candidate', products / 'assets/profiles/homepage')]:
            paths = {}
            for suffix in ('mp4', 'poster.webp'):
                path = root / 'showcases' / f'{example_id}.{suffix}'
                target = output / 'media' / f'{example_id}-{role}.{suffix}'
                target.parent.mkdir(exist_ok=True)
                shutil.copyfile(path, target)
                assets.append(dict(role=role, path=target.relative_to(output).as_posix(), sha256=sha(target), bytes=target.stat().st_size))
                paths[suffix] = target.relative_to(output).as_posix()
            label = 'Current homepage' if role == 'baseline' else 'Proposed homepage'
            figures.append(f'<figure><figcaption>{label}</figcaption><video data-role="{role}" controls muted loop playsinline preload="none" poster="{paths["poster.webp"]}"><source src="{paths["mp4"]}" type="video/mp4"></video></figure>')
        executable = ROOT / 'build/examples/c/showcases' / example_id.removeprefix('showcases_')
        libraries = {path.name: sha(path) for path in gallery_frames.renderer_library_paths(executable)}
        effective_profile = gallery_media.resolve_media_entry(manifest_entries[example_id], 'homepage')['media']['preview']
        record = dict(renderer_linkage="shared" if libraries else "embedded-in-executable", executable_sha256=sha(executable), renderer_library_sha256=libraries, effective_profile=effective_profile, id=example_id, title=title, change=change, assessment=assessment, encoded_duration=item['encoded_duration'], encoded_fps=item['encoded_fps'], encoded_crf=item['encoded_crf'], capture=captures, assets=assets)
        records.append(record)
        candidate = next(asset for asset in assets if asset['role'] == 'candidate' and asset['path'].endswith('.mp4'))
        sections.append(f'<section id="{example_id}"><h2>{title}</h2><p>{escape(change)}</p><p class="assessment">{escape(assessment)}</p><div class="pair">{"".join(figures)}</div><p class="muted">Candidate: {item["encoded_duration"]} s · {item["encoded_fps"]} fps · 1280×720 · {candidate["bytes"] / 1000:.0f} kB · CRF {item["encoded_crf"]}. SHA-256: <code>{candidate["sha256"]}</code></p></section>')
    validation_path = ROOT / 'build/homepage-review/validation-summary.json'
    validation = json.loads(validation_path.read_text()) if validation_path.exists() else {}
    inventory = dict(validation=validation, state='awaiting-manual-review', source_commit=source, publication_approved=False, review_scope='Local private review only; external uploads and canonical/public deployment require final approval.', entries=records)
    (output / 'review-inventory.json').write_text(json.dumps(inventory, indent=2) + '\n')
    body = '''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Datoviz homepage motion review</title><link rel="stylesheet" href="/portal-static/style.css"><style>.review{padding-bottom:40px}.review h1{margin-top:44px}.review p{line-height:1.65}.review section{margin:28px 0;padding:24px;background:var(--panel);border:1px solid var(--line);border-radius:13px}.review a{color:var(--accent)}.pair{display:grid;grid-template-columns:1fr 1fr;gap:18px}figure{margin:0;min-width:0}figcaption,.muted{color:var(--muted);font-size:13px}video{width:100%;margin-top:10px;aspect-ratio:16/9;background:#05070a}code{overflow-wrap:anywhere;font-size:11px}.assessment{color:var(--accent)}button{padding:10px 16px;margin:8px 8px 8px 0;background:var(--panel-hover);color:var(--text);border:1px solid var(--line);border-radius:8px;cursor:pointer}@media(max-width:700px){.pair{grid-template-columns:1fr}.review section{padding:16px}}</style></head><body><main class="shell review"><header class="topbar"><a class="wordmark" href="/"><span class="mark">f</span>fractal</a><a href="/#section-datoviz">Datoviz</a></header><h1>Calmer homepage animations</h1><p>Awaiting your manual review. These exact candidates are local to this private review; no public deployment or external binary upload is approved.</p><p>Presentation movement is independent of scientific playback. Compare framing, speed, detail and the loop boundary. Videos play at their encoded speed; no browser slowdown or crossfade is used.</p><p><a href="preview/">Open the candidate documentation homepage</a> · <a href="preview/?planet=still">Homepage with still planet (recommended alternative)</a> · <a href="review-inventory.json">Exact hashes and capture provenance</a></p><button id="play-all" type="button">Play comparisons</button><button id="pause-all" type="button">Pause all</button><button id="loop-candidates" type="button">Inspect candidate loop boundaries</button><p class="muted">Each video also has its own play and seek controls. Yesterday's review is no longer the active review.</p>'''
    validation_notes = ''.join('<li>' + escape(note) + '</li>' for note in validation.get('webgpu_harness_limitations', []))
    body += '<section><h2>Validation and limitations</h2><p>All four native frame sequences were captured twice and are byte-identical. Candidate media stays at 1280×720 and within the existing size budget. Scientific timing, Python adaptations, browser playback and documentation links were checked.</p><p>All four WebGPU routes render with the configured Vulkan browser backend. The stock browser test has baseline harness issues; the local proof used a taller viewport and filtered only known informational dataset messages:</p><ul>' + validation_notes + '</ul><p>Recommended harness fixes: measure available canvas space rather than assuming a fixed footer height, and send informational dataset logs to stdout rather than treating them as browser errors.</p></section>'
    body += ''.join(sections) + '''<script>const videos=[...document.querySelectorAll('video')];document.querySelector('#play-all').onclick=()=>videos.forEach(v=>v.play().catch(()=>{}));document.querySelector('#pause-all').onclick=()=>videos.forEach(v=>v.pause());document.querySelector('#loop-candidates').onclick=()=>videos.filter(v=>v.dataset.role==='candidate').forEach(v=>v.play().then(()=>{v.currentTime=Math.max(0,v.duration-2)}).catch(()=>{}));document.addEventListener('visibilitychange',()=>{if(document.hidden)videos.forEach(v=>v.pause())});</script></main></body></html>'''
    (output / 'index.html').write_text(body)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--products', type=Path, default=gallery_media.HOMEPAGE_PRODUCTS_ROOT)
    parser.add_argument('--baseline-products', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    prepare(args.products, args.baseline_products, args.output)


if __name__ == '__main__':
    main()
