# Utility

The utility library provides three key components for common C++ patterns: thread-safe counters, move-only wrappers, and strong typing.

## Counter

`ach::counter` is a thread-safe atomic counter that selects optimal memory ordering based on the target architecture. On x86/x64 (TSO architectures), it uses relaxed memory ordering for better performance. On ARM and PowerPC (weakly-ordered architectures), it uses acquire-release semantics. All operations are atomic and use zero-overhead optimal memory ordering for each architecture

### Usage

```cpp
ach::counter<std::size_t> count;

// Increment
count++;
count += 10;

// Decrement
count--;
count -= 5;

// Read value
std::size_t value = count.load();
```

### Exception Safety

All operations provide a `noexcept` guarantee

## Resource

`ach::resource` provides move-only semantics for value types, similar to `std::unique_ptr` but without heap allocation. Copy operations are deleted to enforce exclusive ownership. Value storage happens inline without heap allocation overhead, making it RAII-friendly for use in containers without double indirection

### Usage

```cpp
ach::resource<FileHandle> handle(open_file("data.txt"));

// Move into vector (no heap allocation)
std::vector<ach::resource<FileHandle>> handles;
handles.push_back(std::move(handle));

// Borrow reference
FileHandle& borrowed = ach::borrow(handles[0]);
```

### Exception Safety

The constructor may throw if `T`'s constructor throws. Move operations are `noexcept` only if `T`'s move constructor is `noexcept`

## Distinct

`ach::distinct` creates strong types from underlying types, preventing implicit conversions while preserving comparison semantics. Distinct types with the same underlying type cannot be implicitly converted to each other. The integral specialization supports use as non-type template parameters. The implementation has zero runtime cost

### Usage

```cpp
using UserId = ach::distinct<int, struct UserIdTag>;
using ProductId = ach::distinct<int, struct ProductIdTag>;

UserId user(42);
ProductId product(42);

// Type error - cannot mix distinct types
// user == product; // Compile error

// Explicit extraction
int user_value = ach::type_cast(user);
```

### Exception Safety

The constructor may throw if `T`'s constructor throws. The `value()` function is `noexcept` if `T` is nothrow copy constructible. The `type_cast()` function is `noexcept` if `value()` is `noexcept`.

### Implementation Details

The integral specialization allows `consteval` conversion for use as template parameters. Comparison operators are only available in constant evaluation contexts
