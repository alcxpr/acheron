# Acheron

Acheron is a small, general-purpose C++ library that implements many useful components. The goal
of this project is to be as powerful as possible while maintaining the small size to usefulness
ratio. Acheron is composed of twelve different sub-libraries, some of which may be useful to you. See
[here](#components) for the list of the components and their statuses.

Originally being a library for personal uses, Acheron has grown into a focused collection of C++
utilities that come from user feedback. It is designed around the idea that a small, well-engineered
set of abstractions can greatly improve ergonomics and safety while maintaining zero overhead. The 
library tries its best to provide the missing pieces that the standard library either does not yet offer 
or still is an active draft. 

Despite its scope, Acheron remains deliberately minimal. All features are implemented as lightweight, 
self-contained headers with no runtime dependencies.

> [!NOTE]
> Acheron is still under active development. There are currently no automated tests or benchmarks,
> and breaking chances are expected as the library evolves. Stability is yet to be guaranteed until tests
> and benchmarks are in place.

## Components

| Name          | Description                                                                   |
|---------------|-------------------------------------------------------------------------------|
| algorithm     | Hash combining and search utilities                                           |
| allocator     | Thread-safe, and efficient memory allocator                                   |
| argument      | An encoding-friendly and modern interface for accessing command-line argument |
| bitfield      | A target-independent bitfield                                                 |
| codecvt       | UTF-8/16/32 conversion utilities                                              |
| cstring_view  | Null-terminated string views for C interop. Based on P3655                    |
| defer         | RAII scope guards for cleaning up dynamic resources from C side               |
| diagnostic    | Ergonomic assertions, exceptions, with stacktraces for debugging              |
| static_string | Compile-time string container                                                 |
| variant       | Type-safe union with pattern matching and type-safe index access              |

## License

MIT License. See [LICENSE.txt](LICENSE.txt) for details.
