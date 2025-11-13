# Variant

`std::variant` is a powerful type introduced in C++17, however, it can be cumbersome to use as it requires verbose `std::visit` calls and can lead to boilerplate code.
`ach::variant` solves this with natural switch case support and pattern matching while being standard conforming (except the extensions).

### Usage

```cpp
using Value = ach::variant<int, double, std::string>;

auto v = Value(42);
switch (v.index()) {
    case Value::of<int>:
        std::println("int: {}", ach::get<int>(v));
        break;
    case Value::of<double>:
        std::println("double: {}", ach::get<double>(v));
        break;
    case Value::of<std::string>:
        std::println("string: {}", ach::get<std::string>(v));
        break;
}
```

All APIs of `ach::variant` are available to view at cppreference. This version supports up to C++26's API.
