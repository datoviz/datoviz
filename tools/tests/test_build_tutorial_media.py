#!/usr/bin/env python3
"""Focused tests for Vulkan course preview generation."""

from __future__ import annotations

import os
import sys
import tempfile
import time
from pathlib import Path
from unittest import TestCase, main, mock

from PIL import Image

TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import build_tutorial_media  # noqa: E402


class TutorialMediaTest(TestCase):
    """Exercise capture scheduling, media cache validation and encoder arguments."""

    def _executables(self, root: Path) -> Path:
        directory = root / 'executables'
        directory.mkdir()
        for index in range(1, 17):
            executable = directory / f'step{index:02d}'
            executable.write_bytes(b'executable')
            future = time.time() + 3600
            os.utime(executable, (future, future))
        return directory

    def _run_step(
        self, executable: Path, arguments: list[str], cwd: Path = build_tutorial_media.ROOT
    ) -> str:
        del cwd
        if executable.name == 'step16' and '--time' in arguments:
            self.mesh_arguments = arguments
            if not hasattr(self, 'mesh_capture_times'):
                self.mesh_capture_times = []
            self.mesh_capture_times.append(float(arguments[arguments.index('--time') + 1]))
        if executable.name == 'step01':
            return 'Datoviz 0.4.0-dev\n'
        png = Path(arguments[arguments.index('--png') + 1])
        if executable.name == 'step02':
            rgba = (89, 97, 118, 255)
        elif executable.name == 'step03':
            time_s = float(arguments[arguments.index('--time') + 1])
            frame_index = round(time_s * build_tutorial_media.ANIMATION_FPS)
            rgba = build_tutorial_media.EXPECTED_STEP03_RGBA[frame_index]
        else:
            size = (
                build_tutorial_media.MESH_ANIMATION_SIZE
                if executable.name == 'step16' and '--time' in arguments
                else build_tutorial_media.SIZE
            )
            image = Image.new('RGBA', size, (40, 45, 55, 255))
            accent = (240, 80, 60, 255)
            if executable.name == 'step16' and '--time' in arguments:
                time_s = float(arguments[arguments.index('--time') + 1])
                frame_index = round(time_s * build_tutorial_media.MESH_ANIMATION_FPS)
                # A symmetric texture can revisit the same view later in the loop.
                accent = (100 + frame_index % 8, 80, 60, 255)
            image.putpixel((size[0] // 2, size[1] // 2), accent)
            image.save(png)
            return 'rendered 1 frames\nvalidation errors: 0\n'
        Image.new('RGBA', build_tutorial_media.SIZE, rgba).save(png)
        return 'rendered 1 frames\nvalidation errors: 0\n'

    @staticmethod
    def _encode_static(source: Path, output: Path, quality: int) -> None:
        del source, quality
        output.write_bytes(b'static-webp')

    @staticmethod
    def _encode_animation(
        frames: list[Path],
        output: Path,
        quality: int,
        fps: int = build_tutorial_media.ANIMATION_FPS,
        *,
        all_keyframes: bool = False,
    ) -> None:
        del quality, fps, all_keyframes
        output.write_bytes(b'animated-webp:' + b'|'.join(path.read_bytes() for path in frames))

    def test_generate_expected_previews(self) -> None:
        """Capture every expected preview and schedule one endpoint-free mesh turn."""
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            executables = self._executables(root)
            output_dir = root / 'output'
            with (
                mock.patch.object(build_tutorial_media, '_run_step', side_effect=self._run_step),
                mock.patch.object(
                    build_tutorial_media, '_encode_static', side_effect=self._encode_static
                ),
                mock.patch.object(
                    build_tutorial_media, '_encode_animation', side_effect=self._encode_animation
                ) as encode_animation,
            ):
                rc, result = build_tutorial_media.generate_tutorial_media(
                    executables_dir=executables, output_dir=output_dir, strict=True
                )
            self.assertEqual(rc, 0)
            self.assertEqual(result.generated, len(build_tutorial_media.EXPECTED_OUTPUTS))
            self.assertNotIn('all_keyframes', encode_animation.call_args_list[0].kwargs)
            self.assertTrue(encode_animation.call_args_list[1].kwargs['all_keyframes'])
            for name in build_tutorial_media.EXPECTED_OUTPUTS:
                self.assertTrue((output_dir / name).is_file())
            self.assertEqual(self.mesh_arguments[0], '--png')
            self.assertEqual(self.mesh_arguments[2:6], ['--width', '1280', '--height', '720'])
            self.assertEqual(len(build_tutorial_media.MESH_ANIMATION_TIMES), 320)
            self.assertEqual(
                self.mesh_capture_times, list(build_tutorial_media.MESH_ANIMATION_TIMES)
            )
            self.assertEqual(self.mesh_capture_times[0], 0)
            self.assertEqual(self.mesh_capture_times[-1], 15.95)
            self.assertLess(max(self.mesh_capture_times), 16)
            self.assertAlmostEqual(
                self.mesh_capture_times[-1] + 1 / build_tutorial_media.MESH_ANIMATION_FPS,
                build_tutorial_media.MESH_ANIMATION_PERIOD_SECONDS,
            )

    @mock.patch.object(build_tutorial_media, 'MESH_ANIMATION_SIZE', (64, 48))
    def test_current_outputs_are_skipped(self) -> None:
        """Reuse valid products and reject stale tools, timing or flat geometry."""
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            executables = self._executables(root)
            output_dir = root / 'output'
            output_dir.mkdir()
            for name in build_tutorial_media.EXPECTED_OUTPUTS:
                path = output_dir / name
                if name == '02-window.webp':
                    image = Image.new('RGBA', build_tutorial_media.SIZE, (89, 97, 118, 255))
                    image.save(path, format='WEBP', lossless=True)
                elif name == '03-frame.webp':
                    frames = [
                        Image.new('RGBA', build_tutorial_media.SIZE, rgba)
                        for rgba in build_tutorial_media.EXPECTED_STEP03_RGBA
                    ]
                    frames[0].save(
                        path,
                        save_all=True,
                        append_images=frames[1:],
                        format='WEBP',
                        lossless=True,
                        duration=round(1000 / build_tutorial_media.ANIMATION_FPS),
                    )
                elif name == '16-mesh-animated.webp':
                    frames = []
                    for index in range(len(build_tutorial_media.MESH_ANIMATION_TIMES)):
                        frame = Image.new(
                            'RGBA', build_tutorial_media.MESH_ANIMATION_SIZE, (40, 45, 55, 255)
                        )
                        frame.putpixel((32, 24), (100 + index % 8, 80, 60, 255))
                        frames.append(frame)
                    frames[0].save(
                        path,
                        save_all=True,
                        append_images=frames[1:],
                        format='WEBP',
                        lossless=True,
                        duration=round(1000 / build_tutorial_media.MESH_ANIMATION_FPS),
                    )
                    mesh_frames = frames
                else:
                    rgba = (
                        build_tutorial_media.EXPECTED_STEP03_RGBA[3]
                        if name == '03-frame-still.webp'
                        else (40, 45, 55, 255)
                    )
                    image = Image.new('RGBA', build_tutorial_media.SIZE, rgba)
                    if name != '03-frame-still.webp':
                        image.putpixel((400, 300), (240, 80, 60, 255))
                    else:
                        # Static and animated encodes need not reconstruct identical bytes.
                        image.putpixel((1, 0), (rgba[0] + 1, rgba[1] - 1, rgba[2], 255))
                    image.save(path, format='WEBP', lossless=True)
                future = time.time() + 3600
                os.utime(path, (future, future))
            rc, result = build_tutorial_media.generate_tutorial_media(
                executables_dir=executables, output_dir=output_dir, strict=True
            )
            self.assertEqual(rc, 0)
            self.assertEqual(result.skipped, len(build_tutorial_media.EXPECTED_OUTPUTS))
            outputs = [output_dir / name for name in build_tutorial_media.EXPECTED_OUTPUTS]

            tool = root / 'generator.py'
            tool.write_text('# newer generator')
            future = time.time() + 7200
            os.utime(tool, (future, future))
            with mock.patch.object(build_tutorial_media, '__file__', str(tool)):
                self.assertFalse(
                    build_tutorial_media._current(list(executables.iterdir()), outputs, False)
                )

            # A cache with the right frames but the old playback rate is stale.
            mesh_path = output_dir / '16-mesh-animated.webp'
            mesh_frames[0].save(
                mesh_path,
                save_all=True,
                append_images=mesh_frames[1:],
                format='WEBP',
                lossless=True,
                duration=40,
            )
            future = time.time() + 3600
            os.utime(mesh_path, (future, future))
            self.assertFalse(
                build_tutorial_media._current(list(executables.iterdir()), outputs, False)
            )
            mesh_frames[0].save(
                mesh_path,
                save_all=True,
                append_images=mesh_frames[1:],
                format='WEBP',
                lossless=True,
                duration=50,
            )
            os.utime(mesh_path, (future, future))

            flat = output_dir / '04-triangle.webp'
            Image.new('RGBA', build_tutorial_media.SIZE, (40, 45, 55, 255)).save(
                flat, format='WEBP', lossless=True
            )
            future = time.time() + 3600
            os.utime(flat, (future, future))
            outputs = [output_dir / name for name in build_tutorial_media.EXPECTED_OUTPUTS]
            self.assertFalse(
                build_tutorial_media._current(list(executables.iterdir()), outputs, False)
            )

    def test_mesh_motion_accepts_repeated_nonadjacent_phases(self) -> None:
        """Symmetric geometry can revisit a rendered view without pausing its motion."""
        hashes = [b'front', b'side', b'back', b'side'] * 80
        self.assertEqual(build_tutorial_media._require_mesh_motion(hashes), 3)

    def test_mesh_motion_rejects_static_and_adjacent_duplicate_frames(self) -> None:
        """Reject a still image, an interior pause and a duplicate loop endpoint."""
        cases = (
            [b'still'] * 320,
            [b'front', b'side', b'side', b'back'],
            [b'front', b'side', b'back', b'front'],
        )
        for hashes in cases:
            with (
                self.subTest(hashes=hashes[:4]),
                self.assertRaisesRegex(RuntimeError, 'no motion|adjacent frames'),
            ):
                build_tutorial_media._require_mesh_motion(hashes)

    def test_mesh_encoder_preserves_the_sixteen_second_loop(self) -> None:
        """The encoder uses exact 50 ms delays rather than rounding another frame rate."""
        frames = [Path(f'frame-{index:03d}.png') for index in range(320)]
        with (
            mock.patch.object(
                build_tutorial_media.shutil, 'which', return_value='/tools/img2webp'
            ),
            mock.patch.object(build_tutorial_media.subprocess, 'run') as run,
        ):
            build_tutorial_media._encode_animation(
                frames,
                Path('output.webp'),
                90,
                fps=build_tutorial_media.MESH_ANIMATION_FPS,
                all_keyframes=True,
            )
        command = run.call_args.args[0]
        delays = [int(command[i + 1]) for i, value in enumerate(command) if value == '-d']
        self.assertEqual(delays, [50] * 320)
        self.assertEqual(sum(delays), 16000)
        self.assertEqual(command[:5], ['/tools/img2webp', '-loop', '0', '-kmax', '1'])
        self.assertEqual([arg for arg in command if arg.endswith('.png')], list(map(str, frames)))

    def test_step03_encoder_keeps_default_keyframe_policy(self) -> None:
        """The mesh-specific keyframe policy does not change chapter 3's animation."""
        with (
            mock.patch.object(
                build_tutorial_media.shutil, 'which', return_value='/tools/img2webp'
            ),
            mock.patch.object(build_tutorial_media.subprocess, 'run') as run,
        ):
            build_tutorial_media._encode_animation(
                [Path('frame-0.png'), Path('frame-1.png')], Path('output.webp'), 90
            )
        self.assertNotIn('-kmax', run.call_args.args[0])

    def test_stale_mesh_executable_is_rejected_before_capture(self) -> None:
        """Changed source must not reuse an old binary even if the media cache exists."""
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            executables = self._executables(root)
            os.utime(executables / 'step16', (1, 1))
            with mock.patch.object(build_tutorial_media, '_run_step') as run:
                rc, result = build_tutorial_media.generate_tutorial_media(
                    executables_dir=executables, output_dir=root / 'output', strict=True
                )
            self.assertEqual(rc, 2)
            self.assertEqual(result.invalid, 1)
            run.assert_not_called()

    def test_strict_mode_rejects_missing_executables(self) -> None:
        """Report missing course programs before attempting any captures."""
        with tempfile.TemporaryDirectory() as tmp:
            rc, result = build_tutorial_media.generate_tutorial_media(
                executables_dir=Path(tmp), output_dir=Path(tmp) / 'output', strict=True
            )
        self.assertEqual(rc, 2)
        self.assertEqual(result.missing, 16)

    def test_decoded_clear_accepts_three_level_rgb_variation(self) -> None:
        """Lossy WebP cache checks allow bounded quantization across all pixels."""
        expected = (100, 120, 140, 255)
        image = Image.new('RGBA', (2, 1), expected)
        image.putpixel((1, 0), (103, 117, 143, 255))
        self.assertTrue(build_tutorial_media._decoded_clear_matches(image, expected))

    def test_decoded_clear_rejects_four_level_rgb_variation(self) -> None:
        """One out-of-range pixel invalidates a decoded clear, even if the corner matches."""
        expected = (100, 120, 140, 255)
        image = Image.new('RGBA', (2, 1), expected)
        image.putpixel((1, 0), (104, 120, 140, 255))
        self.assertFalse(build_tutorial_media._decoded_clear_matches(image, expected))

    def test_decoded_clear_rejects_nonopaque_pixels(self) -> None:
        """RGB compression tolerance never permits a different alpha value."""
        expected = (100, 120, 140, 255)
        image = Image.new('RGBA', (2, 1), expected)
        image.putpixel((1, 0), (100, 120, 140, 254))
        self.assertFalse(build_tutorial_media._decoded_clear_matches(image, expected))

    def test_uniform_rgba_accepts_one_level_rgb_delta(self) -> None:
        """Allow the documented one-level RGB tolerance in flat captures."""
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'close.png'
            Image.new('RGBA', build_tutorial_media.SIZE, (90, 96, 119, 255)).save(path)
            build_tutorial_media._validate_uniform_rgba(path, (89, 97, 118, 255))

    def test_uniform_rgba_rejects_larger_rgb_delta(self) -> None:
        """Reject flat captures outside the allowed RGB tolerance."""
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'wrong.png'
            Image.new('RGBA', build_tutorial_media.SIZE, (89, 97, 120, 255)).save(path)
            with self.assertRaisesRegex(RuntimeError, 'channel deltas'):
                build_tutorial_media._validate_uniform_rgba(path, (89, 97, 118, 255))

    def test_uniform_rgba_rejects_nonuniform_image(self) -> None:
        """Reject a color-clear capture containing varying pixels."""
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'gradient.png'
            image = Image.new('RGBA', build_tutorial_media.SIZE, (89, 97, 118, 255))
            image.putpixel((1, 0), (90, 97, 118, 255))
            image.save(path)
            with self.assertRaisesRegex(RuntimeError, 'uniform RGBA image'):
                build_tutorial_media._validate_uniform_rgba(path, (89, 97, 118, 255))


if __name__ == '__main__':
    main()
