//! A more rust-onic api around the datoviz C API.

mod api;

pub use api::dvz_demo_scatter;

pub struct App {
    app: api::DvzApp,
}

impl App {
    pub fn new() -> Self {
        let DVZ_BACKEND_GLFW = 1;
        let app = unsafe { api::dvz_app(DVZ_BACKEND_GLFW) };
        Self {
            app
        }
    }

    pub fn run(&self, frame_count: u64) {
        unsafe {
            api::dvz_app_run(self.app, frame_count);
        }
    }

    pub fn destroy(&self) {
        unsafe {
            api::dvz_app_destroy(self.app);
        }
    }
}

pub struct Gpu {
    gpu: api::DvzGpu,
}

impl Gpu {
    pub fn new(app: &App, index: u32) -> Self {
        let gpu = unsafe { api::dvz_gpu(app.app, index) };
        Self {
            gpu
        }
    }
}

pub struct Canvas {
    canvas: api::DvzCanvas,
}

impl Canvas {
    pub fn new(gpu: &Gpu, width: u32, height: u32) -> Self {
        let flags = 0; // TODO
        let canvas = unsafe { api::dvz_canvas(gpu.gpu, width, height, flags)};
        Self {
            canvas
        }
    }
}

pub struct Scene {
    scene: api::DvzScene,
}

impl Scene {
    pub fn new(canvas: &Canvas, rows: u32, cols: u32) -> Self {
        let scene = unsafe { api::dvz_scene(canvas.canvas, rows, cols) };
        Self {
            scene
        }
    }
}

pub struct Panel {
    panel: api::DvzPanel,
}

impl Panel {
    pub fn new(scene: &Scene, row: u32, col: u32) -> Self {
        let DVZ_CONTROLLER_AXES_2D = 2;
        let flags = 0; // TODO
        let panel = unsafe { api::dvz_scene_panel(scene.scene, row, col, DVZ_CONTROLLER_AXES_2D, flags) };
        Self {
            panel
        }
    }
}