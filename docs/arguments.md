# Arguments

`ach::arguments` provides an encoding-friendly and modern interface for accessing command-line arguments throughout a program. The author read the [P3474R0](https://isocpp.org/files/papers/P3474R0.html) proposal and was inspired to write this implementation.

The implementation is platform-aware, using UTF-16 encoding on Windows and UTF-8 on Unix-like systems. You can construct `arguments` at any point during program execution, not just in `main()`. The interface is modern with no raw pointers required, providing standard container semantics. Arguments can be converted to UTF-8, UTF-16, or UTF-32 on demand

### Usage

```cpp
ach::arguments args;

// Check for help flag
if (args.size() > 1 && args[1].string() == "--help") {
    print_help();
}

// Iterate over arguments
for (const auto& arg : args) {
    std::println("{}", arg.string());
}

// Access specific encodings
std::string utf8 = args[1].string();
std::wstring wide = args[1].wstring();
std::u16string utf16 = args[1].u16string();
std::u32string utf32 = args[1].u32string();
```

### Platform Behavior

On Windows, the implementation uses `CommandLineToArgvW` to retrieve UTF-16 arguments. On Linux, it reads `/proc/self/cmdline` for the argument list. On macOS, it uses `_NSGetArgv` and `_NSGetArgc`.

### Exception Safety

The constructor may throw if memory allocation fails. The `at()` function may throw `std::out_of_range` if the index is invalid. The `operator[]` provides `noexcept` guarantee but has undefined behavior if the index is out of bounds. Encoding conversions may throw `std::bad_alloc` on memory allocation failure.

### Implementation Details

Arguments are lazily initialized on the first `ach::arguments` construction using thread-safe initialization with `std::once_flag`. The native encoding is stored and conversions are performed on demand. Whether modifications to `argv` in `main()` are reflected is implementation-defined
