/*************************************************************************************************/
/*  Panels organized within a 2D grid layout (subplots)                                          */
/*************************************************************************************************/

#ifndef VKL_PANEL_HEADER
#define VKL_PANEL_HEADER

#include "transforms.h"
#include "visuals.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_GRID_MAX_COLS         64
#define VKL_GRID_MAX_ROWS         64
#define VKL_MAX_PANELS            1024
#define VKL_MAX_VISUALS_PER_PANEL 64

// Group index of the set of panel VklCommands objects.
#define VKL_COMMANDS_GROUP_PANELS 1



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Panel mode.
typedef enum
{
    VKL_PANEL_GRID,
    VKL_PANEL_INSET,
    VKL_PANEL_FLOATING,
} VklPanelMode;



// Grid axis.
typedef enum
{
    VKL_GRID_HORIZONTAL,
    VKL_GRID_VERTICAL,
} VklGridAxis;



// Size unit.
typedef enum
{
    VKL_PANEL_UNIT_NORMALIZED,
    VKL_PANEL_UNIT_FRAMEBUFFER,
    VKL_PANEL_UNIT_SCREEN,
} VklPanelSizeUnit;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklGrid VklGrid;
typedef struct VklPanel VklPanel;
typedef struct VklController VklController;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklPanel
{
    VklObject obj;
    VklScene* scene;

    VklGrid* grid;
    VklPanelMode mode;
    VklPanelSizeUnit size_unit; // the unit x, y, width, height are in
    VklDataCoords data_coords;  // data CPU transformation

    // User-specified:
    uint32_t row, col;
    uint32_t hspan, vspan;

    // Computed automatically from above and grid widths and heights:
    // Unless using inset
    float x, y;
    float width, height;

    // Visuals
    uint32_t visual_count;
    VklVisual* visuals[VKL_MAX_VISUALS_PER_PANEL];

    // Viewport.
    VklViewport viewport;

    // GPU objects
    VklBufferRegions br_mvp; // for the uniform buffer containing the MVP

    VklController* controller;
    VklCommands* cmds;
    int prority_max;
};



struct VklGrid
{
    VklCanvas* canvas;
    uint32_t n_rows, n_cols;

    // In normalized coordinates (in [0, 1]):
    float xs[VKL_GRID_MAX_COLS];
    float ys[VKL_GRID_MAX_ROWS];
    double widths[VKL_GRID_MAX_COLS];
    double heights[VKL_GRID_MAX_ROWS];

    VklContainer panels;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a grid of panels.
 *
 * @param canvas the canvas
 * @param row_count the number of rows
 * @param col_count the number of columns
 * @returns the grid object
 */
VKY_EXPORT VklGrid vkl_grid(VklCanvas* canvas, uint32_t row_count, uint32_t col_count);

/**
 * Destroy a grid.
 *
 * @param grid the grid
 */
VKY_EXPORT void vkl_grid_destroy(VklGrid* grid);

/**
 * Create a panel at a given location in a grid.
 *
 * @param grid the grid
 * @param row the row index in the grid
 * @param col the column index in the grid
 */
VKY_EXPORT VklPanel* vkl_panel(VklGrid* grid, uint32_t row, uint32_t col);

/**
 * Update a panel viewport.
 *
 * @param panel the panel
 */
VKY_EXPORT void vkl_panel_update(VklPanel* panel);

/**
 * Set panel margins.
 *
 * Margins are represented as a vec4 vector: top, right, bottom, left.
 *
 * @param panel the panel
 * @param margins the margins, in pixels
 */
VKY_EXPORT void vkl_panel_margins(VklPanel* panel, vec4 margins);

/**
 * Set the DPI scaling factor for a panel.
 *
 * @param panel the panel
 * @param scaling the scaling factor (1 by default)
 */
VKY_EXPORT void vkl_panel_dpi_scaling(VklPanel* panel, float scaling);

/**
 * Set the unit in which the panel size is specified.
 *
 * @param panel the panel
 * @param unit the unit
 */
VKY_EXPORT void vkl_panel_unit(VklPanel* panel, VklPanelSizeUnit unit);

/**
 * Set the panel mode (grid or detached).
 *
 * @param panel the panel
 * @param mode the mode
 */
VKY_EXPORT void vkl_panel_mode(VklPanel* panel, VklPanelMode mode);

/**
 * Add a visual to a panel.
 *
 * @param panel the panel
 * @param visual the visual
 */
VKY_EXPORT void vkl_panel_visual(VklPanel* panel, VklVisual* visual);

/**
 * Set a panel position (in detached mode).
 *
 * The unit in which the coordinates are specified is controller by `vkl_panel_unit()`.
 *
 * @param panel the panel
 * @param x the position
 * @param y the position
 */
VKY_EXPORT void vkl_panel_pos(VklPanel* panel, float x, float y);

/**
 * Set a panel size (in detached mode).
 *
 * The unit in which the size is specified is controller by `vkl_panel_unit()`.
 *
 * @param panel the panel
 * @param axis the axis on which to specify the size
 * @param size the size
 */
VKY_EXPORT void vkl_panel_size(VklPanel* panel, VklGridAxis axis, float size);

/**
 * Set the number of cells a panel is spanning.
 *
 * @param panel the panel
 * @param axis the direction to set the span
 * @param span the number of cells the panel spans
 */
VKY_EXPORT void vkl_panel_span(VklPanel* panel, VklGridAxis axis, uint32_t span);

/**
 * Set the position of a panel within a grid.
 *
 * @param panel the panel
 * @param row the row index
 * @param col the column index
 */
VKY_EXPORT void vkl_panel_cell(VklPanel* panel, uint32_t row, uint32_t col);

/**
 * Set the coordinate system transposition (order and direction of the 3 xyz axes).
 *
 * @param panel the panel
 * @param transpose the transposition mode
 */
VKY_EXPORT void vkl_panel_transpose(VklPanel* panel, VklCDSTranspose transpose);

/**
 * Returns whether a point is contained in a panel.
 *
 * @param panel the panel
 * @param pos the position
 */
VKY_EXPORT bool vkl_panel_contains(VklPanel* panel, vec2 pos);

/**
 * Return the panel at a given position within the canvas.
 *
 * @param grid the grid
 * @param pos the position in screen coordinates (pixels)
 */
VKY_EXPORT VklPanel* vkl_panel_at(VklGrid* grid, vec2 pos);

/**
 * Destroy a panel and all visuals inside it.
 *
 * @param panel the panel
 */
VKY_EXPORT void vkl_panel_destroy(VklPanel* panel);

/**
 * Return the viewport of a panel.
 *
 * @param panel the panel
 * @returns the viewport
 */
VKY_EXPORT VklViewport vkl_panel_viewport(VklPanel* panel);



#endif
