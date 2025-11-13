# Defer

`ach::defer` is an RAII scope guard that executes a callable on destruction. It ensures cleanup actions run when leaving scope, either normally or through exception. Designed for integrating with dynamic resources from C-style functions.

Cleanup executes automatically on scope exit, whether exiting normally or through an exception. The guard is movable, allowing ownership transfer to another scope. Execution can be cancelled if cleanup is no longer needed, or triggered early before scope exit

### Usage

```cpp
void process_file()
{
    int fd = open("file.txt", O_RDONLY);
    auto guard = ach::defer([fd] { close(fd); });

    // Use file...

    // close() called automatically on scope exit
}
```

### Advanced Usage

```cpp
auto guard = ach::defer([] { cleanup(); });

// Cancel if cleanup not needed
if (some_condition)
    guard.cancel();

// Execute early
if (need_cleanup_now)
    guard.execute();

// Move ownership
auto moved_guard = std::move(guard);
```

### Exception Safety

The constructor may throw if the callable's constructor throws. The destructor is `noexcept` if the callable is `noexcept`, but calls `std::terminate` if the callable throws during stack unwinding. The `execute()` function may throw if the callable throws. The `cancel()` function provides a `noexcept` guarantee. Move constructor and assignment are `noexcept` only if the callable's move is `noexcept`.

### Requirements

The callable must satisfy the `Deferrable` concept, meaning it must be invocable with no arguments and nothrow move constructible.

### Thread Safety

`defer` is not thread-safe. Each instance should be owned by a single thread
