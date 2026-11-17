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
    'showcases_protein': ('Protein', 'Gentle ±12° presentation sway over 14.4 seconds; molecule, selection and crosshair stay aligned.', 'Accepted by you. The exact previously reviewed video and poster are preserved.'),
    'showcases_point_cloud': ('Point cloud', 'Restored all 6 million points. A 9.6-second sweeping camera path adds ±20° parallax, a 14% approach and a 5° rise around the scene.', 'The first proposal accidentally used the 500,000-point fallback and aggressive compression. The required full input now prevents that fallback; this candidate uses a higher quality allowance (up to 20 MB, CRF 28; a smaller balanced alternative is also provided). Check fine point detail and whether the stronger movement has enough impact.'),
    'showcases_cortical_activity': ('Cortical activity', 'Gentle ±12° brain rotation around its anatomical pivot; the established 3.6-second activity cycle still runs four times per clip.', 'Rotation and activity run independently. Check that the movement reveals enough anatomy without hiding the active region. The source activity wraps as before.'),
    'showcases_textured_planet': ('Textured planet', 'Restored the original planet display rotation (about 29° during this clip) and retained the established orbital-debris timing and time acceleration.', 'A visible loop jump is retained, as discussed. Inspect the globe and debris movement and judge whether the transition is acceptable; no crossfade conceals it.'),
}


