import os
import subprocess
import sys
from pathlib import Path

from tqdm import tqdm

EXAMPLES_DIR = Path('examples')
SCREENSHOTS_DIR = Path('data/gallery')


def collect_example_scripts():
    example_scripts = []
    for root, dirs, files in os.walk(EXAMPLES_DIR):
        # Skip directories starting with '_' or containing '~'
        dirs[:] = [d for d in dirs if not (d.startswith('_') or '~' in d)]
        for file in files:
            if file.endswith('.py') and file != 'video.py':
                script_path = Path(root) / file
                example_scripts.append(script_path)
    return example_scripts


def run_examples():
    example_scripts = collect_example_scripts()
    for script_path in tqdm(example_scripts, desc='Processing examples', unit='script'):
        relative_path = script_path.relative_to(EXAMPLES_DIR)
        category = relative_path.parts[0]
        example_name = script_path.stem

        # Construct the output PNG path
        if category.endswith('.py'):
            png_path = SCREENSHOTS_DIR / f'{example_name}.png'
        else:
            png_path = SCREENSHOTS_DIR / category / f'{example_name}.png'

        # Ensure the output directory exists
        png_path.parent.mkdir(parents=True, exist_ok=True)

        # Set the environment variable for the screenshot path
        env = os.environ.copy()
        env['DVZ_CAPTURE_PNG'] = str(png_path)

        try:
            subprocess.run([sys.executable, str(script_path)], env=env, check=True)
        except subprocess.CalledProcessError:
            print(f'❌ Error while running: {script_path}')
            raise


if __name__ == '__main__':
    run_examples()
