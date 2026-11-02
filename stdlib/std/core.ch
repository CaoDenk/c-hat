module std.core;

public func min(int a, int b) -> int {
    if (a < b) { return a; }
    return b;
}

public func max(int a, int b) -> int {
    if (a > b) { return a; }
    return b;
}

public func abs(int value) -> int {
    if (value < 0) { return -value; }
    return value;
}

public func clamp(int value, int low, int high) -> int {
    if (value < low) { return low; }
    if (value > high) { return high; }
    return value;
}
