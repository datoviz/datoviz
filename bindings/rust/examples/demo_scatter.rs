extern crate libc;
use rand::distributions::{Normal, Distribution};

fn main() {

    let normal = Normal::new(0.0, 0.25);
    // NOTE: this number is the number of points TIMES 3 (x, y, z)
    let mut x: [f64; 30000] = [0.0; 30000];
    for i in 0..x.len() {
        x[i] = normal.sample(&mut rand::thread_rng());
    }
    unsafe { datoviz::dvz_demo_scatter(10000, x.as_mut_ptr() as *mut f64) };
}
