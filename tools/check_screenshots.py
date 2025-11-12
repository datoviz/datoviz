import os
import subprocess
import sys
from pathlib import Path
import typing as tp
import skimage.io
import numpy as np

from tqdm import tqdm

SCREENSHOTS_DIR = Path('data/gallery')
EXPECTED_SCREENSHOTS_DIR = Path('expected_gallery')


def collect_expected_screenshots()-> tp.List[Path]:
    example_scripts = []
    for root, dirs, files in os.walk(EXPECTED_SCREENSHOTS_DIR):
        # Skip directories starting with '_' or containing '~'
        dirs[:] = [d for d in dirs if not (d.startswith('_') or '~' in d)]
        for file in files:
            if file.endswith('.png'):
                script_path = Path(root) / file
                example_scripts.append(script_path)
    return example_scripts


def check_screenshots(filter=None) -> int:
    expected_screenshots = collect_expected_screenshots()
    error_count = 0
    for image_path in tqdm(expected_screenshots, desc='Checking screenshots', unit='screenshot'):
        relative_path = image_path.relative_to(EXPECTED_SCREENSHOTS_DIR)
        example_name = image_path.stem

        # Skip scripts if filter is provided and doesn't match example_name
        if filter and filter not in example_name:
            continue

        generated_image_path = SCREENSHOTS_DIR / relative_path
        if not generated_image_path.exists():
            print(f'❌ Missing screenshot: {generated_image_path}')
            error_count += 1
            continue

        # Read images as np.ndarray using skimage
        expected_image: np.ndarray = skimage.io.imread(image_path)
        generated_image: np.ndarray = skimage.io.imread(generated_image_path)

        # structural similarity index to compare images perceptually - score between -1.0 and 1.0
        score_np = skimage.metrics.structural_similarity(expected_image, generated_image, channel_axis=2)
        score = float(score_np)

        # tolerance threshold - 1.0 is exact match
        # Make this number less than 1.0 to allow for minor differences due to compression, etc.
        tolerance_threshold = 0.99
        if score < tolerance_threshold:
            print(f'❌ Screenshot does not match for: {relative_path} (SSIM: {score:.4f})')
            error_count += 1
            continue

    return error_count

if __name__ == '__main__':
    filter_arg = sys.argv[1] if len(sys.argv) > 1 else None
    error_count = check_screenshots(filter=filter_arg)
    if error_count > 0:
        print(f'\n❌ Total screenshot mismatches: {error_count}')
        sys.exit(1)
    else:
        print('\n✅ All screenshots match expected ones.')
        sys.exit(0)
