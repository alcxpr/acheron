# Bitfield

`ach::bitfield` provides compile-time bitfield manipulation with position and width validation. All operations are `constexpr` and generate optimal assembly equivalent to manual bit shifting.

Bit position and width are checked at compile time, providing early error detection. The implementation has zero overhead, generating optimal assembly code. Method chaining is supported through a fluent interface for setting multiple fields. Endianness conversion between byte orders is available

### Usage

```cpp
// ARM instruction encoding
constexpr auto instr = ach::bitfield<std::uint32_t>()
    .set<31, 1>(1)        // 64-bit operation
    .set<29, 2>(0b00)     // ADD opcode
    .set<21, 8>(42)       // Immediate value
    .set<5, 5>(0)         // Source register
    .set<0, 5>(1)         // Destination register
    .raw();

// Extract fields
ach::bitfield<std::uint32_t> bf(0x12345678);
std::uint32_t value = bf.get<16, 8>(); // Extract bits 16-23
```

### Operations

```cpp
ach::bitfield<std::uint32_t> bf;

// Set bits
bf.set<0, 8>(0xFF);     // Set bits 0-7 to 0xFF
bf.set<8, 4>(0xA);      // Set bits 8-11 to 0xA

// Get bits
std::uint32_t val = bf.get<0, 8>();

// Test, clear, flip
bool is_set = bf.test<0, 8>();
bf.clear<0, 8>();
bf.flip<0, 8>();

// Endianness conversion
auto be = bf.to_endian<std::endian::big>();
auto le = bf.to_endian<std::endian::little>();
```

### Exception Safety

All operations are `noexcept` and `constexpr`.

### Implementation Details

Template parameters for position and width enable compile-time checking, with `static_assert` failures providing clear error messages. Modern compilers generate a single instruction for set/get operations. Endianness conversion uses `std::byteswap`, which is a no-op for single-byte types
