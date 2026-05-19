# Market Microstructure Dashboard

> **Agent Pickup**
> - **Category:** `dashboards`
> - **Implementation target:** Dense multi-panel example with predictable controls and a synthetic or cached data mode.
> - **Data policy:** Synthetic fallback is mandatory; public or local replay data may be optional.
> - **Preprocessing:** Use a deterministic Python preparation script when real data is downloaded or converted.
> - **Validation:** Bounded smoke run plus manual interaction checklist for linked panels, picking, and updates.


## Summary

Create a Python example demonstrating high-density financial time-series visualization with Datoviz
v0.4. The example should show a realistic market microstructure dashboard combining price, trades,
volume, spread, and order-book liquidity over time.

The example is intended to stress-test the new Datoviz scene architecture with dense 2D rendering,
linked panels, camera interaction, overlays, picking, dynamic data updates, and level-of-detail
logic. It should work out of the box by downloading a prepared dataset from the `datoviz/data`
GitHub repository if the dataset is not already cached locally.

The exact Datoviz v0.4 Python API is not yet fixed. The implementation should therefore use the
actual available v0.4 API while preserving the structure and behavior described here. The API is
expected to be conceptually close to Datoviz v0.3 and to GSP-style scene abstractions. The first
practical slice should load or generate one bounded session, draw the three core panels with shared
horizontal navigation, and add a simple hover or crosshair before replay and advanced LOD.

For broader v0.5+ dashboard architecture ideas that may eventually simplify linked panels,
streaming traces, cursor overlays, telemetry, and high-density LOD paths, see
`spec/scene/dashboards/DASHBOARD_RENDERING_ROADMAP.md`.

Suggested example name:

```text
market_microstructure.py
```

Suggested documentation/spec filename:

```text
MARKET_MICROSTRUCTURE.md
```

---

## Goals

The example should demonstrate:

- High-density time-series visualization with millions of samples.
- Linked 2D panels sharing the same time axis.
- Smooth pan and zoom over dense financial data.
- Multiple visual representations: lines, points, bars, heatmaps, overlays.
- Dynamic level of detail depending on zoom level.
- Interactive crosshair and hover readout.
- Optional animated replay of a trading session.
- Efficient buffer and texture updates without unnecessary CPU/GPU transfers.

This should look like a compact professional market data dashboard, but the objective is not to implement a trading system. The objective is to exercise Datoviz v0.4 visualization architecture under realistic high-density 2D workloads.

---

## Scene Layout

Use a single window with several vertically stacked panels sharing the same X/time axis.

Recommended layout:

```text
+------------------------------------------------------------+
| Main price / trades panel                                  |
| Candles, mid-price line, bid/ask, dense trade points        |
+------------------------------------------------------------+
| Volume / spread panel                                      |
| Volume bars, spread curve, large-trade markers              |
+------------------------------------------------------------+
| Order book depth heatmap                                   |
| Time × price, color = liquidity/depth                       |
+------------------------------------------------------------+
| Optional event / replay timeline                           |
| Session cursor, selected range, annotations                 |
+------------------------------------------------------------+
```

Minimum viable version:

1. Main price panel.
2. Volume/spread panel.
3. Order book heatmap panel.
4. Linked horizontal navigation across all panels.

---

## Data

### Preferred dataset

Use a preprocessed dataset stored in the `datoviz/data` GitHub repository.

Suggested path:

```text
data/finance/market_microstructure_sample.npz
```

The Python example should:

1. Check whether the dataset exists in the local Datoviz cache.
2. If not, download it from `datoviz/data`.
3. Load it with NumPy.
4. Convert arrays into Datoviz resources/visual data.

The example should not depend on a live finance API by default. Live APIs are fragile, rate-limited, sometimes licensed, and may break reproducibility.

### Dataset content

The dataset should contain one trading session for one representative liquid instrument. The data can be real if licensing allows redistribution, or synthetic but realistic if not.

Recommended arrays:

```text
time_ns                 int64, shape (N_trades,)
trade_price             float32, shape (N_trades,)
trade_size              float32, shape (N_trades,)
trade_side              int8, shape (N_trades,)      # -1 sell, +1 buy, 0 unknown

quote_time_ns            int64, shape (N_quotes,)
bid_price               float32, shape (N_quotes,)
ask_price               float32, shape (N_quotes,)
bid_size                float32, shape (N_quotes,)
ask_size                float32, shape (N_quotes,)

ohlc_time_ns             int64, shape (N_bars,)
ohlc_open               float32, shape (N_bars,)
ohlc_high               float32, shape (N_bars,)
ohlc_low                float32, shape (N_bars,)
ohlc_close              float32, shape (N_bars,)
volume                  float32, shape (N_bars,)
vwap                    float32, shape (N_bars,)
spread                  float32, shape (N_bars,)

book_time_ns             int64, shape (N_book_times,)
book_price_levels        float32, shape (N_price_levels,)
book_bid_depth           float32, shape (N_book_times, N_price_levels)
book_ask_depth           float32, shape (N_book_times, N_price_levels)
book_depth               float32, shape (N_book_times, N_price_levels) # signed or combined depth
```

For compactness, the final dataset can omit redundant arrays if the example can derive them cheaply.

### Target dataset scale

The example should be configurable, but the default dataset should be large enough to exercise performance without being too large to download.

Recommended target:

```text
N_trades       ~ 1,000,000 to 5,000,000
N_quotes       ~ 1,000,000 to 10,000,000
N_bars         ~ 20,000 to 100,000
N_book_times   ~ 2,000 to 20,000
N_price_levels ~ 100 to 400
```

The `.npz` file should ideally remain below ~100-200 MB compressed.

### Synthetic fallback

If no real/preprocessed dataset is available yet, implement a deterministic synthetic generator with a fixed random seed.

Synthetic model:

- One trading day, e.g. 6.5 hours.
- Irregular trade timestamps from a non-homogeneous Poisson process.
- Mid-price as a noisy random walk with intraday volatility pattern.
- Bid/ask spread widening during volatility spikes.
- Trade sizes from a heavy-tailed distribution.
- Order-book depth as a 2D time/price field centered around the evolving mid-price.
- Liquidity shocks and temporary gaps around large trades.

The fallback should produce visually plausible market microstructure patterns, not just white noise.

---

## Visuals

### 1. Main price panel

Display:

- OHLC candles or bars.
- Mid-price line.
- Bid and ask lines, optionally thin and semi-transparent.
- Dense trade points.
- Optional VWAP line.

Visual encoding:

```text
X = time
Y = price
trade point size = function(log(trade_size))
trade point color = buy/sell/unknown side
trade point alpha = low to medium, to reveal density
candles = green/up, red/down, or neutral monochrome if color support is limited
```

Recommended implementation notes:

- At full-session zoom, render downsampled/aggregated data.
- At close zoom, render raw trades.
- Use line strips for mid-price, bid, ask, VWAP.
- Use instanced quads or line segments for candles.
- Avoid CPU recomputation every frame; recompute LOD only when camera scale changes significantly.

### 2. Volume / spread panel

Display:

- Volume bars per time bin.
- Spread curve, possibly on a secondary normalized scale.
- Markers for unusually large trades.

Visual encoding:

```text
X = time
Y left = volume
Y right or normalized overlay = spread
bar height = volume
marker size = large trade size
```

This panel should be linked horizontally with the price panel, but have its own Y scale.

### 3. Order book depth heatmap

This is the hero visual.

Display a 2D heatmap:

```text
X = time
Y = price level relative to current or absolute price
color = available liquidity / depth
```

Two possible modes:

#### Absolute price mode

The Y-axis is absolute price. The mid-price moves through the heatmap.

Pros:

- Easy to interpret.
- Naturally linked to price panel.

Cons:

- Price drift may waste vertical space unless the view follows the price.

#### Relative price mode

The Y-axis is relative to mid-price, e.g. basis points or ticks from mid.

Pros:

- Shows book shape clearly.
- Stable and compact.

Cons:

- Less directly aligned with absolute price panel.

Default recommendation: use absolute price mode if the dataset has modest price drift; otherwise use relative price mode and clearly label it.

Heatmap implementation:

- Store the depth matrix as a 2D texture.
- Use an image visual or textured quad.
- Use a transfer function or colormap in the shader.
- Optionally encode bid depth and ask depth with different signs/colors.

Possible color mapping:

```text
zero / no liquidity: near black
bid liquidity: blue/cyan intensity
ask liquidity: orange/red intensity
large liquidity walls: bright highlights
```

The exact colormap can be adapted to Datoviz facilities.

### 4. Overlays

Required overlays:

