module std.string;

public func length(byte![] text) -> long {
    return text.len;
}

public func equal(byte![] a, byte![] b) -> bool {
    if (a.len != b.len) { return false; }
    long i = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}
