# Visuals

This page lists all visuals currently implemented in the library.

## Technical notes

* In a given visual, a prop is entirely defined by its type and its index. A visual may have multiple props of the same type. For example, the `segment` visual has a first prop `pos` with the segment start position, and another with the segment end position. The tables below specify the role of these indices in each case.
* A **visual source** corresponds to a GPU object holding the data for the visual. Common source types include: vertex buffer, index buffer, uniform buffer, texture. In a given visual, a source is entirely defined by its type and its index. Each prop is typically linked to a given source. Most props correspond either to shader attributes, in which case they are associated with the vertex buffer, or to global variables, in which case they are associated with uniform buffers.
* A visual is composed of one or several **pipelines**: graphics pipelines (or just **graphics**), and optionally compute pipelines (or just **computes**). A graphics pipeline corresponds to a vertex shader, a fragment shader, and possibly other shaders. In a given visual, each pipeline is entirely defined by its type (graphics or compute) and its index. The tables below specify the different pipelines when there are several of them in a given visual. For example, the axes visual contains a `segment` graphics for tick segments, and a `text` graphics for tick labels.
* Props marked *uniform* below can only receive a single value. They correspond to struct fields in a uniform buffer, and they are thus shared across all vertices of a given visual.


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

!!! note
    Marker shapes are computed in real-time in the fragment shader (vector graphics rather than bitmaps). The GLSL code is from [Rougier 2014, *Antialiased 2D Grid, Marker, and Arrow Shaders*](http://jcgt.org/published/0003/04/01/).

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



### Polygon

![](../images/visuals/polygon.png)

This visual currently only uses a basic `triangle` underlying graphics. It performs a triangulation of the polygons with the earcut C++ library by mapbox. Several arbitrary-sized polygons can be specified in the same visual.

#### Props

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | all polygon points, concatenated |
| `length` | 0 | `uint` | polygon lengths, one integer per polygon |
| `color` | 0 | `cvec4` | polygon colors, one per polygon |



### Image

![](../images/visuals/image.png)


#### Props

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | top left position |
| `pos` | 1 | `dvec3` | top right position |
| `pos` | 2 | `dvec3` | bottom right position |
| `pos` | 3 | `dvec3` | bottom left position |
| `texcoords` | 0 | `vec3` | top left texture coordinates |
| `texcoords` | 1 | `vec3` | top right texture coordinates |
| `texcoords` | 2 | `vec3` | bottom right texture coordinates |
| `texcoords` | 3 | `vec3` | bottom left texture coordinates |

#### Sources

| Type | Index | Description |
| ---- | ---- | ---- |
| `vertex` | 0 | vertex buffer |
| `param` | 0 | parameter struct |
| `image` | 0..3 | 2D texture with image #i |



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


#### Sources

| Type | Index | Graphics | Description |
| ---- | ---- | ---- | ---- |
| `vertex` | 0 | `segment` | vertex buffer for ticks |
| `index` | 0 | `segment` | index buffer for ticks |
| `vertex` | 1 | `text` | vertex buffer for labels |
| `index` | 1 | `text` | index buffer for labels |
| `font_atlas` | 0 | `text` | font atlas for labels |







## 3D visuals

### Mesh

![](../images/visuals/mesh.png)

Features:

* Up to four blendable textures
* Up to four lights
* Automatic computation of normals (cross product of face vertices) if the `normal` prop is not provided

#### Props

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | vertex position |
| `normal` | 0 | `vec3` | vertex normal |
| `texcoords` | 0 | `vec2` | texture coordinates |
| `color` | 0 | `cvec4` | color as RGB 3-bytes |
| `alpha` | 0 | `char` | alpha transparency value |
| `index` | 0 | `uint32` | faces, as vertex indices |
| `light_pos` | 0 | `mat4` | light positions (*uniform*) |
| `light_params` | 0 | `mat4` | light coefficients (*uniform*) |
| `view_pos` | 0 | `vec4` | camera position (*uniform*) |
| `texcoefs` | 0 | `vec4` | texture blending coefficients (*uniform*) |
| `clip` | 0 | `vec4` | clip vector (*uniform*) |

!!! warning
    The `texcoords` and `color` props are mutually exclusive. The color has precedence over the texcoords. The mesh vertex struct has no color field, only a texcoord field. When the color prop is set, special texcoords values are computed (packing 3 bytes into the second texture coordinate floating-point number).

#### Sources

| Type | Index | Description |
| ---- | ---- | ---- |
| `vertex` | 0 | vertex buffer (vertices) |
| `index` | 0 | index buffer (faces) |
| `param` | 0 | parameter struct |
| `image` | 0..3 | 2D texture with image #i |



### Volume slice

![](../images/visuals/volume_slice.png)


#### Props

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | top left position |
| `pos` | 1 | `dvec3` | top right position |
| `pos` | 2 | `dvec3` | bottom right position |
| `pos` | 3 | `dvec3` | bottom left position |
| `texcoords` | 0 | `vec3` | top left texture coordinates |
| `texcoords` | 1 | `vec3` | top right texture coordinates |
| `texcoords` | 2 | `vec3` | bottom right texture coordinates |
| `texcoords` | 3 | `vec3` | bottom left texture coordinates |
| `transfer_x` | 0 | `vec4` | colormap transfer function, x values (*uniform*) |
| `transfer_y` | 0 | `vec4` | colormap transfer function, y values (*uniform*) |
| `transfer_x` | 1 | `vec4` | alpha transfer function, x values (*uniform*) |
| `transfer_y` | 1 | `vec4` | alpha transfer function, y values (*uniform*) |
| `colormap` | 0 | `int` | colormap enum (*uniform*) |
| `scale` | 0 | `float` | volume value scaling factor (*uniform*) |

#### Sources

| Type | Index | Description |
| ---- | ---- | ---- |
| `vertex` | 0 | vertex buffer |
| `param` | 0 | parameter struct |
| `color_texture` | 0 | 2D texture with the colormap texture |
| `volume` | 0 | 3D texture with the volume |





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


### Line strip

![](../images/visuals/line_strip.png)

#### Props

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | point position |
| `color` | 0 | `color` | point color |


### Triangle

![](../images/visuals/triangle.png)

#### Props

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | triangle position 0 |
| `pos` | 1 | `dvec3` | triangle position 1 |
| `pos` | 2 | `dvec3` | triangle position 2 |
| `color` | 0 | `color` | triangle color |

!!! note
    The line strip is normally a single contiguous line. In this example, we use the following trick to show 2 apparently disjoint lines (the same trick would work with an arbitrary number of lines): we add an additional duplicate point at the end of the first line, and at the beginning of the second line, and both of these duplicate points have an alpha value of 0. The line joining these two points is therefore invisible. This trick allows us to use a single drawing command rather than 2 (or N).

### Triangle strip

![](../images/visuals/triangle_strip.png)

#### Props

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | point position |
| `color` | 0 | `color` | point color |


### Triangle fan

![](../images/visuals/triangle_fan.png)

#### Props

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | point position |
| `color` | 0 | `color` | point color |





## Common data

The sources and props below are shared by all builtin visuals.

### Common sources

| Type | Index |  Description |
| ---- | ---- | ---- |
| `mvp` | 0 | `DvzMVP` structure with model-view-proj matrices |
| `viewport` | 0 | `DvzViewport` structure with viewport info |

### Common props

| Type | Index | Type | Source | Description |
| ---- | ---- | ---- | ---- | ---- |
| `model` | 0 | `mat4` | `mvp` | model transformation matrix |
| `view` | 0 | `mat4` | `mvp` | view transformation matrix |
| `proj` | 0 | `mat4` | `mvp` | proj transformation matrix |
| `time` | 0 | `float` | `mvp` | time since app start, in seconds |


## Common enums

### Visual transform

### Visual clip

### Data types

| Data type | Component size | Type size | Description |
| ---- | ---- | ---- | ---- |
| `char`  | 8 |  8 | 1 byte (unsigned byte) |
| `cvec2` | 8 | 16 | 2 bytes |
| `cvec3` | 8 | 24 | 3 bytes |
| `cvec4` | 8 | 32 | 4 bytes |
| `ushort` | 16 | 16 | 1 unsigned short integer |
| `usvec2` | 16 | 32 | 2 ushort |
| `usvec3` | 16 | 48 | 3 ushort |
| `usvec4` | 16 | 64 | 4 ushort |
| `short` | 16 | 16 | 1 signed short integer |
| `svec2` | 16 | 32 | 2 short |
| `svec3` | 16 | 48 | 3 short |
| `svec4` | 16 | 64 | 4 short |
| `uint` | 32 | 32 | 1 unsigned long integer |
| `uvec2` | 32 | 64 | 2 uint |
| `uvec3` | 32 | 96 | 3 uint |
| `uvec4` | 32 | 128 | 4 uint |
| `int` | 32 | 32 | 1 long integer |
| `ivec2` | 32 | 64 | 2 int |
| `ivec3` | 32 | 96 | 3 int |
| `ivec4` | 32 | 128 | 4 int |
| `float` | 32 | 32 | 1 single-precision floating-point number |
| `vec2` | 32 | 64 | 2 float |
| `vec3` | 32 | 96 | 3 float |
| `vec4` | 32 | 128 | 4 float |
| `double` | 64 | 64 | 1 double-precision floating-point number |
| `dvec2` | 64 | 128 | 2 double |
| `dvec3` | 64 | 192 | 3 double |
| `dvec4` | 64 | 256 | 4 double |
| `mat2` | 32 | 128 | 2x2 matrix of floats |
| `mat3` | 32 | 288 | 3x3 matrix of floats |
| `mat4` | 32 | 512 | 4x4 matrix of floats |
| `custom` | - | - | used by structured/record arrays (heterogeneous types) |
| `str` | 64 | 64 | pointer to `char` |
