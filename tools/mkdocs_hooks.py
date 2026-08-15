# mkdocs hooks

# Imports
# -------------------------------------------------------------------------------------------------

import ast
import io
import os
import shutil
import sys
from datetime import datetime, timezone
from pathlib import Path

from mkdocs.plugins import event_priority
from mkdocs.structure.files import File

# Constants
# -------------------------------------------------------------------------------------------------

CURDIR = Path(__file__).parent
ROOT = CURDIR.parent
ROOT_DOCS = ('ARCHITECTURE', 'BUILD', 'CONTRIBUTING', 'MAINTAINERS')
SITE_ASSETS_ENV = 'DATOVIZ_DOCS_SITE_ASSETS'
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


def copy_vulkan_course_media(site_dir):
    site = Path(site_dir)
    copy_tree_if_exists(
        'build/vulkan-course-media',
        site / 'assets/gpu-graphics',
        'Vulkan course media asset',
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


def build_gallery_nav_sections():
    if str(CURDIR) not in sys.path:
        sys.path.insert(0, str(CURDIR))

    from build_gallery import EXAMPLE_NAVIGATION, collect_examples, load_manifest, validate_navigation
    manifest = load_manifest(ROOT / 'examples/c/MANIFEST.yaml')
    examples = collect_examples(manifest)
    validate_navigation(EXAMPLE_NAVIGATION, examples)
    by_id = {example.id: example for example in examples if example.has_detail_page}
    sections = []
    for section in EXAMPLE_NAVIGATION.sections:
        overview = f'examples/{section.overview}'
        pages = [{'Overview': overview}]
        if section.groups:
            for group in section.groups:
                group_pages = [
                    {by_id[id_].title: f'examples/{by_id[id_].page_path}'}
                    for id_ in group.example_ids
                ]
                pages.append({group.title: group_pages})
        else:
            pages.extend(
                {by_id[id_].title: f'examples/{by_id[id_].page_path}'}
                for id_ in section.example_ids
            )
        sections.append({section.title: pages})
    return sections


def prepare_optional_site_assets():
    """Stage external publication assets only for an explicit site build."""
    import build_gallery_webp
    import build_webgpu_data_bundles

    gallery_output = ROOT / 'build/gallery-webp/v0.4'
    webgpu_output = ROOT / 'build/webgpu-data'
    if os.environ.get(SITE_ASSETS_ENV) != '1':
        shutil.rmtree(gallery_output, ignore_errors=True)
        shutil.rmtree(webgpu_output, ignore_errors=True)
        stage_hermetic_gallery_placeholders(gallery_output)
        print(f"mkdocs: external publication assets disabled; set {SITE_ASSETS_ENV}=1 to stage real media")
        return

    build_gallery_webp.generate_gallery_webp(quiet_missing=False, animated_fallbacks=True)
    build_webgpu_data_bundles.stage_bundles()


def stage_hermetic_gallery_placeholders(output):
    """Create generated-only stand-ins so strict docs builds can validate publication links."""
    from PIL import Image, ImageDraw
    from build_gallery import collect_examples, load_manifest

    image = Image.new('RGB', (640, 360), color=(32, 38, 48))
    draw = ImageDraw.Draw(image)
    draw.line((0, 0, 640, 360), fill=(80, 96, 120), width=8)
    draw.line((0, 360, 640, 0), fill=(80, 96, 120), width=8)
    draw.text((24, 24), 'MEDIA EXCLUDED FROM HERMETIC BUILD', fill=(220, 226, 235))
    encoded = io.BytesIO()
    image.save(encoded, format='WEBP', quality=70, method=6)
    payload = encoded.getvalue()

    examples = collect_examples(load_manifest(ROOT / 'examples/c/MANIFEST.yaml'))
    count = 0
    for example in examples:
        if not example.screenshot_expected:
            continue
        path = Path(output) / example.lane / f'{example.id}.webp'
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(payload)
        count += 1
    print(f'mkdocs: staged {count} hermetic gallery placeholders')


# Hooks
# -------------------------------------------------------------------------------------------------


def on_config(config):
    examples_nav = find_named_nav_section(config.get('nav'), 'Examples')
    if examples_nav is None:
        raise ValueError("MkDocs navigation is missing the Examples section")
    if examples_nav != [{'Overview': 'examples/index.md'}]:
        raise ValueError("MkDocs Examples navigation must contain only its Overview page")
    examples_nav.extend(build_gallery_nav_sections())
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


@event_priority(-99)
def on_page_context(context, page, config, nav):
    """Keep scheduled posts unlisted but directly previewable under ``mkdocs serve``."""
    blog = config.plugins.get('material/blog')
    if not blog or not blog.config.draft_if_future_date or not blog.config.draft:
        return context

    now = datetime.now(timezone.utc)
    posts = getattr(page, 'posts', None)
    if posts is not None:
        posts[:] = [post for post in posts if post.config.date.created <= now]

    if getattr(getattr(page, 'config', None), 'date', None):
        for attr in ('previous_page', 'next_page'):
            adjacent = getattr(page, attr, None)
            adjacent_date = getattr(getattr(adjacent, 'config', None), 'date', None)
            if adjacent_date and adjacent_date.created > now:
                setattr(page, attr, None)

    return context


def on_pre_build(**kwargs):
    remove_example_docstrings()
    import sys
    sys.path.insert(0, str(CURDIR))
    import build_tutorial_media
    import gen_start_thumbs

    prepare_optional_site_assets()
    tutorial_rc, _ = build_tutorial_media.generate_tutorial_media(strict=True)
    if tutorial_rc != 0:
        raise RuntimeError("Vulkan tutorial preview generation failed")
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
    add_generated_tree(
        files,
        config,
        'build/vulkan-course-media',
        'assets/gpu-graphics',
        'Vulkan course media asset',
    )
    add_generated_tree(files, config, 'build/webgpu-data', 'webgpu-data', 'WebGPU data bundle')
    return files


def on_post_build(config, **kwargs):
    copy_gallery_webp_assets(config['site_dir'])
    copy_vulkan_course_media(config['site_dir'])
    copy_webgpu_live_assets(config['site_dir'])
