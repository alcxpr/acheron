# Freelist

`ach::freelist` is a high-performance node recycling allocator that maintains a pool of reusable nodes. It allocates contiguous memory blocks with geometric growth and chains freed nodes for fast reuse.

Block sizes grow geometrically by 2x (starting at 64, then 128, 256, and so on). Released nodes are reused instead of being deallocated, providing fast recycling. Nodes within each block are allocated contiguously for better cache locality. Pointer types receive special handling through intrusive nodes, achieving zero overhead

### Usage

```cpp
ach::freelist<MyType> freelist;

// Allocate a node
MyType* ptr = freelist.allocate();

// Construct object
std::construct_at(ptr, /* args */);

// Use object...

// Destroy object
std::destroy_at(ptr);

// Return node to freelist (fast reuse)
freelist.deallocate(ptr);
```

### Exception Safety

The `allocate()` function may throw `std::bad_alloc` if memory allocation fails. The `deallocate()` function provides a `noexcept` guarantee.

### Implementation Details

The first block contains 64 nodes, and each subsequent block doubles in size (128, 256, 512, and so on). Freed nodes are chained in a singly-linked list for fast reuse. The next allocation reuses from the free list before allocating a new block. Pointer types use intrusive nodes for zero overhead
