// match + 守卫
//   ./build/src/c_hat_compiler examples/match.ch --run
func main() -> int {
    var total = 0;
    for (int i = 0; i < 4; i++) {
        match (i) {
            0 => { total += 1; }
            1 => { total += 10; }
            _ => { total += 100; }
        }
    }
    return total; // 211
}
