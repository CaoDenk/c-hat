// 泛型类单态化：Box<int> / Box<double>
//   ./build/src/c_hat_compiler examples/generic_class.ch --run
class Box<T> {
    T value;

    Box(T v) { value = v; }

    func get_value() -> T { return value; }
}

func main() -> int {
    Box<int>^ a = new Box<int>(40);
    Box<double>^ b = new Box<double>(2.5);
    var d = b->get_value();
    if (d > 2.0) {
        return a->get_value() + 2; // 42
    }
    return 0;
}
