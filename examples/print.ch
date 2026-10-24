// 通过 extern "C" 调用 libc puts
//   ./build/src/c_hat_compiler examples/print.ch
//   ./print
extern "C" {
    func puts(byte!^ s) -> int;
}

func main() -> int {
    puts("hello from C^");
    return 0;
}
