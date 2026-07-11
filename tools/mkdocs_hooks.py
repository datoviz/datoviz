# mkdocs hooks

# Imports
# -------------------------------------------------------------------------------------------------

import ast
import os
import shutil
import sys
from pathlib import Path

from mkdocs.structure.files import File

# Constants
# -------------------------------------------------------------------------------------------------

CURDIR = Path(__file__).parent
ROOT = CURDIR.parent
ROOT_DOCS = ('ARCHITECTURE', 'BUILD', 'CONTRIBUTING', 'MAINTAINERS')
GALLERY_NAV_SECTIONS = (
    ('Showcases', ('showcases',)),
    ('Visuals & Composites', ('visuals', 'composites')),
    ('Features', ('features',)),
    ('Runtime & Capture', ('runtime',)),
    ('Advanced', ('advanced',)),
)


# Util functions
# -------------------------------------------------------------------------------------------------


def read(path):
    with open(CURDIR / path) as f:
        return f.read()


# Functions
# -------------------------------------------------------------------------------------------------


def remove_example_docstrings():
    src_root = 'examples'
    dst_root = 'cleaned'

    for dirpath, _, filenames in os.walk(src_root):
        py_files = [f for f in filenames if f.endswith('.py')]
        if not py_files:
            continue

        rel_dir = os.path.relpath(dirpath, src_root)
        dst_dir = os.path.join(dst_root, rel_dir)
        os.makedirs(dst_dir, exist_ok=True)

        for filename in py_files:
            src_path = os.path.join(dirpath, filename)
            dst_path = os.path.join(dst_dir, filename)

            with open(src_path) as f:
                source = f.read()
            mod = ast.parse(source)
            lines = source.splitlines()

            if (
                mod.body
                and isinstance(mod.body[0], ast.Expr)
                and isinstance(mod.body[0].value, ast.Constant)
            ):
                docstring = mod.body[0].value.value
                doc_lines = docstring.splitlines()
                start = mod.body[0].lineno
                end = start + len(doc_lines)
                lines = lines[end:]

            with open(dst_path, 'w') as f:
                f.write('\n'.join(lines).lstrip() + '\n')


def copy_tree_if_exists(src, dst, label='asset'):
    src_path = ROOT / src
    dst_path = Path(dst)
    if not src_path.exists():
        print(f"mkdocs: skipping missing {label} source {src_path}")
        return
    if src_path.resolve() == dst_path.resolve():
        return
    if dst_path.exists():
        shutil.rmtree(dst_path)
    shutil.copytree(src_path, dst_path)


def first_existing_path(*paths):
    for path in paths:
        src_path = ROOT / path
        if src_path.exists():
            return path
    return paths[0] if paths else None


def copy_webgpu_live_assets(site_dir):
    site = Path(site_dir)
    copy_tree_if_exists('examples/webgpu', site / 'examples/webgpu', 'WebGPU asset')
    copy_tree_if_exists('web/wasm', site / 'web/wasm', 'WebGPU asset')
    copy_tree_if_exists('web/drp2', site / 'web/drp2', 'WebGPU asset')
    copy_tree_if_exists(
        first_existing_path('build-wasm-scene/wasm', 'site/build-wasm-scene/wasm'),
        site / 'build-wasm-scene/wasm',
        'WebGPU asset',
    )


def copy_gallery_webp_assets(site_dir):
    site = Path(site_dir)
    copy_tree_if_exists(
        'build/gallery-webp/v0.4', site / 'assets/gallery/v0.4', 'gallery WebP asset'
    )


def add_generated_tree(files, config, src, dst_prefix, label='asset'):
    src_path = ROOT / src
    if not src_path.exists():
        print(f"mkdocs: skipping missing {label} source {src_path}")
        return files

    for path in sorted(src_path.rglob('*')):
        if not path.is_file():
            continue
        rel = path.relative_to(src_path).as_posix()
        files.append(
            File.generated(
                config,
                f"{dst_prefix.rstrip('/')}/{rel}",
                abs_src_path=str(path),
            )
        )
    return files


def find_named_nav_section(nav, title):
    for item in nav or []:
        if isinstance(item, dict) and title in item:
            section = item[title]
            return section if isinstance(section, list) else None
    return None


def build_gallery_nav_sections(examples_nav):
    if str(CURDIR) not in sys.path:
        sys.path.insert(0, str(CURDIR))

    from build_gallery import collect_examples, load_manifest, ordered_lane_examples

    manifest = load_manifest(ROOT / 'examples/c/MANIFEST.yaml')
    examples = collect_examples(manifest)
    sections = []
    for label, lanes in GALLERY_NAV_SECTIONS:
        section = find_named_nav_section(examples_nav, label)
        if section is not None:
            raise ValueError(f"Examples navigation section {label!r} is already nested")
        overview = next(
            (item[label] for item in examples_nav if isinstance(item, dict) and label in item),
            None,
        )
        if not isinstance(overview, str):
            raise ValueError(f"Examples navigation is missing the {label!r} overview page")
        pages = [{'Overview': overview}]
        for lane in lanes:
            lane_examples = ordered_lane_examples(examples, lane)
            pages.extend(
                {example.title: f"examples/{example.page_path}"} for example in lane_examples
            )
        if len(pages) == 1:
            continue
        sections.append({label: pages})
    return sections


# Hooks
# -------------------------------------------------------------------------------------------------


def on_config(config):
    examples_nav = find_named_nav_section(config.get('nav'), 'Examples')
    if examples_nav is None:
        raise ValueError("MkDocs navigation is missing the Examples section")
    replacements = {
        next(iter(section)): section for section in build_gallery_nav_sections(examples_nav)
    }
    examples_nav[:] = [
        replacements.get(next(iter(item)), item)
        if isinstance(item, dict) and len(item) == 1
        else item
        for item in examples_nav
    ]
    return config


def on_page_markdown(markdown, page, config, files):
    name = page.file.name
    # if name == "index":
    #     markdown = read("../README.md")
    #     return re.sub(r'\]\(docs/(.*?)\.md\)', r'](\1.md)', markdown)
    if name == 'LICENSE':
        return read('../LICENSE')
    elif name in ROOT_DOCS:
        return read(f'../{name}.md')
    return markdown


def on_pre_build(**kwargs):
    remove_example_docstrings()
    import sys
    sys.path.insert(0, str(CURDIR))
    import build_gallery_webp
    import gen_start_thumbs

    build_gallery_webp.generate_gallery_webp(quiet_missing=True)
    gen_start_thumbs.generate()


def on_files(files, config):
    add_generated_tree(files, config, 'examples/webgpu', 'examples/webgpu', 'WebGPU asset')
    add_generated_tree(files, config, 'web/wasm', 'web/wasm', 'WebGPU asset')
    add_generated_tree(files, config, 'web/drp2', 'web/drp2', 'WebGPU asset')
    add_generated_tree(
        files,
        config,
        first_existing_path('build-wasm-scene/wasm', 'site/build-wasm-scene/wasm'),
        'build-wasm-scene/wasm',
        'WebGPU asset',
    )
    add_generated_tree(
        files,
        config,
        'build/gallery-webp/v0.4',
        'assets/gallery/v0.4',
        'gallery WebP asset',
    )
    return files


def on_post_build(config, **kwargs):
    copy_gallery_webp_assets(config['site_dir'])
    copy_webgpu_live_assets(config['site_dir'])
