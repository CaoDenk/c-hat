
module std.io.Console;

extern "C" {
    func puts(byte^ s) -> int;
}

public func println(byte!^ message) {
    puts(message);
}
