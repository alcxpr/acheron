# Unique Map

`ach::unique_map` is an open-addressed hash table that maintains stable pointers to keys. Keys are never moved or destroyed after insertion (only reassigned on slot reuse), making it suitable for scenarios where key addresses must remain constant.

The implementation uses open addressing with linear probing for collision resolution. Key storage is managed by `ach::freelist`, ensuring addresses never change after insertion. The interface is compatible with standard `std::hash` and `std::equal_to` customization points

### Usage

```cpp
ach::unique_map<std::string, int> map;

// Insert elements
auto [key_ptr, value_ptr, inserted] = map.emplace("key", 42);

// Key pointer remains stable
const std::string* stable_key = key_ptr;

// Insert more elements - pointer still valid
map.emplace("another", 100);
assert(stable_key == key_ptr); // Still points to same key

// Lookup
if (auto it = map.find("key"); it != map.end()) {
    int value = *it->second;
}
```

### Exception Safety

The `emplace()` function provides a strong exception guarantee, making no changes if an exception occurs. The `find()` function provides a `noexcept` guarantee. The `erase()` function may throw if the value's destructor throws.

### Implementation Details

The implementation uses open addressing with linear probing. Slots maintain one of three states: empty, occupied, or deleted. Keys are stored in `freelist<Key>` to ensure stable addresses, while values are stored in `std::vector<Value>` (which may move on reallocation). Deleted slots use tombstones to maintain probe sequences
