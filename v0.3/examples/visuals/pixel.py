"""
# Pixel visual

Show the pixel visual.

---
tags:
  - pixel
  - panzoom
dependencies:
  - matplotlib
in_gallery: true
make_screenshot: true
---

"""

import matplotlib.colors as mcolors
import numpy as np

import datoviz as dvz


def generate_data():
    """Return N, positions (N,3) float32, colors (N,4) uint8"""
    # Parameters
    n_arms = 5
    n_particles_per_arm = 200_000
    n_total = n_arms * n_particles_per_arm

    rng = np.random.default_rng(seed=3141)

    # Radius from center, with more points toward center
    r = rng.power(2.0, size=n_total)  # values in [0, 1), biased toward 0

    # Angle with swirl per arm and some noise
    base_theta = np.repeat(np.linspace(0, 2 * np.pi, n_arms, endpoint=False), n_particles_per_arm)
    swirl = r * 3  # spiral effect
    noise = rng.normal(scale=0.2, size=n_total)
    theta = base_theta + swirl + noise

    # Convert polar to Cartesian
    x = r * np.cos(theta) * 6.0 / 8.0  # HACK: window aspect ratio
    y = r * np.sin(theta)
    z = np.zeros_like(x)

    positions = np.stack([x, y, z], axis=1).astype(np.float32)

    # Colors based on radius and angle — create a vibrant, cosmic feel
    hue = (theta % (2 * np.pi)) / (2 * np.pi)  # hue from angle
    saturation = np.clip(r * 1.5, 0.2, 1.0)  # more saturated at edges
    value = np.ones_like(hue)

    # Convert HSV to RGB

    rgb = mcolors.hsv_to_rgb(np.stack([hue, saturation, value], axis=1))
    rgb_u8 = (rgb * 255).astype(np.uint8)

    # Alpha: slight fade with radius
    alpha = np.clip(128 * (1.0 - r), 1, 255).astype(np.uint8)
    alpha = (200 * np.exp(-5 * r * r)).astype(np.uint8)

    colors = np.concatenate([rgb_u8, alpha[:, None]], axis=1)

    return n_total, positions, colors


N, position, color = generate_data()

app = dvz.App()
figure = app.figure()
panel = figure.panel()
panzoom = panel.panzoom()

visual = app.pixel(position=position, color=color)
panel.add(visual)

app.run()
app.destroy()
