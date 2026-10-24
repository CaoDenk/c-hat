// 类、构造函数、方法、self 字段访问
//   ./build/src/c_hat_compiler examples/class.ch --run
class Counter {
    int value;

    Counter(int start) {
        value = start;
    }

    func add(int n) -> int {
        value = value + n;
        return value;
    }

    func get_value(self!) -> int {
        return self.value;
    }
}

func main() -> int {
    Counter^ c = new Counter(40);
    c->add(2);
    return c->get_value();
}
