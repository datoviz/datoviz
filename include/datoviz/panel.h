/*************************************************************************************************/
/*  Panels organized within a 2D grid layout (subplots)                                          */
/*************************************************************************************************/

#ifndef DVZ_PANEL_HEADER
#define DVZ_PANEL_HEADER

#include "transforms.h"
#include "visuals.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_GRID_MAX_COLS         64
#define DVZ_GRID_MAX_ROWS         64
#define DVZ_MAX_PANELS            1024
#define DVZ_MAX_LINKS             16
#define DVZ_MAX_VISUALS_PER_PANEL 64

// Group index of the set of panel DvzCommands objects.
#define DVZ_COMMANDS_GROUP_PANELS 1



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Panel mode.
typedef enum
{
    DVZ_PANEL_GRID,
    DVZ_PANEL_INSET,
    DVZ_PANEL_FLOATING,
} DvzPanelMode;



// Grid axis.
typedef enum
{
    DVZ_GRID_HORIZONTAL,
    DVZ_GRID_VERTICAL,
} DvzGridAxis;



// Size unit.
typedef enum
{
    DVZ_PANEL_UNIT_NORMALIZED,
    DVZ_PANEL_UNIT_FRAMEBUFFER,
    DVZ_PANEL_UNIT_SCREEN,
} DvzPanelSizeUnit;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGrid DvzGrid;
typedef struct DvzPanel DvzPanel;
typedef struct DvzPanelLink DvzPanelLink;
typedef struct DvzController DvzController;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzPanel
{
    DvzObject obj;
    DvzScene* scene;

    DvzGrid* grid;
    DvzPanelMode mode;
    DvzPanelSizeUnit size_unit; // the unit x, y, width, height are in
    DvzDataCoords data_coords;  // data CPU transformation

    // User-specified:
    uint32_t row, col;
    uint32_t hspan, vspan;

    // Computed automatically from above and grid widths and heights:
    // Unless using inset
    float x, y;
    float width, height;

    // Visuals
    uint32_t visual_count;
    DvzVisual* visuals[DVZ_MAX_VISUALS_PER_PANEL];

    // Viewport.
    DvzViewport viewport;

    // GPU objects
    DvzBufferRegions br_mvp; // for the uniform buffer containing the MVP

    DvzController* controller;
    DvzCommands* cmds;
    int prority_max;
};



struct DvzPanelLink
{
    DvzGrid* grid;
    DvzPanel* source;
    DvzPanel* target;
};



struct DvzGrid
{
    DvzCanvas* canvas;
    uint32_t n_rows, n_cols;

    // In normalized coordinates (in [0, 1]):
    float xs[DVZ_GRID_MAX_COLS];
    float ys[DVZ_GRID_MAX_ROWS];
    double widths[DVZ_GRID_MAX_COLS];
    double heights[DVZ_GRID_MAX_ROWS];

    uint32_t link_count;
    DvzPanelLink links[DVZ_MAX_LINKS];

    DvzContainer panels;
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
DVZ_EXPORT DvzGrid dvz_grid(DvzCanvas* canvas, uint32_t row_count, uint32_t col_count);

/**
 * Destroy a grid.
 *
 * @param grid the grid
 */
DVZ_EXPORT void dvz_grid_destroy(DvzGrid* grid);

/**
 * Create a panel at a given location in a grid.
 *
 * @param grid the grid
 * @param row the row index in the grid
 * @param col the column index in the grid
 * @returns the panel
 */
DVZ_EXPORT DvzPanel* dvz_panel(DvzGrid* grid, uint32_t row, uint32_t col);

/**
 * Update a panel viewport.
 *
 * @param panel the panel
 */
DVZ_EXPORT void dvz_panel_update(DvzPanel* panel);

/**
 * Set panel margins.
 *
 * Margins are represented as a vec4 vector: top, right, bottom, left.
 *
 * @param panel the panel
 * @param margins the margins, in pixels
 */
DVZ_EXPORT void dvz_panel_margins(DvzPanel* panel, vec4 margins);

/**
 * Set the unit in which the panel size is specified.
 *
 * @param panel the panel
 * @param unit the unit
 */
DVZ_EXPORT void dvz_panel_unit(DvzPanel* panel, DvzPanelSizeUnit unit);

/**
 * Set the panel mode (grid or detached).
 *
 * @param panel the panel
 * @param mode the mode
 */
DVZ_EXPORT void dvz_panel_mode(DvzPanel* panel, DvzPanelMode mode);

/**
 * Add a visual to a panel.
 *
 * @param panel the panel
 * @param visual the visual
 */
DVZ_EXPORT void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual);

/**
 * Set a panel position (in detached mode).
 *
 * The unit in which the coordinates are specified is controller by `dvz_panel_unit()`.
 *
 * @param panel the panel
 * @param x the position
 * @param y the position
 */
DVZ_EXPORT void dvz_panel_pos(DvzPanel* panel, float x, float y);

/**
 * Set a panel size (in detached mode).
 *
 * The unit in which the size is specified is controller by `dvz_panel_unit()`.
 *
 * @param panel the panel
 * @param axis the axis on which to specify the size
 * @param size the size
 */
DVZ_EXPORT void dvz_panel_size(DvzPanel* panel, DvzGridAxis axis, float size);

/**
 * Set the number of cells a panel is spanning.
 *
 * @param panel the panel
 * @param axis the direction to set the span
 * @param span the number of cells the panel spans
 */
DVZ_EXPORT void dvz_panel_span(DvzPanel* panel, DvzGridAxis axis, uint32_t span);

/**
 * Set the position of a panel within a grid.
 *
 * @param panel the panel
 * @param row the row index
 * @param col the column index
 */
DVZ_EXPORT void dvz_panel_cell(DvzPanel* panel, uint32_t row, uint32_t col);

/**
 * Set the transform of a panel.
 *
 * !!! note
 *     Not implemented yet.
 *
 * @param panel the panel
 * @param transform the type of transform
 */
DVZ_EXPORT void dvz_panel_transform(DvzPanel* panel, DvzTransformType transform);

/**
 * Set the coordinate system transposition (order and direction of the 3 xyz axes).
 *
 * @param panel the panel
 * @param transpose the transposition mode
 */
DVZ_EXPORT void dvz_panel_transpose(DvzPanel* panel, DvzCDSTranspose transpose);

/**
 * Returns whether a point is contained in a panel.
 *
 * @param panel the panel
 * @param screen_pos the position in screen pixel coordinates
 * @returns a boolean
 */
DVZ_EXPORT bool dvz_panel_contains(DvzPanel* panel, vec2 screen_pos);

/**
 * Return the panel at a given position within the canvas.
 *
 * @param grid the grid
 * @param screen_pos the position in screen pixel coordinates
 * @returns the panel
 */
DVZ_EXPORT DvzPanel* dvz_panel_at(DvzGrid* grid, vec2 screen_pos);

/**
 * Add a link between two panels.
 *
 * @param grid the grid
 * @param source the source panel
 * @param source the source panel
 */
DVZ_EXPORT void dvz_panel_link(DvzGrid* grid, DvzPanel* source, DvzPanel* target);

/**
 * Destroy a panel and all visuals inside it.
 *
 * @param panel the panel
 */
DVZ_EXPORT void dvz_panel_destroy(DvzPanel* panel);

/**
 * Return the viewport of a panel.
 *
 * @param panel the panel
 * @returns the viewport
 */
DVZ_EXPORT DvzViewport dvz_panel_viewport(DvzPanel* panel);



#endif
