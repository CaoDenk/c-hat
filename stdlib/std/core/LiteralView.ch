
module std.core.LiteralView;

import std.core.Types;

// LiteralView 扩展
extension LiteralView {
    // 隐式转换为只读切片
    public implicit operator byte![]() {
        return { ptr = self.ptr, len = self.len };
    }
    
    // 隐式转换为只读指针（C 互操作）
    public implicit operator byte!^() {
        return self.ptr;
    }
    
    // 便捷方法
    public func isEmpty() -> bool {
        return self.len == 0;
    }
}
