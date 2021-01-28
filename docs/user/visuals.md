# Visuals


## Common sources and props

### Common sources

| Type | Index |  Description |
| ---- | ---- | ---- | ---- |
| `mvp` | 0 | `VklMVP` structure with model-view-proj matrices |
| `viewport` | 0 | `VklViewport` structure with viewport info |


### Common props

| Type | Index | Type | Source | Description |
| ---- | ---- | ---- | ---- | ---- |
| `model` | 0 | `mat4`| `mvp` | model transformation matrix |
| `view` | 0 | `mat4`| `mvp` | view transformation matrix |
| `proj` | 0 | `mat4`| `mvp` | proj transformation matrix |
| `time` | 0 | `float`| `mvp` | time since app start, in seconds |



## Axes

### Tick level

| Index | Level | Description |
| ---- | ---- | ---- |
| 0 | `minor` | minor ticks |
| 1 | `major` | major ticks |
| 2 | `grid` | grid |
| 3 | `lim` | axes delimiters |


### Graphics

| Index | Graphics | Description |
| ---- | ---- | ---- |
| 0 | `segment` | ticks (minor, major, grid, lim) |
| 1 | `text` | tick labels |


### Sources

| Type | Index | Graphics | Description |
| ---- | ---- | ---- | ---- |
| `vertex` | 0 | `segment` | vertex buffer for ticks |
| `index` | 0 | `segment` | index buffer for ticks |
| `vertex` | 1 | `text` | vertex buffer for labels |
| `index` | 1 | `text` | index buffer for labels |
| `font_atlas` | 0 | `text` | font atlas for labels |



### Props

| Type | Index | Type | Graphics | Description |
| ---- | ---- | ---- | ---- | ---- | ---- |
| `pos` | any level | `double`| `segment` | tick positions in data coordinates |
| `color` | any level | `cvec4`| `segment` | tick colors |
| `line_width` | any level | `float`| `segment` | tick line width |
| `length` | `minor` | `float`| `segment` | minor tick length |
| `length` | `major` | `float`| `segment` | major tick length |
| `text` | 0 | `str`| `text` | tick labels text |
| `text_size` | 0 | `float`| `text` | tick labels font size |
