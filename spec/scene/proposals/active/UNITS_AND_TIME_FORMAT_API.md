> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-06-02`
> - **Purpose:** define the v0.4 direction for shared axis and scale-bar units, duration labels,
>   absolute datetime axes, and long-term formatting APIs.

# Units And Time Format API

This proposal records the desired public API shape for unit-aware axes and scale bars. It is driven
by the v0.4 scale-bar examples and by the need to avoid surprising labels such as `5 cs` on time
axes.


## Design Goals

1. One unit-formatting system must serve both axes and scale bars.
2. Numeric physical/domain units, elapsed durations, and absolute datetimes must be separate
   concepts.
3. Unit formatting must be inspectable, deterministic, serializable, and testable.
4. Common cases should use builtins; advanced users should be able to define unit ladders without
   writing callbacks.
5. The most common metric-length examples should not need to spell out ladder construction.
6. Callbacks may exist later, but should not be the primary v0.4 answer.
7. The teaching API should avoid deeply nested public descriptors with `struct_size` in ordinary
   examples.


## Concepts

### Unit Ladder

A `DvzUnitLadder` maps numeric magnitudes to display unit strings.

Each entry stores:

1. `factor`: display-unit factor in canonical units,
2. `label`: display unit string,
3. optional preference metadata later.

Examples:

```c
DvzUnitLadder* metric = dvz_unit_ladder_create(scene, "m");
dvz_unit_ladder_add(metric, 1e-9, "nm");
dvz_unit_ladder_add(metric, 1e-6, "um");
dvz_unit_ladder_add(metric, 1e-3, "mm");
dvz_unit_ladder_add(metric, 1.0,  "m");
dvz_unit_ladder_add(metric, 1e3,  "km");
```

```c
DvzUnitLadder* genome = dvz_unit_ladder_create(scene, "bp");
dvz_unit_ladder_add(genome, 1.0, "bp");
dvz_unit_ladder_add(genome, 1e3, "kb");
dvz_unit_ladder_add(genome, 1e6, "Mb");
```

`centi` is not part of the default metric ladder unless a user explicitly adds it. This avoids
labels such as `5 cs` for elapsed time.


### Units

`DvzUnits` binds panel data coordinates to canonical units and a display ladder.

```c
DvzUnits* units = dvz_units_create(scene);
dvz_units_data_to_canonical(units, 1e-6);
dvz_units_ladder(units, metric);
```

Interpretation:

```text
data value -> canonical value -> display value + unit label
```

If data coordinates are micrometers and canonical units are meters:

```c
dvz_units_data_to_canonical(units, 1e-6);
dvz_units_ladder(units, metric);
```


### Duration Format

Elapsed time is not just an SI unit ladder because users expect `ms`, `s`, `min`, and `h`, not
`cs`. Datoviz should provide a duration-specific builtin ladder:

```c
DvzUnitLadder* duration = dvz_unit_ladder_builtin(scene, DVZ_UNIT_LADDER_DURATION);
```

Initial builtin duration entries:

```text
ns: 1e-9 s
us: 1e-6 s
ms: 1e-3 s
s:  1 s
min: 60 s
h:  3600 s
```

For a signal whose X coordinates are milliseconds:

```c
DvzUnits* time_units = dvz_units_create(scene);
dvz_units_data_to_canonical(time_units, 1e-3);
dvz_units_ladder(time_units, duration);
```


### Datetime Format

Absolute datetimes require a separate API. Calendar ticks cannot be represented correctly as a
decimal unit ladder because months, years, daylight-saving transitions, and time zones are calendar
concepts.

Recommended representation:

```c
typedef int64_t DvzTimestamp;
```

`DvzTimestamp` should be integer time since Unix epoch. The implementation must choose and document
one resolution, preferably nanoseconds if the runtime and bindings can support it safely, otherwise
microseconds.

Datetime axes use calendar-aware locators and scale-dependent format rules:

```c
DvzDateTimeFormat* dt = dvz_datetime_format_create(scene);
dvz_datetime_format_timezone(dt, "UTC");
dvz_datetime_format_rule(dt, DVZ_TIME_INTERVAL_SECOND, "%H:%M:%S");
dvz_datetime_format_rule(dt, DVZ_TIME_INTERVAL_MINUTE, "%H:%M");
dvz_datetime_format_rule(dt, DVZ_TIME_INTERVAL_DAY, "%b %d");
dvz_datetime_format_rule(dt, DVZ_TIME_INTERVAL_YEAR, "%Y");
```

Scale bars on datetime axes should display durations, not absolute dates.


## Public API Sketch

### Builtin Ladders

```c
typedef enum DvzUnitLadderBuiltin
{
    DVZ_UNIT_LADDER_METRIC_LENGTH,
    DVZ_UNIT_LADDER_DURATION,
    DVZ_UNIT_LADDER_RAW,
} DvzUnitLadderBuiltin;