- Vertical crosshair synchronized across all panels.
- Horizontal crosshair in the active panel.
- Tooltip with timestamp and local values.
- Selected time range rectangle if the user drags.

Optional overlays:

- Session replay cursor.
- Event markers for volatility spikes, large trades, opening/closing auction.
- Minimap of the full trading day.

---

## Interaction

### Camera interaction

All panels should share the same X/time camera range.

Required controls:

```text
mouse drag left/right       pan time
mouse wheel                 zoom around cursor time
shift + wheel or drag       vertical zoom in active panel
double click                reset view
```

Optional controls:

```text
space                       pause/resume replay
left/right arrows            step replay cursor
r                           reset camera
l                           toggle LOD/debug overlay
h                           toggle heatmap mode absolute/relative
```

### Linked panels

When the user pans or zooms horizontally in any panel:

- The visible time interval updates for all panels.
- Each panel keeps its own vertical scale.
- Crosshair X position is shared.

### Picking / hover

On mouse move:

- Find the nearest trade, quote, candle, or heatmap cell under the cursor.
- Display a compact tooltip.

Suggested tooltip fields:

```text
Timestamp: 2024-01-02 10:31:04.123456
Price: 187.42
Bid/Ask: 187.41 / 187.43
Spread: 0.02
Trade size: 300
Volume bin: 12,430
Book depth: 8,200
```

Picking can initially be CPU-side using sorted timestamp arrays and binary search. GPU picking may be added later if the scene API exposes it.

---

## Level of Detail

LOD is important for this example.

At full-session view, raw tick rendering may overdraw too much. The implementation should switch between representations depending on horizontal scale.

Suggested LOD levels:

### LOD 0: Full session

Visible duration: hours.

Render:

- Aggregated OHLC candles.
- Downsampled mid-price line.
- Aggregated volume bars.
- Heatmap at reduced time resolution.
- No raw trade points, or only sampled density points.

### LOD 1: Minutes

Visible duration: several minutes to one hour.

Render:

- Finer OHLC bars.
- More detailed bid/ask lines.
- Decimated trade points.
- Heatmap at medium resolution.

### LOD 2: Seconds / microstructure

Visible duration: seconds to minutes.

Render:

- Raw trades.
- Raw or near-raw bid/ask updates.
- Individual large trades.
- Full-resolution heatmap columns if available.

The LOD policy does not need to be perfect. It should be explicit, robust, and easy to inspect.

---

## Animation / Replay Mode

Add an optional replay mode that scrolls through the trading session.

Behavior:

- A vertical cursor moves from market open to close.
- The visible window follows the cursor after it reaches a configurable margin.
- New data appears progressively, as if the market were live.
- Heatmap columns may be revealed over time.

This mode should exercise partial buffer/texture updates if supported by the v0.4 API.

If partial GPU updates are not yet available, the implementation may simply update CPU-side resources and let the scene/DRP layer upload dirty regions as supported.

---

## Datoviz v0.4 Architecture Pressure Points

This example should intentionally pressure the following parts of Datoviz v0.4:

### Scene and panels

- Multiple stacked panels.
- Shared horizontal camera state.
- Independent vertical scales.
- Per-panel overlays.

### Visual channels

Use a mix of attribute and constant channels:

- Attribute positions for trade points and line vertices.
- Attribute colors or side flags for buy/sell trade classification.
- Constant alpha/stroke width where appropriate.
- Attribute bar heights for volume.

### Resources

- Large CPU-side arrays mapped to GPU buffers.
- Dirty tracking when replay mode updates visible data.
- Texture resources for order-book heatmap.

### Framegraph

The basic version can use a simple render pass per panel.

A more advanced version may use:

- Main color pass.
- Overlay pass.
- Optional picking pass.
- Optional offscreen pass for heatmap colorization or histogram density.

### Picking

Start with CPU picking.

The example should be structured so GPU picking can be tested later:

- Each visual has logical object IDs.
- Trade points and candles can expose item IDs.
- Heatmap cells can map back to time/price/depth values.

### Text and overlays

Use text if available for tooltip and axis labels. If text rendering is not ready, draw the crosshair and print hover values to an ImGui/debug panel.

---

## Suggested Implementation Structure

```text
examples/python/market_microstructure.py
```

Suggested structure:

