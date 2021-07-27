fn main() {
    let mut count : f64 = 2.0;
    let mut i : f64 = 1.0;
    'outter: while i < 1000000.0 {
        i += 2.0;
        let mut j: f64 = 3.0;
        while j * j <= i {
            if i % j == 0.0 {
                continue 'outter;
            }
            j += 2.0;
        }
        count += 1.0;
    }
    println!("{}", count);
}
