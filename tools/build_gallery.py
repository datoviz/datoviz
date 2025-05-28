#!/usr/bin/env python3
import re
from pathlib import Path
from textwrap import dedent

import yaml

EXAMPLES_DIR = Path('examples')
GALLERY_DIR = Path('docs/gallery')
DATA_DIR = Path('data')
CATEGORIES = ['showcase', 'visuals', 'features']
GITHUB_IMG_BASE = 'https://raw.githubusercontent.com/datoviz/data/main/gallery'

INTRO = {
    'gallery': """The Datoviz gallery shows what the library can do through concrete examples.
It is divided into three sections.

* The [**showcase**](#showcase) section features polished demos based on real-world data.
* The [**visuals**](#visuals) section shows one example per visual type.
* The [**features**](#features) section focuses on specific API features.

""",
    'showcase': """This section highlights polished demos built on real-world datasets.

""",
    'visuals': """Each example in this section focuses on a single visual type.

""",
    'features': """This section isolates individual features of the Datoviz API.
Each example is designed to demonstrate a specific capability.

!!! warning

    Some examples use GUI elements that are not yet supported in automatic screenshots.
    As a result, certain screenshots may appear blank. This limitation will be addressed in a
    future release.

""",
}


def extract_metadata(script_path):
    content = script_path.read_text(encoding='utf-8')

    # Match top-level docstring
    match = re.match(r'\s*[ru]?["\']{3}(.*?["\']{3})', content, re.DOTALL)
    if not match:
        raise ValueError(f'No docstring found in {script_path}')
    docstring = match.group(1).strip().rstrip('\'"')

    # Extract title: first line starting with #
    lines = docstring.splitlines()
    title_line = (
        next((line for line in lines if line.strip().startswith('#')), '').lstrip('#').strip()
    )

    # Extract YAML block
    yaml_match = re.search(r'^---(.*?)---', docstring, re.DOTALL | re.MULTILINE)
    yaml_raw = yaml_match.group(1).strip() if yaml_match else ''
    metadata = yaml.safe_load(yaml_raw) if yaml_raw else {}
    tags = metadata.get('tags', [])

    # Extract description: all lines between title and YAML block
    title_index = next(i for i, line in enumerate(lines) if line.strip().startswith('#'))
    description_lines = []
    for i in range(title_index + 1, len(lines)):
        if lines[i].strip().startswith('---'):
            break
        description_lines.append(lines[i].rstrip())
    description = '\n'.join(description_lines).strip()

    # Remove docstring from the rest of the code
    code_body = re.sub(
        r'^\s*[ru]?["\']{3}.*?["\']{3}', '', content, count=1, flags=re.DOTALL
    ).lstrip()

    in_gallery = True
    if 'in_gallery: false' in content:
        in_gallery = False
    return {
        'title': title_line,
        'description': description,
        'tags': tags,
        'code': dedent(code_body).rstrip(),
        'in_gallery': in_gallery,
    }


def example_image_exists(category, name):
    return (DATA_DIR / f'gallery/{category}/{name}.png').exists()


def get_screenshot_url(category, name):
    if example_image_exists(category, name):
        return f'{GITHUB_IMG_BASE}/{category}/{name}.png'
    else:
        return f'{GITHUB_IMG_BASE}/empty.png'


def generate_example_page(category, script_path, output_path):
    name = script_path.stem
    meta = extract_metadata(script_path)
    if example_image_exists(category, name):
        screenshot_image = f'![Screenshot]({get_screenshot_url(category, name)})'
    else:
        screenshot_image = ''
    back_link = f'[← Back to gallery](../index.md#{category})'

    md = f"""\
# {meta['title']}

{meta['description']}

**Tags**: {', '.join(meta['tags'])}

{screenshot_image}

=== "Python code"

    ```python
    {meta['code'].replace('\n', '\n    ')}
    ```

{back_link}
"""
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(md.strip() + '\n', encoding='utf-8')


def generate_index(pages):
    lines = [
        '# Gallery\n',
        '<!-- WARNING: this file is auto-generated, edit tools/build_gallery.py instead -->\n',
    ]
    lines.extend(INTRO['gallery'].splitlines())
    for category in CATEGORIES:
        lines.append(f'## {category.capitalize()}\n')
        lines.extend(INTRO[category].splitlines())
        lines.append('<div class="grid cards" markdown="1">\n')
        for name, title in pages.get(category, []):
            screenshot_url = get_screenshot_url(category, name)
            page_url = f'{category}/{name}/'
            lines.append(f"""\
<div class="card">
  <a href="{page_url}">
    <img src="{screenshot_url}" alt="{title}" />
    <div class="card-title">{title}</div>
  </a>
</div>
""")
        lines.append('</div>\n')
    return '\n'.join(lines)


def main():
    all_pages = {}
    for category in CATEGORIES:
        example_dir = EXAMPLES_DIR / category
        output_dir = GALLERY_DIR / category
        for script_path in sorted(example_dir.glob('*.py')):
            name = script_path.stem
            output_path = output_dir / f'{name}.md'
            metadata = extract_metadata(script_path)
            # Skip examples not in the gallery
            if not metadata['in_gallery']:
                continue
            generate_example_page(category, script_path, output_path)
            all_pages.setdefault(category, []).append((name, metadata['title']))
    index_path = GALLERY_DIR / 'index.md'
    index_md = generate_index(all_pages)
    index_path.write_text(index_md, encoding='utf-8')


if __name__ == '__main__':
    main()
