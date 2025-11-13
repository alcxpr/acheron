<div align="center">
    <!-- TODO: add logo -->
    <h1> Acheron </h1>
    <p>
        <img src="https://img.shields.io/github/v/release/alcxpr/acheron" alt="Release">
        <img src="https://img.shields.io/badge/C%2B%2B-23-00599C.svg" alt="C++ Version">
        <img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License">
    </p>
</div>

Acheron is a small, general-purpose C++ library that implements many useful components. The goal
of this project is to be as powerful as possible while maintaining the small size to usefulness
ratio. Acheron is composed of thirteen different sub-libraries, some of which may be useful to you. See
[here](#components) for the list of the components and their statuses.

Originally being a library for personal uses, Acheron has grown into a focused collection of C++
utilities that come from user feedback. It is designed around the idea that a small, well-engineered
set of abstractions can greatly improve ergonomics and safety while maintaining zero overhead. The 
library tries its best to provide the missing pieces that the standard library either does not yet offer 
or still is an active draft. 

Despite its scope, Acheron remains deliberately minimal. All features are implemented as lightweight, 
self-contained headers with no runtime dependencies.

## Components

| Name                                              | Description                                                                    |
|---------------------------------------------------|--------------------------------------------------------------------------------|
| [algorithm](include/acheron/algorithm.hpp)        | Hash combining and search utilities                                            |
| [allocator](include/acheron/allocator.hpp)        | Thread-safe, and efficient memory allocator                                    |
| [arguments](include/acheron/arguments.hpp)        | An encoding-friendly and modern interface for accessing command-line argument  |
| [bitfield](include/acheron/bitfield.hpp)          | A target-independent bitfield                                                  |
| [codecvt](include/acheron/codecvt.hpp)            | UTF-8/16/32 conversion utilities                                               |
| [cstring_view](include/acheron/cstring_view.hpp)  | Null-terminated string views for C interop. Based on P3655                     |
| [defer](include/acheron/defer.hpp)                | RAII scope guards for cleaning up dynamic resources from C side                |
| [diagnostic](include/acheron/diagnostic.hpp)      | Ergonomic assertions, exceptions, with stacktraces for debugging               |
| [freelist](include/acheron/freelist.hpp)          | Node recycling container with geometric growth                                 |
| [utility](include/acheron/utility.hpp)            | Utilities for C++ like counter                                                 |
| [unique_map](include/acheron/unique_map.hpp)      | Hash map with stable key pointers and open addressing                          |
| [value](include/acheron/value.hpp)                | Type-erased value container with 32-byte SSO                                   |
| [variant](include/acheron/variant.hpp)            | Type-safe union with pattern matching and type-safe index access               |

## Installation

### Requirements

You need:
- C++23 compiler support
- CMake 3.15+ for building tests

### Integration

You can use [CMake](https://cmake.org/) to add the project as a dependency.

```cmake
include(FetchContent)
FetchContent_Declare(acheron
    GIT_REPOSITORY https://github.com/alcxpr/acheron.git
    GIT_TAG main
)
FetchContent_MakeAvailable(acheron)
# ...
target_link_libraries(your_target PRIVATE Acheron::Acheron) # or simply acheron
```

To build tests, simply do:

```bash
$ cd build/
$ cmake .. -DACHERON_BUILD_TESTS=ON # optionally, -G ninja
$ cmake --build .
$ ctest --test-dir .
```

## License

MIT License. See [LICENSE.txt](LICENSE.txt) for details.
