// 泛型结构体单态化：Point<int> / Point<double>
//   ./build/src/c_hat_compiler examples/generic_struct.ch --run
struct Point<T> {
    T x;
    T y;

    Point(T a, T b) { x = a; y = b; }

    func sum() -> T { return x + y; }
}

class Box<T> where Comparable<T> {
    Point<T> p;

    Box(T a, T b) {
        p = Point<T>(a, b);
    }

    func total() -> T { return p.sum(); }
}

func main() -> int {
    Box<int>^ b = new Box<int>(40, 2);
    return b->total(); // 42
}
