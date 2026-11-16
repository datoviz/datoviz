"""Thumbnail crops must stay inside current gallery captures."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from PIL import Image


spec = importlib.util.spec_from_file_location('gen_start_thumbs', Path(__file__).parents[1] / 'gen_start_thumbs.py')
thumbs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(thumbs)


class ThumbnailTests(unittest.TestCase):
    def test_reference_crops_do_not_pad_current_capture_with_black(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'source'
            source.mkdir()
            Image.new('RGB', (1280, 720), (80, 150, 200)).save(source / 'capture.png')
            entries = [('preview', 'capture.png', (150, 60, 1350, 960))]
            with patch.object(thumbs, 'THUMBS', entries):
                thumbs.generate(force=True, image_dir=source, output_dir=root / 'output')
            with Image.open(root / 'output/preview.webp') as image:
                self.assertEqual(image.size, (240, 180))
                self.assertTrue(all(low > 60 for low, high in image.getextrema()))
