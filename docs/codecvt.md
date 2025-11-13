# Codecvt

`ach::codecvt` provides utilities for converting between UTF-8, UTF-16, and UTF-32 encodings. Invalid sequences are replaced with the Unicode replacement character (U+FFFD).

### Usage

```cpp
// UTF-8 to UTF-16
std::string utf8 = "Hello, 世界";
std::u16string utf16 = ach::utf8_to_utf16(utf8);

// UTF-16 to UTF-32
std::u32string utf32 = ach::utf16_to_utf32(utf16);

// UTF-32 to UTF-8
std::string back = ach::utf32_to_utf8(utf32);
```

### Available Conversions

The library provides bidirectional conversions between all UTF encodings: `utf8_to_utf16` / `utf16_to_utf8`, `utf8_to_utf32` / `utf32_to_utf8`, and `utf16_to_utf32` / `utf32_to_utf16`. All functions are also available as iterator-based versions for custom buffer management
