# Value

`ach::value` is a type-erased container that can store any type through a customization point (`value_traits`). Small objects use inline storage via small buffer optimization, while larger objects are heap-allocated.

The container provides type erasure, allowing you to store any type without templates at the usage site. Small objects (up to 32 bytes) are stored inline without heap allocation, while larger objects are heap-allocated. Storage representation is customizable through `value_traits`, and the allocator for heap allocations is configurable

### Usage

```cpp
ach::value<> v;

// Store different types
v.emplace<int>(42);
v.emplace<std::string>("hello");
v.emplace<std::vector<int>>({1, 2, 3});

// Retrieve values
int i = v.get<int>();
std::string s = v.get<std::string>();
```

### Custom Storage with value_traits

```cpp
template<>
struct ach::value_traits<MyType>
{
    struct value
    {
        int data;
        // Custom storage layout
    };
};

ach::value<> v;
v.emplace<MyType>(/* args */);
```

### Exception Safety

The `emplace()` function provides a strong exception guarantee, making no changes if an exception occurs. The `get()` function may throw if there's a type mismatch or the value is empty. Copy construction may throw if the stored type's copy constructor throws, while move construction is `noexcept` only if the stored type's move constructor is `noexcept`.

### Implementation Details

The implementation uses a 32-byte inline buffer with `alignof(std::max_align_t)` alignment for small objects. Objects larger than 32 bytes are heap-allocated. Type information is stored via compile-time vtables, providing zero virtual dispatch overhead
