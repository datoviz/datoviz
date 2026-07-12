import numpy as np
import datoviz as dvz


n = 10_000
rng = np.random.default_rng(12345)
positions = rng.uniform(-1, 1, (n, 3)).astype(np.float32)
positions[:, 2] = 0
colors = rng.integers(0, 256, (n, 4), dtype=np.uint8)
colors[:, 3] = 200
diameters = rng.uniform(4, 12, n).astype(np.float32)

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 1280, 720, 0)
panel = dvz.dvz_panel_full(figure)

points = dvz.dvz_point(scene, 0)
dvz.dvz_visual_set_data_many(
    points,
    {"position": positions, "color": colors, "diameter_px": diameters},
)
dvz.dvz_panel_add_visual(panel, points, None)

panzoom = dvz.dvz_panzoom(scene, None)
dvz.dvz_panel_bind_controller(panel, panzoom, dvz.DVZ_DIM_MASK_XY)

dvz.run(scene, figure, title="Datoviz Quickstart")
