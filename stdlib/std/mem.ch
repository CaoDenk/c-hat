module std.mem;

public func zero(int[] data) {
    long i = 0;
    while (i < data.len) {
        data[i] = 0;
        i = i + 1;
    }
}
public func copy(int[] dst, int[] src) -> long {
    long count = dst.len;
    if (src.len < count) { count = src.len; }
    long i = 0;
    while (i < count) {
        dst[i] = src[i];
        i = i + 1;
    }
    return count;
}
