# Colormaps

Datoviz natively includes a collection of common colormaps, both continuous and discrete (color palettes). These colormaps come from the following sources:

* matplotlib
* bokeh
* colorcet
* [Kenneth Moreland's page](https://www.kennethmoreland.com/color-advice/)

![Colormap texture](../images/color_texture.png){: align=right }
These colormaps are stored in a 256x256 texture. Each row contains either:

* a 256-color continuous colormap,
* a 256-color discrete color palette,
* eight discrete 32-color palettes.

Unused space may be used for future or user-defined colormaps. The texture is always loaded both in CPU and GPU memory. It is shared between all visuals and canvases.

Datoviz provides a few functions to easily make colors out of scalar values:

=== "Python"
    ```python
    import numpy as np
    from datoviz import colormap

    values = np.random.rand(1000)
    colors = colormap(values, vmin=0, vmax=1, cmap='viridis')
    print(colors)

    # output:
    # [[126 210  78 255]
    #  [ 64  68 135 255]
    #  [ 36 170 130 255]
    #  ...
    #  [ 36 132 141 255]
    #  [ 61  75 137 255]
    #  [ 31 148 139 255]]
    ```

=== "C"
    ```c
    DvzColormap cmap = DVZ_CMAP_VIRIDIS;
    cvec4 color = {0};
    uint8_t value = 128;
    double dvalue = .5;

    // Get a single color from a byte.
    dvz_colormap(cmap, 128, color);

    // Get a single color from a double, with a custom vmin-vmax range.
    dvz_colormap_scale(cmap, dvalue, 0, 1, color);

    // Get an array of colors from an array of values.
    const uint32_t N = 10;
    double* values = calloc(N, sizeof(double));
    cvec4* colors = calloc(N, sizeof(cvec4));
    dvz_colormap_array(cmap, N, values, 0, 1, colors);
    FREE(values);
    FREE(colors);
    ```

## List of colormaps and color palettes

!!! note
    The row and col give the offset of the colormap or color palette within the 256x256 colormap texture.

<!-- The list of colormaps is generated in a mkdocs hook from utils/export_colormap.py -->
