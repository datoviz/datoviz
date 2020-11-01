# Colormaps

Visky has built-in support for many continuous colormaps and discrete color palettes. It comes with most colormaps provided by the Python libraries matplotlib, bokeh, and colorcet.

## The colormap texture

Visky natively integrates a 256x256 24-bit texture called the **colormap texture**. Each of the non-empty rows of the texture contains either one 256-color colormap, or 8 discrete 32-color palettes. A lot of empty space remains in the texture, which can used for future colormaps or user-defined colormaps.

When using the Scene API, this texture is automatically loaded both on the CPU and on the GPU. For the sake of efficient GPU memory usage, the same GPU texture buffer is shared by all visuals and all canvases of an application. Visuals that follow the standard Visky visual convention (which is the case for all built-in visuals) have a reserved binding slot (descriptor set) for this texture.
