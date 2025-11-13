# Diagnostic

The diagnostic library provides ergonomic assertions, exceptions, and logging with automatic source location capture and colored output. In debug builds, full stack traces are captured for fatal errors and assertions.

Source location (file, line, column, and function name) is captured automatically. Full stack traces are available in debug builds when `<stacktrace>` is available. Output is ANSI color-coded by default but can be disabled. Multiple severity levels are supported: fatal errors, assertions, warnings, and info messages

## Fatal Error

```cpp
void initialize_system()
{
    if (!load_critical_config())
        throw ach::fatal_error("failed to load critical configuration");
}
```

Prints diagnostic information to stderr before throwing:
```
[FATAL] failed to load critical configuration
/path/to/file.cpp:42:9
in void initialize_system()

stack trace:
  ...
```

## Assertions

```cpp
void process(int* ptr, std::size_t size)
{
    ach::assert(ptr != nullptr, "pointer is null");
    ach::assert(size > 0, std::format("invalid size: {}", size));

    // Use ptr and size...
}
```

Calls `std::terminate()` if condition is false. Always checks regardless of build type.

## Debug Assertions

```cpp
void hot_path(int* ptr)
{
    // Checked in debug, compiled out in release
    ach::debug_assert(ptr != nullptr, "pointer is null");
}
```

## Panic

```cpp
const char* color_name(Color c)
{
    switch (c) {
        case Color::Red:   return "red";
        case Color::Green: return "green";
        case Color::Blue:  return "blue";
    }
    ach::panic(std::format("invalid color: {}", static_cast<int>(c)));
}
```

Immediately terminates with a message. Equivalent to `assert(false, message)`.

## Warnings and Info

```cpp
void allocate(std::size_t size)
{
    if (size > 1024 * 1024)
        ach::warn(std::format("large allocation: {} bytes", size));

    ach::info("allocating memory");
    // Allocate...
    ach::debug("allocation complete"); // Debug builds only
}
```

### Colored Output

By default, diagnostics use ANSI color codes. To disable:
```cpp
#define ACHERON_COLORED_DIAGNOSTICS 0
#include <acheron/diagnostic.hpp>
```

### Exception Safety

The `fatal_error` constructor may throw but prints diagnostics first. The `assert` and `debug_assert` functions call `std::terminate()` on failure and never throw. The `panic` function is marked `[[noreturn]]` and calls `std::terminate()`. The `warn`, `info`, and `debug` functions provide a `noexcept` guarantee

### Macro Aliases

```cpp
ACH_ASSERT(condition, message)
ACH_DEBUG_ASSERT(condition, message)
ACH_PANIC(message)
ACH_WARN(message)
ACH_INFO(message)
ACH_DEBUG(message)
ACH_UNREACHABLE()
ACH_TODO(message)
```
