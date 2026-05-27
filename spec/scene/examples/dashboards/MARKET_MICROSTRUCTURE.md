# Market Microstructure Dashboard

> **Example status:** informative pressure test
> **Target:** Python dashboard example
> **Data:** prepared cached session with deterministic synthetic fallback
> **Validation:** smoke, linked-panel, LOD, hover, and performance checks

## Summary

Build a compact professional market-data dashboard with linked price, volume/spread, and
order-book heatmap panels. The example should load a prepared dataset from `datoviz/data` when
available, or generate a deterministic realistic synthetic session. It should not depend on live
finance APIs.

## User-Visible Result

```text
+------------------------------------------------------------+
| Price / trades: candles, mid, bid/ask, dense trade points   |
+------------------------------------------------------------+
| Volume / spread: bars, spread curve, large-trade markers    |
+------------------------------------------------------------+
| Order book depth heatmap: time x price, color = liquidity   |
+------------------------------------------------------------+
| Optional replay timeline / selected range / annotations     |
+------------------------------------------------------------+
```

- Shared horizontal time navigation across all panels.
- Independent vertical scales per panel.
- Crosshair synchronized across panels.
- Hover tooltip with timestamp, price, bid/ask, spread, trade size, volume bin, and book depth.
- Optional replay cursor that moves through the session.

## Feature Pressure Points

- Dense 2D rendering with millions of samples.
- Linked panels and shared X camera state.
- Large buffer-backed line, point, bar, and candle visuals.
- Texture-backed order-book heatmap.
- Dynamic LOD as zoom level changes.
- CPU picking via sorted timestamp arrays, with GPU picking hook later.
- Overlay/crosshair/text paths.
- Optional replay with partial buffer/texture updates.

## Required Data And Resources

Preferred dataset:

```text
data/finance/market_microstructure_sample.npz
```

Recommended arrays:

```text
time_ns int64[N_trades]
trade_price float32[N_trades]
trade_size float32[N_trades]
trade_side int8[N_trades]          # -1 sell, +1 buy, 0 unknown

quote_time_ns int64[N_quotes]
bid_price, ask_price float32[N_quotes]
bid_size, ask_size float32[N_quotes]

ohlc_time_ns int64[N_bars]
ohlc_open, ohlc_high, ohlc_low, ohlc_close float32[N_bars]
volume, vwap, spread float32[N_bars]

book_time_ns int64[N_book_times]
book_price_levels float32[N_price_levels]
book_bid_depth, book_ask_depth float32[N_book_times, N_price_levels]
book_depth float32[N_book_times, N_price_levels]  # signed or combined
```

Target default scale:

```text
N_trades       ~ 1M to 5M
N_quotes       ~ 1M to 10M
N_bars         ~ 20k to 100k
N_book_times   ~ 2k to 20k
N_price_levels ~ 100 to 400
compressed size below ~100-200 MB if practical
```

Synthetic fallback: one 6.5-hour trading day with irregular trades, random-walk mid-price,
intraday volatility pattern, spread widening during spikes, heavy-tailed trade sizes, depth matrix
centered on mid-price, liquidity shocks, and temporary gaps.

## Scene Shape And Runtime Behavior

Visuals:

| Panel | Required visuals |
|---|---|
| Price | OHLC candles/bars, mid-price, bid/ask, trade points, optional VWAP |
| Volume/spread | Volume bars, spread curve, large-trade markers |
| Heatmap | 2D depth texture, absolute or relative price mode |
| Overlays | Synchronized crosshair, active-panel horizontal crosshair, tooltip, range rectangle |

Heatmap defaults:

```text
X = time
Y = absolute price if drift is modest, otherwise price relative to mid
zero liquidity = near black
bid liquidity = blue/cyan
ask liquidity = orange/red
large walls = bright highlights
```

Interactions:

```text
drag left/right       pan time
mouse wheel           zoom around cursor time
shift+wheel or drag   vertical zoom in active panel
double click          reset view
space                 pause/resume replay
h                     toggle heatmap mode
l                     toggle LOD/debug overlay
```

LOD policy:

| LOD | Visible duration | Render |
|---|---|---|
| Full session | hours | aggregated candles, downsampled mid, volume bars, reduced heatmap |
| Minutes | minutes to hour | finer bars, bid/ask lines, decimated trades, medium heatmap |
| Microstructure | seconds to minutes | raw trades, raw or near-raw quotes, full heatmap columns |

Replay mode should reveal data progressively and exercise dirty updates when available.

## Minimal Implementation Target

- Load or generate one bounded session.
- Three linked panels: price/trades, volume/spread, order-book heatmap.
- Shared horizontal camera state and independent Y scales.
- Crosshair or hover readout.
- Coarse LOD switching on camera scale.
- `--synthetic`, `--size small|medium|large`, and optional `--lod-debug`.

## Validation / Acceptance Criteria

- Runs with a single command and resolves data automatically.
- Displays at least three linked panels.
- Price/trades, volume/spread, and heatmap are all nonblank and aligned in time.
- Pan/zoom updates all X ranges smoothly.
- Hover/crosshair returns plausible local values.
- LOD changes visibly and prevents inappropriate raw rendering at full-session zoom.
- Default dataset or synthetic fallback remains interactive; target pan/zoom is at least 30 FPS on
  a modern discrete GPU.
- The code remains compact enough to serve as an example rather than a dashboard framework.

## Links

- [Shared example policies](../POLICIES.md)
- [Dashboard rendering roadmap](../../dashboards/DASHBOARD_RENDERING_ROADMAP.md)
- [Frame plan](../../pipeline/FRAME_PLAN.md)
- [Resource model](../../pipeline/RESOURCE_MODEL.md)
- [Invalidation and caching](../../pipeline/INVALIDATION_AND_CACHING.md)
- [DRP2 specs](../../../drp2/)