```python
# Pseudocode only. Adapt to the actual Datoviz v0.4 API.

from pathlib import Path
import numpy as np


def get_cache_dir():
    ...


def download_dataset_if_needed():
    ...


def load_dataset(path):
    ...


def generate_synthetic_dataset(seed=0):
    ...


def build_lod_tables(data):
    ...


def create_scene(data):
    # Create figure/window.
    # Create vertically stacked panels.
    # Create cameras.
    # Create visuals.
    # Upload resources.
    ...


def update_lod(scene, camera_state, data, lod_tables):
    ...


def update_crosshair(scene, mouse_event, data):
    ...


def update_replay(scene, t):
    ...


def main():
    path = download_dataset_if_needed()
    data = load_dataset(path) if path.exists() else generate_synthetic_dataset()
    lod_tables = build_lod_tables(data)
    scene = create_scene(data)
    scene.run()


if __name__ == "__main__":
    main()
```

The final implementation should not expose unnecessary abstractions. Keep the example readable and suitable for documentation.

---

## Dataset Download Behavior

The example should follow this behavior:

1. Determine cache directory:

```text
~/.cache/datoviz/finance/
```

or Datoviz's standard cache directory if available.

2. Check for:

```text
market_microstructure_sample.npz
```

3. If missing, download from a stable URL such as:

```text
https://raw.githubusercontent.com/datoviz/data/main/finance/market_microstructure_sample.npz
```

or the corresponding GitHub release asset if the file is too large for raw GitHub.

4. Verify file size and optionally checksum.

5. Fall back to deterministic synthetic data if download fails.

The fallback is important so the example always runs out of the box, even offline after first use or if the data repository is unavailable.

---

## Visual Quality Requirements

The default rendering should be visually impressive.

Recommended appearance:

- Dark background.
- Subtle grid lines.
- Thin anti-aliased lines where available.
- Alpha-blended dense trade points.
- Bright but not oversaturated heatmap.
- Clear panel separation.
- Minimal UI chrome.

The screenshot should immediately communicate:

- dense time series;
- market activity;
- order-book liquidity structure;
- linked multiscale dashboard.

---

## Performance Targets

On a modern discrete GPU, the example should remain interactive at default dataset size.

Indicative targets:

```text
startup time after dataset is cached: < 3 seconds if possible
pan/zoom interaction:              >= 30 FPS, ideally 60 FPS
raw points supported:              at least 1 million visible points with LOD
heatmap texture:                   at least 2048 x 256 default, larger if supported
memory use:                        reasonable for a showcase example
```

The example should expose a simple scale parameter:

```text
--size small|medium|large
```

Optional command-line parameters:

```text
--synthetic       force synthetic data
--no-replay       disable replay animation
--lod-debug       show current LOD level and visible sample count
--symbol SYMBOL   select symbol if dataset contains several symbols
```

---

## Acceptance Criteria

The example is complete when:

1. It runs with a single command from the Datoviz repository.
2. It downloads or generates data automatically.
3. It displays at least three linked panels.
4. The main panel shows price/trades over time.
5. The volume/spread panel shows aggregated activity.
6. The heatmap panel shows order-book depth or a realistic synthetic equivalent.
7. Mouse pan/zoom works smoothly.
8. Crosshair or hover feedback works.
9. LOD changes visibly and avoids rendering all raw data at inappropriate zoom levels.
10. The code is compact enough to serve as a readable example.

---

## Stretch Goals

These are optional, not required for the first implementation.

### GPU density aggregation

Use compute shaders to aggregate raw trades into screen-space bins or density textures.

This would exercise compute/render interaction, but it is not necessary for the first version.

### Streaming simulation

Append new trades/quotes every frame using ring buffers.

Useful for testing dynamic resources and dirty-region uploads.

### GPU picking

Use an ID buffer to pick individual visual items.

### Multi-symbol comparison

Show two or more instruments with synchronized time axes.

### Offscreen rendering / video export

Render replay mode to a video using the Datoviz video/export infrastructure if available.

---

## Notes for the Implementing Agent

Do not hard-code a speculative Datoviz v0.4 Python API. Use the actual API available at implementation time.

The desired scene semantics are:

- one scene/window;
- several stacked 2D panels;
- linked X camera;
- independent Y camera per panel;
- large buffer-backed visuals;
- image/texture-backed heatmap;
- overlay/crosshair visual;
- optional ImGui controls.

Prefer simple, explicit code over a complex reusable dashboard framework. The example should be easy to inspect, modify, and use as a regression test for Datoviz v0.4.
