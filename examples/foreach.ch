// foreach：数组 / 切片 / 范围
//   ./build/src/c_hat_compiler examples/foreach.ch --run
func main() -> int {
    int[3] a = [1, 2, 3];
    var total = 0;

    foreach (var x : a) { total += x; }          // 6
    foreach (var i : 0..3) { total += i; }       // + 0+1+2+3 = 12
    foreach (var idx, var x : a) { total += idx; } // + 0+1+2 = 15

    return total;
}
