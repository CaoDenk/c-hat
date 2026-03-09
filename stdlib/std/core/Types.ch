
module std.core.Types;

// 基础类型别名
using size_t = long;
using ptrdiff_t = long;

// StringView 类型别名（byte![]）
using StringView = byte![];

// 比较操作
public func compare(long a, long b) -> int {
    if (a < b) {
        return -1;
    } else if (a > b) {
        return 1;
    } else {
        return 0;
    }
}

public func equals(long a, long b) -> bool {
    return a == b;
}