def sha(path: Path) -> str:
    """Hash one exact local artifact."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(products: Path, baseline: Path, output: Path, accepted_inventory: Path | None = None, balanced_products: Path | None = None, balanced_report: Path | None = None) -> None:
    """Freeze comparison copies, exact hashes and capture provenance without publication."""
    if output.exists():
        raise ValueError('Review output already exists; choose a new frozen review directory')
    report = json.loads((products / 'reports/homepage/report.json').read_text())
    manifest = gallery_media.load_manifest(gallery_media.DEFAULT_MANIFEST)
    manifest_entries = {entry['id']: entry for entry in manifest['examples']}
    source = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    accepted = json.loads(accepted_inventory.read_text()) if accepted_inventory else None
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
        if example_id == 'showcases_protein' and accepted:
            expected = {asset['sha256'] for asset in accepted['assets'] if asset['role'] == 'candidate'}
            actual = {asset['sha256'] for asset in assets if asset['role'] == 'candidate'}
            if actual != expected:
                raise ValueError('Accepted protein bytes must remain unchanged')
            record.update(state='accepted', capture=accepted['capture'], capture_source_commit=accepted['source_commit'], executable_sha256=accepted['executable_sha256'], renderer_library_sha256=accepted['renderer_library_sha256'])
        else:
            record['state'] = 'awaiting-review'
        if example_id == 'showcases_point_cloud':
            import struct
            input_path = ROOT / effective_profile['required_inputs'][0]
            with input_path.open('rb') as file:
                _, _, point_count = struct.unpack('<8sII', file.read(16))
            record.update(dataset_points=point_count, dataset_sha256=sha(input_path))
        records.append(record)
        alternative = ''
        if example_id == 'showcases_point_cloud' and balanced_products:
            if balanced_report is None:
                raise ValueError('Balanced products require their encoding report')
            balanced = next(row for row in json.loads(balanced_report.read_text())['comparisons'] if row['id'] == example_id)
            for suffix in ('mp4', 'poster.webp'):
                source_path = balanced_products / 'showcases' / f'{example_id}.{suffix}'
                target_path = output / 'media' / f'{example_id}-balanced.{suffix}'
                shutil.copyfile(source_path, target_path)
                assets.append(dict(role='candidate-balanced', path=target_path.relative_to(output).as_posix(), sha256=sha(target_path), bytes=target_path.stat().st_size))
            alternative_asset = next(asset for asset in assets if asset['role'] == 'candidate-balanced' and asset['path'].endswith('.mp4'))
            record['balanced_encoding'] = dict(encoded_crf=balanced['encoded_crf'], encoded_fps=balanced['encoded_fps'], encoded_duration=balanced['encoded_duration'], video_budget_bytes=balanced['video_budget_bytes'])
            high_asset = next(asset for asset in assets if asset['role'] == 'candidate' and asset['path'].endswith('.mp4'))
            alternative = f'<p><strong>Both versions below use the same 6 million points and camera path.</strong> High detail: {high_asset["bytes"]/1000000:.1f} MB, CRF {item["encoded_crf"]}. Balanced: {alternative_asset["bytes"]/1000000:.1f} MB, CRF {balanced["encoded_crf"]}. Use these buttons to switch the proposed video:</p><button type="button" data-cloud-src="{high_asset["path"]}">High detail</button><button type="button" data-cloud-src="{alternative_asset["path"]}">Balanced</button><p class="muted" id="cloud-quality">Selected: high detail</p>'
        candidate = next(asset for asset in assets if asset['role'] == 'candidate' and asset['path'].endswith('.mp4'))
        sections.append(f'<section id="{example_id}"><h2>{title}</h2><p>{escape(change)}</p><p class="assessment">{escape(assessment)}</p>{alternative}<div class="pair">{"".join(figures)}</div><p class="muted">High-detail candidate: {item["encoded_duration"]} s · {item["encoded_fps"]} fps · 1280×720 · {candidate["bytes"] / 1000:.0f} kB · CRF {item["encoded_crf"]}. SHA-256: <code>{candidate["sha256"]}</code></p></section>')
    validation_path = ROOT / 'build/homepage-review/validation-summary.json'
    validation = json.loads(validation_path.read_text()) if validation_path.exists() else {}
    inventory = dict(validation=validation, state='awaiting-manual-review', source_commit=source, publication_approved=False, review_scope='Local private review only; external uploads and canonical/public deployment require final approval.', entries=records)
    (output / 'review-inventory.json').write_text(json.dumps(inventory, indent=2) + '\n')
    body = '''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Datoviz homepage motion review</title><link rel="stylesheet" href="/portal-static/style.css"><style>.review{padding-bottom:40px}.review h1{margin-top:44px}.review p{line-height:1.65}.review section{margin:28px 0;padding:24px;background:var(--panel);border:1px solid var(--line);border-radius:13px}.review a{color:var(--accent)}.pair{display:grid;grid-template-columns:1fr 1fr;gap:18px}figure{margin:0;min-width:0}figcaption,.muted{color:var(--muted);font-size:13px}video{width:100%;margin-top:10px;aspect-ratio:16/9;background:#05070a}code{overflow-wrap:anywhere;font-size:11px}.assessment{color:var(--accent)}button{padding:10px 16px;margin:8px 8px 8px 0;background:var(--panel-hover);color:var(--text);border:1px solid var(--line);border-radius:8px;cursor:pointer}@media(max-width:700px){.pair{grid-template-columns:1fr}.review section{padding:16px}}</style></head><body><main class="shell review"><header class="topbar"><a class="wordmark" href="/"><span class="mark">f</span>fractal</a><a href="/#section-datoviz">Datoviz</a></header><h1>Calmer homepage animations</h1><p>Awaiting your manual review. These exact candidates are local to this private review; no public deployment or external binary upload is approved.</p><p>Presentation movement is independent of scientific playback. Compare framing, speed, detail and the loop boundary. Videos play at their encoded speed; no browser slowdown or crossfade is used.</p><p><a href="preview/?cloud=balanced">Open the candidate homepage (balanced point cloud)</a> · <a href="preview/">Homepage with high-detail point cloud</a> · <a href="review-inventory.json">Exact hashes and capture provenance</a></p><button id="play-all" type="button">Play comparisons</button><button id="pause-all" type="button">Pause all</button><button id="loop-candidates" type="button">Inspect candidate loop boundaries</button><p class="muted">Each video also has its own play and seek controls. Yesterday's review is no longer the active review.</p>'''
    validation_notes = ''.join('<li>' + escape(note) + '</li>' for note in validation.get('webgpu_harness_limitations', []))
    body += '<section><h2>Validation and limitations</h2><p>Protein retains its accepted exact bytes. The three revised native frame sequences were captured twice and are byte-identical. Candidate media stays at 1280×720 and within their explicit size budgets (the high-detail point cloud allows up to 20 MB to protect detail). Scientific timing, Python adaptations, browser playback and documentation links were checked.</p><p>All four WebGPU routes render with the configured Vulkan browser backend. The stock browser test has baseline harness issues; the local proof used a taller viewport and filtered only known informational dataset messages:</p><ul>' + validation_notes + '</ul><p>Recommended harness fixes: measure available canvas space rather than assuming a fixed footer height, and send informational dataset logs to stdout rather than treating them as browser errors.</p></section>'
    body += ''.join(sections) + '''<script>const videos=[...document.querySelectorAll('video')];document.querySelector('#play-all').onclick=()=>videos.forEach(v=>v.play().catch(()=>{}));document.querySelector('#pause-all').onclick=()=>videos.forEach(v=>v.pause());document.querySelector('#loop-candidates').onclick=()=>videos.filter(v=>v.dataset.role==='candidate').forEach(v=>v.play().then(()=>{v.currentTime=Math.max(0,v.duration-2)}).catch(()=>{}));document.querySelectorAll('[data-cloud-src]').forEach(b=>b.onclick=()=>{const v=document.querySelector('#showcases_point_cloud video[data-role=candidate]');const playing=!v.paused;const time=v.currentTime;v.src=b.dataset.cloudSrc;v.onloadedmetadata=()=>{v.currentTime=Math.min(time,v.duration);if(playing)v.play().catch(()=>{})};v.load();document.querySelector('#cloud-quality').textContent='Selected: '+b.textContent.toLowerCase()});document.addEventListener('visibilitychange',()=>{if(document.hidden)videos.forEach(v=>v.pause())});</script></main></body></html>'''
    (output / 'index.html').write_text(body)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--products', type=Path, default=gallery_media.HOMEPAGE_PRODUCTS_ROOT)
    parser.add_argument('--baseline-products', type=Path, required=True)
    parser.add_argument('--accepted-inventory', type=Path, help='Previously accepted protein inventory; exact bytes and capture provenance are retained')
    parser.add_argument('--balanced-products', type=Path)
    parser.add_argument('--balanced-report', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    prepare(args.products, args.baseline_products, args.output, args.accepted_inventory, args.balanced_products, args.balanced_report)


if __name__ == '__main__':
    main()
