# Visuals


!!! note
    Props marked *uniform* correspond to the params structure available to the shader as a uniform variable. Each uniform param is shared across all vertices of a given visual. Therefore, these props can receive only a single value.


## 2D visuals

### Marker

![](../images/visuals/marker.png)

#### Props

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | marker position |
| `color` | 0 | `cvec4` | marker  color |
| `marker_size` | 0 | `float` | marker size |
| `marker_type` | 0 | `char` | marker type |
| `angle` | 0 | `char` | marker angle, between 0 (0) and 256 (`M_2PI`) excluded |
| `transform` | 0 | `char` | transform enum |
| `color` | 1 | `vec4` | edge color (*uniform*) |
| `line_width` | 0 | `float` | edge line width (*uniform*) |

#### Marker types

| Marker | Value | Image |
| ---- | ---- | ---- |
| `disc` | 0 | ![marker_disc](../images/graphics/marker_disc.png) |
| `asterisk` | 1 | ![marker_asterisk](../images/graphics/marker_asterisk.png) |
| `chevron` | 2 | ![marker_chevron](../images/graphics/marker_chevron.png) |
| `clover` | 3 | ![marker_clover](../images/graphics/marker_clover.png) |
| `club` | 4 | ![marker_club](../images/graphics/marker_club.png) |
| `cross` | 5 | ![marker_cross](../images/graphics/marker_cross.png) |
| `diamond` | 6 | ![marker_diamond](../images/graphics/marker_diamond.png) |
| `arrow` | 7 | ![marker_arrow](../images/graphics/marker_arrow.png) |
| `ellipse` | 8 | ![marker_ellipse](../images/graphics/marker_ellipse.png) |
| `hbar` | 9 | ![marker_hbar](../images/graphics/marker_hbar.png) |
| `heart` | 10 | ![marker_heart](../images/graphics/marker_heart.png) |
| `infinity` | 11 | ![marker_infinity](../images/graphics/marker_infinity.png) |
| `pin` | 12 | ![marker_pin](../images/graphics/marker_pin.png) |
| `ring` | 13 | ![marker_ring](../images/graphics/marker_ring.png) |
| `spade` | 14 | ![marker_spade](../images/graphics/marker_spade.png) |
| `square` | 15 | ![marker_square](../images/graphics/marker_square.png) |
| `tag` | 16 | ![marker_tag](../images/graphics/marker_tag.png) |
| `triangle` | 17 | ![marker_triangle](../images/graphics/marker_triangle.png) |
| `vbar` | 18 | ![marker_vbar](../images/graphics/marker_vbar.png) |


### Axes

![](../images/visuals/axes.png)

#### Tick level

| Index | Level | Description |
| ---- | ---- | ---- |
| 0 | `minor` | minor ticks |
| 1 | `major` | major ticks |
| 2 | `grid` | grid |
| 3 | `lim` | axes delimiters |


#### Graphics

| Index | Graphics | Description |
| ---- | ---- | ---- |
| 0 | `segment` | ticks (minor, major, grid, lim) |
| 1 | `text` | tick labels |


#### Sources

| Type | Index | Graphics | Description |
| ---- | ---- | ---- | ---- |
| `vertex` | 0 | `segment` | vertex buffer for ticks |
| `index` | 0 | `segment` | index buffer for ticks |
| `vertex` | 1 | `text` | vertex buffer for labels |
| `index` | 1 | `text` | index buffer for labels |
| `font_atlas` | 0 | `text` | font atlas for labels |


#### Props

| Type | Index | Type | Graphics | Description |
| ---- | ---- | ---- | ---- | ---- |
| `pos` | any level | `double` | `segment` | tick positions in data coordinates |
| `color` | any level | `cvec4` | `segment` | tick colors |
| `line_width` | any level | `float` | `segment` | tick line width |
| `length` | `minor` | `float` | `segment` | minor tick length |
| `length` | `major` | `float` | `segment` | major tick length |
| `text` | 0 | `str` | `text` | tick labels text |
| `text_size` | 0 | `float` | `text` | tick labels font size |







## 3D visuals

### Mesh

![](../images/visuals/mesh.png)

#### Props

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | line start position |
| `pos` | 1 | `dvec3` | line end position |
| `color` | 0 | `color` | line color |








## Basic visuals

The basic visuals are simpler and more efficient, but they do not support antialiasing.


### Point

The **point** visual is a trimmed-downed version of the **marker** visual. It is based on the `point` primitive.

![](../images/visuals/point.png)

#### Props

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | point position |
| `color` | 0 | `cvec4` | point color |
| `marker_size` | 0 | `float` | point size (*uniform*) |


### Line

![](../images/visuals/line.png)

#### Props

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | line start position |
| `pos` | 1 | `dvec3` | line end position |
| `color` | 0 | `color` | line color |







## Common data

The sources and props below are shared by all builtin visuals.

### Common sources

| Type | Index |  Description |
| ---- | ---- | ---- |
| `mvp` | 0 | `VklMVP` structure with model-view-proj matrices |
| `viewport` | 0 | `VklViewport` structure with viewport info |

### Common props

| Type | Index | Type | Source | Description |
| ---- | ---- | ---- | ---- | ---- |
| `model` | 0 | `mat4` | `mvp` | model transformation matrix |
| `view` | 0 | `mat4` | `mvp` | view transformation matrix |
| `proj` | 0 | `mat4` | `mvp` | proj transformation matrix |
| `time` | 0 | `float` | `mvp` | time since app start, in seconds |


### Visual transform

### Visual clip