DvzUnitLadder* dvz_unit_ladder_builtin(DvzScene* scene, DvzUnitLadderBuiltin builtin);
```

Use an intermediate variable in examples:

```c
DvzUnitLadder* duration = dvz_unit_ladder_builtin(scene, DVZ_UNIT_LADDER_DURATION);
```

### Builtin Units Helpers

Common examples should not need to show the full ladder/object setup. A builtin-units helper creates
a scene-owned `DvzUnits`, attaches the requested builtin ladder, and sets the data-to-canonical
factor in one call:

```c
DvzUnits* dvz_units_builtin(
    DvzScene* scene, DvzUnitLadderBuiltin builtin, double data_to_canonical);
```

This is equivalent to:

```c
DvzUnitLadder* ladder = dvz_unit_ladder_builtin(scene, builtin);
DvzUnits* units = dvz_units_create(scene);
dvz_units_data_to_canonical(units, data_to_canonical);
dvz_units_ladder(units, ladder);
```

Teaching examples should use this helper for default metric length and other common builtin cases.
Custom ladders still use the explicit ladder API.


### Custom Ladders

```c
DvzUnitLadder* dvz_unit_ladder_create(DvzScene* scene, const char* canonical_unit);
int dvz_unit_ladder_add(DvzUnitLadder* ladder, double factor, const char* label);
void dvz_unit_ladder_clear(DvzUnitLadder* ladder);
```

Rules:

1. `factor` must be finite and positive.
2. `label` is copied into scene-owned storage.
3. entries are sorted by factor before formatting, regardless of insertion order.
4. duplicate labels or duplicate factors should replace the existing entry or fail with a clear
   diagnostic; choose one policy and test it.


### Units Object

```c
typedef enum DvzUnitDisplayMode
{
    DVZ_UNIT_DISPLAY_AUTO,
    DVZ_UNIT_DISPLAY_AXIS_STABLE,
    DVZ_UNIT_DISPLAY_FIXED,
} DvzUnitDisplayMode;

DvzUnits* dvz_units_create(DvzScene* scene);
int dvz_units_data_to_canonical(DvzUnits* units, double factor);
int dvz_units_ladder(DvzUnits* units, DvzUnitLadder* ladder);
int dvz_units_display_mode(DvzUnits* units, DvzUnitDisplayMode mode);
int dvz_units_fixed_label(DvzUnits* units, const char* label);
```

Display modes:

1. `AUTO`: choose the best ladder entry for each formatted value. Good for scale bars.
2. `AXIS_STABLE`: choose one ladder entry for the whole axis range. Default for axes.
3. `FIXED`: force a named ladder entry. Useful when users want all labels in `ms`, `mm`, or `kb`.


### Axes

```c
int dvz_axis_set_units(DvzAxis* axis, DvzUnits* units);
int dvz_axis_set_datetime(DvzAxis* axis, DvzDateTimeFormat* format);
int dvz_axis_set_datetime_range(
    DvzAxis* axis, double data0, double data1, DvzTimestamp t0, DvzTimestamp t1);
