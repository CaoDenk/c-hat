// try / catch / throw
//   ./build/src/c_hat_compiler examples/exception.ch --run
func boom() -> void {
    throw 9;
}

func main() -> int {
    var result = 0;
    try {
        boom();
        result = 1;
    } catch (int e) {
        result = e;
    }
    return result; // 9
}
