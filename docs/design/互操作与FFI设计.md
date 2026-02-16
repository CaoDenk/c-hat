# C^ 互操作与 FFI 设计 (Interoperability & FFI)

为了让 C^ 能够无缝融入现有的 C/C++ 生态系统，我们设计了轻量级且高效的 FFI (Foreign Function Interface) 机制。

## 1. 外部函数声明 (Extern Block)

我们采用 `extern "ABI"` 块来声明外部函数，这比单独标记每个函数更整洁。

```cpp
// 声明引用 C 语言 ABI 的函数
extern "C" {
    // 映射 C 的 int printf(const char* fmt, ...);
    func printf(byte^ fmt, ...) -> int;
    
    // 映射 C 的 void* malloc(size_t size);
    func malloc(size_t size) -> byte^;
    
    // 映射 C 的 void free(void* ptr);
    func free(byte^ ptr);
}
```

### 1.1 动态库导入 (DllImport)

对于 Windows DLL 或 Linux Shared Object，可以使用属性 `[DllImport]` 指定库名。

```cpp
extern "C" {
    [DllImport("kernel32.dll")]
    func GetTickCount() -> ulong;
    
    [DllImport("user32.dll")]
    func MessageBoxA(byte^ hWnd, byte^ lpText, byte^ lpCaption, uint uType) -> int;
}
```

## 2. 类型映射 (Type Mapping)

C^ 的基础类型与 C 语言有着严格的二进制兼容性。

| C^ Type  | C Type (x64)                | 备注                                                                                      |
| :------- | :-------------------------- | :---------------------------------------------------------------------------------------- |
| `byte`   | `uint8_t` / `unsigned char` |                                                                                           |
| `sbyte`  | `int8_t` / `char`           |                                                                                           |
| `short`  | `int16_t`                   |                                                                                           |
| `int`    | `int32_t`                   |                                                                                           |
| `long`   | `int64_t`                   | **注意**：C^ 的 `long` 永远是 64 位，而 C 的 `long` 在 Windows 上是 32 位。映射时需注意。 |
| `float`  | `float`                     |                                                                                           |
| `double` | `double`                    |                                                                                           |
| `byte^`  | `void*` / `char*`           | 裸指针直接映射                                                                            |
| `struct` | `struct`                    | 只要标记 `[Repr(C)]`，内存布局一致                                                        |

### 2.1 结构体布局

默认情况下，C^ 编译器可能会重排结构体字段以优化对齐。如果需要与 C 交互，必须强制使用 C 布局。

```cpp
[Repr(C)]
struct Rect {
    int x;
    int y;
    int width;
    int height;
}
```

## 3. 字符串互操作

C^ 的 `string` 是 UTF-8 编码，并且通常不是以 null 结尾的（虽然底层实现可能会保留，但不能依赖）。

与 C 交互时，通常需要转换：

```cpp
// C^ string -> C string (const char*)
// 需要确保以 \0 结尾
func call_c_api(string s) {
    // 方式 1: 使用 c_str() (如果有实现)
    // printf(s.c_str());
    
    // 方式 2: 手动传递指针 (不安全，需确保 s 包含 \0)
    // printf(s.ptr); 
}
```

## 4. 导出给 C 使用 (Export)

如果想让 C^ 代码被 C 语言调用，可以使用 `[Export]` 属性。

```cpp
[Export(name: "my_add")]
func add(int a, int b) -> int {
    return a + b;
}
```
这将生成符合 C ABI 的符号 `my_add`。