```

Numeric-unit axis behavior:

1. tick positions remain in data coordinates,
2. labels are formatted through the attached `DvzUnits`,
3. default unit display mode is `DVZ_UNIT_DISPLAY_AXIS_STABLE`.

Datetime axis behavior:

1. tick positions remain in ordinary panel data coordinates,
2. `data0 -> t0` and `data1 -> t1` define the affine mapping from data coordinates to absolute
   timestamps,
3. tick generation uses calendar-aware intervals in timestamp space,
4. labels are formatted through `DvzDateTimeFormat`,
5. time zone and calendar format are explicit.

This avoids forcing large Unix-epoch timestamp integers through visual-position data paths. Users
can plot elapsed seconds, samples, or another compact numeric coordinate while the axis displays
absolute UTC time.


### Scale Bars

Long-term retained-object API:

```c
DvzScaleBar* dvz_scalebar(DvzPanel* panel);
int dvz_scalebar_set_dimension(DvzScaleBar* scalebar, DvzDim dim);
int dvz_scalebar_set_anchor(DvzScaleBar* scalebar, DvzSceneAnchor anchor);
int dvz_scalebar_set_units(DvzScaleBar* scalebar, DvzUnits* units);
int dvz_scalebar_set_duration_units(DvzScaleBar* scalebar, DvzUnits* duration_units);
```

Existing descriptor bridge:

```c
DvzAnnotation* dvz_scalebar(DvzPanel* panel, const DvzScaleBarDesc* desc);
```

The descriptor path should remain available in v0.4. Internally, `.unit + .data_to_unit` should be
translated into a default `DvzUnits` object if no explicit units object is attached.

Scale-bar behavior:

1. selected length is computed in canonical units,
2. label display uses attached `DvzUnits`,
3. default unit display mode is `DVZ_UNIT_DISPLAY_AUTO`,
4. datetime axes use duration units for scale bars.


### Datetime Format

```c
typedef enum DvzTimeInterval
{
    DVZ_TIME_INTERVAL_NANOSECOND,
    DVZ_TIME_INTERVAL_MICROSECOND,
    DVZ_TIME_INTERVAL_MILLISECOND,
    DVZ_TIME_INTERVAL_SECOND,
    DVZ_TIME_INTERVAL_MINUTE,
    DVZ_TIME_INTERVAL_HOUR,
    DVZ_TIME_INTERVAL_DAY,
    DVZ_TIME_INTERVAL_MONTH,
    DVZ_TIME_INTERVAL_YEAR,
} DvzTimeInterval;

typedef enum DvzDateTimeBuiltin
{
    DVZ_DATETIME_FORMAT_CONCISE_UTC,
    DVZ_DATETIME_FORMAT_ISO_UTC,
} DvzDateTimeBuiltin;

DvzDateTimeFormat* dvz_datetime_format_builtin(DvzScene* scene, DvzDateTimeBuiltin builtin);
DvzDateTimeFormat* dvz_datetime_format_create(DvzScene* scene);
int dvz_datetime_format_timezone(DvzDateTimeFormat* format, const char* timezone);
int dvz_datetime_format_rule(
    DvzDateTimeFormat* format, DvzTimeInterval interval, const char* strftime_format);
```

The first v0.4 implementation may support only UTC. If local time zones are deferred, unsupported
time zones must emit clear diagnostics.


## Example Shapes

### Metric Spatial Scale Bar

```c
DvzUnits* length_units =
    dvz_units_builtin(scene, DVZ_UNIT_LADDER_METRIC_LENGTH, 1e-3); // data mm -> canonical m

DvzScaleBar* sb = dvz_scalebar(panel, NULL);
dvz_scalebar_set_units(sb, length_units);
```


### Time-Series Axis And Scale Bar

```c
DvzUnitLadder* duration = dvz_unit_ladder_builtin(scene, DVZ_UNIT_LADDER_DURATION);

DvzUnits* time_units = dvz_units_create(scene);
dvz_units_data_to_canonical(time_units, 1e-3); // data units are ms, canonical s
dvz_units_ladder(time_units, duration);

DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
dvz_axis_set_units(x_axis, time_units);

DvzScaleBar* sb = dvz_scalebar(panel, NULL);
dvz_scalebar_set_units(sb, time_units);
```


### Genomics Axis

```c
DvzUnitLadder* genome = dvz_unit_ladder_create(scene, "bp");
dvz_unit_ladder_add(genome, 1.0, "bp");
dvz_unit_ladder_add(genome, 1e3, "kb");
dvz_unit_ladder_add(genome, 1e6, "Mb");

DvzUnits* bp_units = dvz_units_create(scene);
dvz_units_data_to_canonical(bp_units, 1.0);
dvz_units_ladder(bp_units, genome);
dvz_units_display_mode(bp_units, DVZ_UNIT_DISPLAY_AXIS_STABLE);

dvz_axis_set_units(x_axis, bp_units);
```


### Datetime Axis With Duration Scale Bar

```c
DvzDateTimeFormat* dt = dvz_datetime_format_builtin(scene, DVZ_DATETIME_FORMAT_CONCISE_UTC);
dvz_axis_set_datetime(x_axis, dt);
dvz_axis_set_datetime_range(x_axis, 0.0, 3600.0, t0, t1);

DvzUnitLadder* duration = dvz_unit_ladder_builtin(scene, DVZ_UNIT_LADDER_DURATION);
DvzUnits* duration_units = dvz_units_create(scene);
dvz_units_data_to_canonical(duration_units, 1.0);
dvz_units_ladder(duration_units, duration);

dvz_scalebar_set_units(sb, duration_units);
```


## Public Example Set

Three public feature examples are enough for the v0.4 teaching surface:

1. `examples/c/features/scalebar.c`: minimal metric spatial scale bar. Use
   `dvz_units_builtin(scene, DVZ_UNIT_LADDER_METRIC_LENGTH, data_to_m)` so the example stays focused
   on attaching a scale bar, not on unit-ladder mechanics.
2. `examples/c/features/scalebar_units.c`: time-series axis and scale bar. Data coordinates are
   milliseconds, axis labels use stable duration units, and the scale bar uses automatic duration
   labels such as `50 ms`, never SI centi labels such as `5 cs`.
3. `examples/c/features/datetime_axis.c`: absolute UTC datetime axis with an elapsed-duration scale
   bar. Data coordinates stay compact, for example elapsed seconds from `0..3600`, and
   `dvz_axis_set_datetime_range()` maps them to absolute timestamps.

A custom-ladder example such as a genomics `bp/kb/Mb` axis should remain optional. Add it only if
custom ladders are promoted as a first-class v0.4 public teaching surface; otherwise keep custom
ladders in docs and formatter tests.


## Implementation Plan

### Phase 1: Formatting Core

1. Add scene-owned `DvzUnitLadder` storage.
2. Add scene-owned `DvzUnits` storage.
3. Implement `dvz_unit_ladder_builtin()` for metric length, duration, and raw units.
4. Implement `dvz_units_builtin()` as a convenience wrapper for common builtin-unit cases.
5. Implement custom ladder insertion, validation, sorting, and replacement/failure policy.
6. Add a shared formatter:

```c
bool _scene_units_format(
    const DvzUnits* units, double data_value, const DvzUnitFormatContext* context,
    char* out, size_t out_size);
