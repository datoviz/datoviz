"""Tests of the Datoviz ctypes wrapper."""

import unittest
from pathlib import Path

import numpy as np

import datoviz as dvz


CURDIR = Path(__file__).parent
IMGDIR = (CURDIR / "build/artifacts").resolve()
IMGDIR.mkdir(exist_ok=True, parents=True)


class DatovizTests(unittest.TestCase):
    def test_struct(self):
        ev = dvz.MouseDragEvent()
        ev.button = 1
        ev.press_pos[0] = -1.23
        ev.press_pos[1] = +4.56
        ev.shift[0] = +10.987
        ev.shift[1] = -20.456
        ev.is_press_valid = True

        self.assertEqual(ev.button, 1)
        self.assertEqual(ev.is_press_valid, True)
        self.assertAlmostEqual(ev.press_pos[0], -1.23, places=4)
        self.assertAlmostEqual(ev.press_pos[1], +4.56, places=4)
        self.assertAlmostEqual(ev.shift[0], +10.987, places=4)
        self.assertAlmostEqual(ev.shift[1], -20.456, places=4)

    def test_offscreen(self):
        """Generate an offscreen image and check that it worked."""

        app = dvz.app(dvz.APP_FLAGS_OFFSCREEN)
        batch = dvz.app_batch(app)
        scene = dvz.scene(batch)

        figure = dvz.figure(scene, 800, 600, 0)
        panel = dvz.panel_default(figure)
        pz = dvz.panel_panzoom(panel)

        visual = dvz.point(batch, 0)

        n = 1000
        dvz.point_alloc(visual, n)

        pos = np.random.normal(size=(n, 3), scale=.25).astype(np.float32)
        dvz.point_position(visual, 0, n, pos, 0)

        color = np.random.uniform(
            size=(n, 4), low=50, high=240).astype(np.uint8)
        dvz.point_color(visual, 0, n, color, 0)

        size = np.random.uniform(size=(n,), low=10, high=30).astype(np.float32)
        dvz.point_size(visual, 0, n, size, 0)

        dvz.panel_visual(panel, visual, 0)
        dvz.scene_run(scene, app, 0)

        path = IMGDIR / "offscreen_python.png"
        self.assertFalse(path.exists())

        dvz.app_screenshot(app, dvz.figure_id(figure), path)
        dvz.scene_destroy(scene)
        dvz.app_destroy(app)

        self.assertTrue(path.exists())
        self.assertTrue(path.stat().st_size > 100_000)
        path.unlink()


if __name__ == '__main__':
    unittest.main()
