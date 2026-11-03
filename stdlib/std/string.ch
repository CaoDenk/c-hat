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
public func find(byte![] text, byte![] needle) -> long {
    if (needle.len == 0) { return 0; }
    if (text.len < needle.len) { return -1; }
    long i = 0;
    while (i + needle.len <= text.len) {
        long j = 0;
        bool matched = true;
        while (j < needle.len) {
            if (text[i + j] != needle[j]) {
                matched = false;
                break;
            }
            j = j + 1;
        }
        if (matched) { return i; }
        i = i + 1;
    }
    return -1;
}

public func starts_with(byte![] text, byte![] prefix) -> bool {
    if (prefix.len > text.len) { return false; }
    long i = 0;
    while (i < prefix.len) {
        if (text[i] != prefix[i]) { return false; }
        i = i + 1;
    }
    return true;
}

public func ends_with(byte![] text, byte![] suffix) -> bool {
    if (suffix.len > text.len) { return false; }
    long offset = text.len - suffix.len;
    long i = 0;
    while (i < suffix.len) {
        if (text[offset + i] != suffix[i]) { return false; }
        i = i + 1;
    }
    return true;
}
