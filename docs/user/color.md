# Colormaps

Visky has built-in support for many continuous colormaps and discrete color palettes. It comes with most colormaps provided by the Python libraries matplotlib, bokeh, and colorcet.

## The colormap texture

Visky natively integrates a 256x256 24-bit texture called the **colormap texture**. Each of the non-empty rows of the texture contains either:

- a 256-color continuous colormap,
- a 256-color discrete color palette,
- eight discrete 32-color palettes.

A lot of empty space remains in the texture, which can used for future colormaps or user-defined colormaps.

The texture is always loaded in memory, exactly once on each GPU. It can be shared between different visuals and different canvases. It is used by some visuals.

<!-- The list of colormaps is generated in a mkdocs hook from utils/export_colormap.py -->
