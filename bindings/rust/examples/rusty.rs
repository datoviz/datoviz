//! An example of the rust API usage.

fn main() {
    let app = datoviz::App::new();
    let gpu = datoviz::Gpu::new(&app, 0);
    let canvas = datoviz::Canvas::new(&gpu, 1280, 1024);
    let scene = datoviz::Scene::new(&canvas, 1, 1);
    let panel = datoviz::Panel::new(&scene, 0, 0);

    app.run(0);
    app.destroy();
}
