mod foo;

// Runs foo function, then prints result
fn main() {
    let v = foo::foo();
    println!("Program complete.  Return {}",v);
}
