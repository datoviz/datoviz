# Wiggle Visual

The **Wiggle** visual displays multichannel time series data (such as seismic traces) using a traditional wiggle plot format. Each channel is plotted as a waveform offset vertically, with filled positive and/or negative areas.

<figure markdown="span">
![Wiggle visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/wiggle.png)
</figure>

---

## Overview

* Displays multichannel time series (e.g. seismograms, electrophysiology)
* Encoded as a 2D texture: `(channels, samples)`
* Each row is rendered as a horizontally-scaled waveform
* Positive and negative areas can be filled with separate colors
* Highly efficient GPU rendering using a single texture

---

## When to use

Use the **wiggle** visual when:

* You want to render dense multichannel waveform data (e.g., seismic traces)
* You need a compact and interactive time series overview
* You want to distinguish positive/negative lobes with color fill
* You prefer classic geophysical wiggle plotting

---

## Properties

### Per-visual (uniform)

| Parameter        | Type                               | Description                                               |
| ---------------- | ---------------------------------- | --------------------------------------------------------- |
| `bounds`         | `((float, float), (float, float))` | 2D bounds in NDC coordinates `(xmin, xmax), (ymin, ymax)` |
| `xrange`         | `(float, float)`                   | Horizontal range                                          |
| `scale`          | `float`                            | Scale factor for amplitude                                |
| `negative_color` | `(4,) uint8`                       | Fill color for negative lobes in RGBA                     |
| `positive_color` | `(4,) uint8`                       | Fill color for positive lobes in RGBA                     |
| `edgecolor`      | `(4,) uint8`                       | Line color for outlines in RGBA                           |
| `texture`        | `Texture`                          | 2D float32 texture with shape `(channels, samples)`       |

!!! note

    The visual expects the input texture to be a 2D array of shape `(channels, samples)`. Each channel corresponds to a vertically aligned trace. Linear interpolation leads to smoother wiggle plots.

---

## Bounds and xrange

Use `bounds` to control the position and size of the wiggle plot in Normalized Device Coordinates (NDC). Internally, the plot is rendered on a rectangular quad (composed of two triangles and six vertices), with the wiggle waveform computed in real time by the fragment shader—executed in parallel for each pixel. The `bounds` define the full 2D extent of the visual on screen.

Use `xrange` to specify the horizontal domain of the time series, in normalized coordinates (e.g., `(0, 1)` spans the entire width of the plot).


---

## Colors

The wiggle visual can fill waveform lobes with distinct colors depending on polarity.

* `positive_color`: used to fill regions above zero
* `negative_color`: used to fill regions below zero
* `edgecolor`: optional line trace on top of filled wiggle

---

## Scale

The `scale` attribute controls the amplitude of the waveforms. Using `scale = 1` means a texture value of 1 will correspond to the horizontal spacing between two consecutive channels.

---

## Example

```python
--8<-- "cleaned/visuals/wiggle.py"
```

This example creates a synthetic dataset with 16 channels and 1024 samples, maps it to a texture, and displays the wiggle visual with GUI control for scale.

---

## Summary

The **Wiggle** visual provides an efficient and interactive way to visualize dense multichannel time series using the classic wiggle plot style.

* ✔️ Fast GPU rendering from 2D textures
* ✔️ Separate fill colors for positive/negative values
* ✔️ Adjustable scale, range, and bounds
* ✔️ Ideal for seismic or electrophysiology data

See also:

* [**Basic**](basic.md): for drawing low-level primitives
* [**Segment**](segment.md): for individual lines with custom caps
* [**Image**](image.md): for displaying 2D raster textures
