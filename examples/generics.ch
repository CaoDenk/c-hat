// 泛型函数单态化 + concept 检查
//   ./build/src/c_hat_compiler examples/generics.ch --run
concept Addable<T> = requires (T a, T b) {
    { a + b } -> T;
};

func max_of<T>(T a, T b) -> T where Comparable<T> {
    if (a > b) { return a; }
    return b;
}

func first<T>(T[] xs) -> T {
    return xs[0];
}

func main() -> int {
    int[3] a = [7, 8, 9];
    var m = max_of<int>(40, 42);
    var f = first(a);
    return m + f; // 49
}
