module std.mem;

public func zero(int[] data) {
    long i = 0;
    while (i < data.len) {
        data[i] = 0;
        i = i + 1;
    }
}
