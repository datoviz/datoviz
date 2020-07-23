extern crate libc;
use libc::size_t;
use rand::distributions::{Normal, Distribution};

#[link(name = "visky")]
extern {
    fn vky_demo_scatter(point_count: size_t, points: *mut f64);
}

fn main() {

    let normal = Normal::new(0.0, 0.25);
    // NOTE: this number is the number of points TIMES 2 (x and y)
    let mut x: [f64; 50000] = [0.0; 50000];
    for i in 0..x.len() {
        x[i] = normal.sample(&mut rand::thread_rng());
    }
    unsafe { vky_demo_scatter(25000, x.as_mut_ptr() as *mut f64) };
}
