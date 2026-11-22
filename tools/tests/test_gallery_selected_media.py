"""Keep reviewed example videos identical on the homepage and gallery pages."""

from html.parser import HTMLParser
from pathlib import Path
import sys
import re
import unittest
from urllib.parse import urljoin

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import build_gallery


class Videos(HTMLParser):
    def __init__(self):
        super().__init__()
        self.videos = []
        self.current = None

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if tag == 'video':
            self.current = {'poster': attrs.get('poster'), 'sources': []}
            self.videos.append(self.current)
        elif tag == 'source' and self.current is not None:
            self.current['sources'].append(attrs.get('data-src', attrs.get('src')))

    def handle_endtag(self, tag):
        if tag == 'video':
            self.current = None


class SelectedMediaTests(unittest.TestCase):
    def test_homepage_and_example_pages_share_all_reviewed_assets(self):
        manifest = build_gallery.load_manifest(build_gallery.DEFAULT_MANIFEST)
        homepage = Videos()
        homepage.feed((build_gallery.ROOT / 'docs/index.md').read_text())
        selected = [
            example
            for example in build_gallery.collect_examples(manifest)
            if example.selected_assets
        ]
        self.assertEqual(len(selected), 4)
        for example in selected:
            with self.subTest(example=example.id):
                original = next(
                    video for video in homepage.videos if example.id + '.' in video['poster']
                )
                detail = Videos()
                detail.feed(
                    build_gallery.video_media_block(
                        Path('examples') / example.page_path,
                        example,
                        build_gallery.DEFAULT_IMAGE_URL_BASE,
                    )
                )
                self.assertEqual(len(detail.videos), 1)
                base = (
                    'https://docs.invalid/examples/' + example.page_path.removesuffix('.md') + '/'
                )
                actual = detail.videos[0]
                self.assertEqual(
                    urljoin(base, actual['poster']),
                    urljoin('https://docs.invalid/', original['poster']),
                )
                self.assertEqual(
                    [urljoin(base, value) for value in actual['sources']],
                    [urljoin('https://docs.invalid/', value) for value in original['sources']],
                )

    def test_authored_pages_do_not_reference_superseded_video_paths(self):
        manifest = build_gallery.load_manifest(build_gallery.DEFAULT_MANIFEST)
        ids = [
            example.id
            for example in build_gallery.collect_examples(manifest)
            if example.selected_assets
        ]
        pattern = re.compile(
            r'assets/gallery/v0\.4/showcases/(?:' + '|'.join(ids) + r')\.(?:mp4|poster\.webp)'
        )
        for page in (build_gallery.ROOT / 'docs').rglob('*.md'):
            with self.subTest(page=page):
                self.assertIsNone(pattern.search(page.read_text()))

    def test_unselected_example_keeps_its_normal_gallery_media(self):
        manifest = build_gallery.load_manifest(build_gallery.DEFAULT_MANIFEST)
        example = next(
            example
            for example in build_gallery.collect_examples(manifest)
            if example.id == 'features_controller_arcball'
        )
        self.assertFalse(example.selected_assets)
        markup = build_gallery.video_media_block(
            Path('examples') / example.page_path, example, build_gallery.DEFAULT_IMAGE_URL_BASE
        )
        self.assertNotIn('profiles/homepage', markup)
        self.assertIn('features_controller_arcball.mp4', markup)


if __name__ == '__main__':
    unittest.main()
