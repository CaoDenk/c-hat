// 最简单的 C^ 程序：编译并 JIT 运行
//   ./build/src/c_hat_compiler examples/hello.ch --run
func add(int a, int b) -> int {
    return a + b;
}

func main() -> int {
    var x = 40;
    var y = 2;
    return add(x, y);
}
