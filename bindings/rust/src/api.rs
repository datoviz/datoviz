//! These are raw prototypes of the C interface of datoviz.
//! 

use libc::c_int;

// Types:
// Treat types as abstract datatypes.
pub enum _DvzApp {}
pub type DvzApp = *mut _DvzApp;
pub enum _DvzGpu {}
pub type DvzGpu = *mut _DvzGpu;
pub enum _DvzCanvas {}
pub type DvzCanvas = *mut _DvzCanvas;
pub enum _DvzScene {}
pub type DvzScene = *mut _DvzScene;
pub enum _DvzPanel {}
pub type DvzPanel = *mut _DvzPanel;

type DvzBackend = c_int;
type DvzControllerType = c_int;

#[link(name = "datoviz")]
extern {

    // Demo function:
    pub fn dvz_demo_scatter(point_count: i32, points: *mut f64);

    // App api:
    pub fn dvz_app(backend: DvzBackend) -> DvzApp;

    pub fn dvz_gpu(app: DvzApp, idx: u32) -> DvzGpu;

    pub fn dvz_canvas(gpu: DvzGpu, width: u32, height: u32, flags: c_int) -> DvzCanvas;

    pub fn dvz_scene(canvas: DvzCanvas, n_rows: u32, n_cols: u32) -> DvzScene;

    pub fn dvz_scene_panel(scene: DvzScene, row: u32, col: u32, typ: DvzControllerType, flags: c_int) -> DvzPanel;

    pub fn dvz_app_run(app: DvzApp, frame_count: u64);

    pub fn dvz_app_destroy(app: DvzApp) -> c_int;
}