```

7. Add tests for metric labels, duration labels, custom ladders, fixed display labels, and
   `AXIS_STABLE` behavior.


### Phase 2: Scale Bar Integration

1. Keep `DvzScaleBarDesc` public.
2. Add an internal `DvzUnits*` pointer or id to scale-bar retained state.
3. Translate existing `.unit + .data_to_unit` into a default units object when no explicit units are
   set.
4. Replace `_scene_format_si_value()` usage in scale bars with `_scene_units_format()`.
5. Remove `c` from default automatic scale-bar formatting unless the active ladder explicitly
   contains it.
6. Update `examples/c/features/scalebar_units.c` to use the duration builtin once available.
7. Add tests that reject `5 cs` for the duration builtin and produce `50 ms`.


### Phase 3: Axis Integration

1. Add `DvzUnits*` to axis retained state.
2. Format tick labels through `_scene_units_format()`.
3. Use `DVZ_UNIT_DISPLAY_AXIS_STABLE` by default for axes.
4. Add tests where `0..250` data units with `data_to_canonical = 1e-3` format as stable
   millisecond labels.
5. Keep existing numeric axis behavior unchanged when no units are attached.


### Phase 4: Datetime Axis

1. Add `DvzTimestamp` typedef and choose nanosecond or microsecond resolution.
2. Add `DvzDateTimeFormat` retained object and UTC-only builtin formats first.
3. Implement calendar-aware tick interval selection for UTC.
4. Add `dvz_axis_set_datetime()` and `dvz_axis_set_datetime_range()`.
5. Keep scale bars duration-based on datetime axes.
6. Add tests across second, minute, day, month, and year ranges.


### Phase 5: Documentation And Examples

1. Update scale-bar feature docs to explain unit ladders and duration labels.
2. Add or update examples:
   - `features/scalebar.c`: minimal metric scale bar,
   - `features/scalebar_units.c`: duration ladder on a time series,
   - later `features/datetime_axis.c`: absolute datetime axis.
3. Keep custom-ladder examples optional unless they are promoted as first-class v0.4 teaching
   material.
4. Keep callback formatting deferred unless a concrete domain example cannot be represented by
   ladders.


## Migration Notes

1. Existing `.unit + .data_to_unit` descriptors remain valid.
2. Existing `DvzFormatDesc` remains useful for precision, prefix, and suffix, but should not be the
   only way to express unit systems.
3. Public examples should prefer retained objects and setters over deeply nested descriptor
   literals.
4. Descriptor constructors such as `dvz_scalebar_desc()` remain appropriate for ABI-oriented C
   callers, tests, and generated bindings.


## Open Decisions

Preferred v0.4 decisions:

1. Use **microseconds since Unix epoch UTC** for `DvzTimestamp`.
   This is precise enough for absolute datetime axes and works cleanly across C, Python, WASM/JS,
   serialization, and raw binding smoke tests. Nanosecond-resolution data should be plotted as
   compact relative numeric coordinates with duration units unless a future API explicitly promotes
   nanosecond timestamps.
2. Ship **UTC-only timezone support** in v0.4.
   Accept `"UTC"` and reject unsupported timezone strings with a clear diagnostic. IANA timezone and
   local-time support should be deferred until the calendar implementation needs them.
3. **Fail on duplicate custom ladder labels or factors**.
   Returning `-1` with a diagnostic is easier to reason about than replacement because ladder
   construction order cannot silently change the final mapping. Add an explicit update API later if
   replacement becomes useful.
4. Provide two builtin datetime formats:
   - `DVZ_DATETIME_FORMAT_CONCISE_UTC`: scale-dependent axis labels,
   - `DVZ_DATETIME_FORMAT_ISO_UTC`: stable UTC labels for tests, readouts, and exact diagnostics.
5. Start `DVZ_DATETIME_FORMAT_CONCISE_UTC` with these scale-dependent rules:
   - sub-second: `HH:MM:SS.ffffff`,
   - second: `%H:%M:%S`,
   - minute/hour: `%H:%M`,
   - day: `%b %d`,
   - month: `%Y-%m`,
   - year: `%Y`.
   Fractional seconds should be formatted by Datoviz because C `strftime()` does not standardize a
   portable `%f` directive.
6. Land the retained-object `DvzScaleBar*` API with the units work if implementation time permits.
   Keep `dvz_scalebar()` as the descriptor bridge for ABI-oriented C callers, tests, and
   generated bindings. If implementation time gets tight, the retained API may bridge internally to
   `DvzAnnotation*`, but public teaching examples should use the retained setter path.
