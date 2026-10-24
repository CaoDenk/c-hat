// while / for / 复合赋值 / 自增
//   ./build/src/c_hat_compiler examples/loops.ch --run
func main() -> int {
    var sum = 0;
    var i = 0;
    while (i < 3) {
        sum += i;
        i++;
    }

    for (int j = 0; j < 3; j++) {
        sum += j;
    }

    return sum; // 3 + 3 = 6
}
