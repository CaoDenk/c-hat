module std.io.Console;

extern "C" {
    func puts(byte!^ s) -> int;
    func printf(byte!^ format, ...) -> int;
}

public func println(byte!^ message) {
    puts(message);
}

public func print_int(int value) {
    printf("%d", value);
}

public func println_int(int value) {
    printf("%d\n", value);
}

public func print_bool(bool value) {
    if (value) {
        printf("true");
    } else {
        printf("false");
    }
}

public func println_bool(bool value) {
    print_bool(value);
    printf("\n");
}
public func print_long(long value) {
    printf("%lld", value);
}

public func println_long(long value) {
    printf("%lld\n", value);
}

public func print_double(double value) {
    printf("%g", value);
}

public func println_double(double value) {
    printf("%g\n", value);
}
