#!/usr/bin/env python3
"""Read and verify the exact reviewed homepage snapshot without regenerating its assets."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[1]
SELECTED_PRODUCTS = ROOT / 'build/homepage-review/selected-homepage-media'


def selection_for(manifest: dict) -> dict | None:
    """Load the root-relative selected snapshot declared by an example manifest."""
    declaration = manifest.get('homepage_media', {}).get('selection')
    if declaration is None:
        return None
    if not isinstance(declaration, str) or not declaration:
        raise ValueError('Homepage selection declaration must be a repository-relative path')
    relative = PurePosixPath(declaration)
    if "\\" in declaration or relative.is_absolute() or '..' in relative.parts:
        raise ValueError('Homepage selection must use a root-relative repository path')
    selection = json.loads((ROOT / relative).read_text())
    validate_selection(selection)
    return selection


def validate_selection(selection: dict) -> None:
    """Reject ambiguous identities, unsafe paths and incomplete selected examples."""
    if selection.get('schema_version') != 1:
        raise ValueError('Unsupported homepage selection schema')
    examples = selection.get('examples', [])
    expected = {'showcases_protein', 'showcases_point_cloud', 'showcases_cortical_activity', 'showcases_textured_planet'}
    if {entry.get('id') for entry in examples} != expected or len(examples) != 4:
        raise ValueError('Homepage selection must contain exactly the four homepage examples')
    paths = set()
    for entry in examples:
        roles = []
        for asset in entry.get('assets', []):
            path = PurePosixPath(asset['path'])
            if path.is_absolute() or '..' in path.parts or path.parts[:1] != ('showcases',):
                raise ValueError('Unsafe homepage asset path')
            if len(path.parts) != 2 or not path.name.startswith(entry['id'] + '.') or str(path) in paths:
                raise ValueError('Duplicate or mismatched homepage asset path')
            paths.add(str(path))
            digest = asset.get('sha256', '')
            if len(digest) != 64 or any(c not in '0123456789abcdef' for c in digest):
                raise ValueError('Homepage assets require a full SHA-256')
            if type(asset.get('bytes')) is not int or asset['bytes'] <= 0:
                raise ValueError('Homepage assets require positive byte sizes')
            role = asset.get('role')
            if role not in {'primary', 'fallback', 'poster'}:
                raise ValueError('Unknown homepage asset role')
            if role == 'poster' and not str(path).endswith('.poster.webp'):
                raise ValueError('Homepage posters must be WebP assets')
            if role != 'poster' and (not str(path).endswith('.mp4') or asset.get('codec') not in {'av1', 'h264'}):
                raise ValueError('Homepage videos require an explicit supported MP4 codec')
            roles.append(role)
        if roles.count('primary') != 1 or roles.count('poster') != 1 or roles.count('fallback') > 1:
            raise ValueError('Each homepage example needs one primary video and one poster')


def assets_for(selection: dict) -> list[dict]:
    """Return selected assets in deterministic example/role order."""
    return [dict(asset, example=entry['id']) for entry in selection['examples'] for asset in entry['assets']]


def verify_asset(root: Path, asset: dict) -> Path:
    """Verify exact byte count and digest, confining all reads to the selected root."""
    path = (root / asset['path']).resolve()
    if not path.is_relative_to(root.resolve()) or not path.is_file():
        raise ValueError(f'Missing selected homepage asset: {asset["path"]}')
    if path.stat().st_size != asset['bytes']:
        raise ValueError(f'Selected homepage asset size mismatch: {asset["path"]}')
    if hashlib.sha256(path.read_bytes()).hexdigest() != asset['sha256']:
        raise ValueError(f'Selected homepage asset hash mismatch: {asset["path"]}')
    return path


def video_urls(selection: dict) -> list[str]:
    """Return every primary/fallback URL required for local browser availability."""
    return ['/assets/gallery/v0.4/profiles/homepage/' + asset['path']
            for asset in assets_for(selection) if asset['role'] != 'poster']


def main() -> int:
    """Verify or package the selected bytes without performing any network operation."""
    import argparse
    import gzip
    import io
    import tarfile
    import gallery_media

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument('--products', type=Path, default=SELECTED_PRODUCTS)
    parser.add_argument('--package', type=Path, help='Write a deterministic local gzip/tar publication draft')
    parser.add_argument('--publication', action='store_true', help='Require the recorded final publication gate')
    args = parser.parse_args()
    try:
        selection = selection_for(gallery_media.load_manifest(args.manifest))
        if not selection:
            raise ValueError('No selected homepage snapshot is declared')
        if args.publication and not selection.get('publication_authorized'):
            raise ValueError('Final manual publication approval is not recorded; no upload or deployment is authorized')
        verified = [(asset, verify_asset(args.products, asset)) for asset in assets_for(selection)]
        if args.package:
            args.package.parent.mkdir(parents=True, exist_ok=True)
            temporary = args.package.with_name(args.package.name + '.tmp')
            with temporary.open('wb') as raw, gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=0) as compressed:
                with tarfile.open(fileobj=compressed, mode='w') as archive:
                    items = [(asset['path'], path.read_bytes()) for asset, path in verified]
                    bundle = {'schema_version': 1, 'snapshot_id': selection.get('snapshot_id'),
                              'examples': [dict(entry, assets=[{key: value for key, value in asset.items() if key != 'review_state'} for asset in entry['assets']]) for entry in selection['examples']]}
                    items.append(('homepage-media.json', (json.dumps(bundle, indent=2) + '\n').encode()))
                    for name, content in sorted(items):
                        info = tarfile.TarInfo(name)
                        info.size, info.mtime, info.mode = len(content), 0, 0o644
                        archive.addfile(info, io.BytesIO(content))
            if args.package.exists() and hashlib.sha256(args.package.read_bytes()).digest() != hashlib.sha256(temporary.read_bytes()).digest():
                temporary.unlink()
                raise ValueError('Refusing to replace an existing publication package with different bytes')
            temporary.replace(args.package)
            print(f'Local package: {args.package.name}; SHA-256 {hashlib.sha256(args.package.read_bytes()).hexdigest()}')
        print(f'Selected homepage snapshot verified: {len(verified)} exact assets; publication authorized={selection.get("publication_authorized", False)}')
        return 0
    except (OSError, ValueError) as error:
        print(f'Homepage media: {error}')
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
