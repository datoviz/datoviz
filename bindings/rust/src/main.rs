extern crate libc;
use libc::size_t;
use rand::distributions::{Normal, Distribution};

#[link(name = "visky")]
extern {
    fn vky_demo_scatter(point_count: size_t, points: *mut f64);
}

fn main() {

    let normal = Normal::new(0.0, 0.25);
    let mut x: [f64; 2000] = [0.0; 2000];
    for i in 0..x.len() {
        x[i] = normal.sample(&mut rand::thread_rng());
    }
    unsafe { vky_demo_scatter(1000, x.as_mut_ptr() as *mut f64) };
}
