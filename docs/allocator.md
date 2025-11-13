# Allocator

`ach::allocator` is a thread-safe arena-based allocator designed for high-performance memory allocation with minimal fragmentation. It uses bump allocation for new memory and a two-level bitmap for recycling freed blocks.

The allocator organizes memory into fixed-size 32MB arenas, providing predictable allocation behavior. New allocations use O(1) bump allocation, while the two-level bitmap (L1/L2 structure) enables fast free block lookup when recycling memory. Thread safety is configurable via `allocation_policy`, and the interface is compatible with STL containers

### Usage

```cpp
ach::allocator<int> alloc;

// Allocate memory
int* ptr = alloc.allocate(10);

// Use memory
for (int i = 0; i < 10; ++i) {
    ptr[i] = i * 2;
}

// Deallocate
alloc.deallocate(ptr, 10);
```

### Thread Safety

```cpp
// Thread-local allocation (default)
ach::allocator<int, ach::allocation_policy::local> local_alloc;

// Thread-safe allocation
ach::allocator<int, ach::allocation_policy::shared> shared_alloc;
```

### Exception Safety

Arena construction and the `allocate()` function may throw `std::bad_alloc` if memory allocation fails or no memory is available. The `deallocate()` function provides a `noexcept` guarantee.

### Implementation Details

Each arena is 32MB in size, allocated using `VirtualAlloc` on Windows or `mmap` on Unix-like systems. Bitmaps are stored at the end of each arena, with the two-level bitmap structure providing fast O(1) free block search
