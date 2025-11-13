# Algorithm

The algorithm library provides utility functions that complement the standard library algorithms with commonly needed operations. All functions provide both iterator-based and range-based interfaces for convenience. The library includes hash combining for composite types, index-based search utilities, and safe type casting

## enumerate

Iterates over a range while providing both index and element.

```cpp
std::vector<std::string> items = {"a", "b", "c"};

ach::enumerate(items.begin(), items.end(), [](std::size_t i, const auto& item) {
    std::println("{}: {}", i, item);
});

// Range version
ach::ranges::enumerate(items, [](std::size_t i, const auto& item) {
    std::println("{}: {}", i, item);
});
```

## hash_combine

Combines hashes of multiple elements using FNV-1a variant.

```cpp
struct Point {
    int x, y;
};

std::size_t hash = ach::ranges::hash_combine(
    std::array{1, 2, 3},
    [](int n) { return n; }
);

// Custom hash for composite types
template<>
struct std::hash<Point> {
    std::size_t operator()(const Point& p) const {
        return ach::ranges::hash_combine(
            std::array{p.x, p.y},
            [](int n) { return n; }
        );
    }
};
```

## contains / contains_if

Checks if a range contains a value or satisfies a predicate.

```cpp
std::vector<int> numbers = {1, 2, 3, 4, 5};

bool has_three = ach::ranges::contains(numbers, 3); // true
bool has_even = ach::ranges::contains_if(numbers, [](int n) { return n % 2 == 0; }); // true
```

## find_index_if

Returns the index of the first element satisfying a predicate, or `size_t(-1)` if not found.

```cpp
std::vector<int> numbers = {1, 2, 3, 4, 5};

std::size_t index = ach::ranges::find_index_if(numbers, [](int n) { return n > 3; });
// index == 3 (element 4)

std::size_t not_found = ach::ranges::find_index_if(numbers, [](int n) { return n > 10; });
// not_found == size_t(-1)
```

## is_sorted_until_index

Returns the index where the range stops being sorted.

```cpp
std::vector<int> numbers = {1, 2, 3, 2, 5};

std::size_t unsorted_at = ach::ranges::is_sorted_until_index(numbers);
// unsorted_at == 3 (element 2 breaks sort order)
```

## safe_cast

Performs safe type conversions using `std::bit_cast` for trivially copyable types or `static_cast` otherwise.

```cpp
// Bit-cast for same-size trivially copyable types
float f = 3.14f;
std::uint32_t bits = ach::safe_cast<std::uint32_t>(f);

// Regular conversion for convertible types
int i = 42;
long l = ach::safe_cast<long>(i);
```

### Exception Safety

All functions are `noexcept` except where user-provided functions may throw. The `safe_cast` function is `constexpr` and `noexcept`
