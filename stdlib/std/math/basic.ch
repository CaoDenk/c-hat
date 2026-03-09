module std.math.basic;

public func add(int a, int b) -> int {
    return a + b;
}

public func multiply(int a, int b) -> int {
    return a * b;
}

public func subtract(int a, int b) -> int {
    return a - b;
}

public func max(int a, int b) -> int {
    if (a > b) {
        return a;
    }
    return b;
}
