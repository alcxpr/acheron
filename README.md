# Acheron

Acheron is a small, general-purpose C++ library that implements many useful components. The goal
of this project is to be as powerful as possible while maintaining the small size to usefulness
ratio.

Acheron is composed of twelve different sub-libraries, some of which may be useful to you. See
[here](#components) for the list of the components and their statuses.

> [!NOTE]
> Acheron is still heavily under development. Breaking changes are expected and there will be
> no support as it is a rolling release library.

## Components

| Name          | Status   | Description                                                                   |
|---------------|----------|-------------------------------------------------------------------------------|
| algorithm     | Complete | Hash combining and search utilities                                           |
| allocator     | Complete | Thread-safe, and efficient memory allocator                                   |
| argument      | Complete | An encoding-friendly and modern interface for accessing command-line argument |
| bitfield      | Complete | A target-independent bitfield                                                 |
| codecvt       | Complete | UTF-8/16/32 conversion utilities                                              |
| cstring_view  | Complete | Null-terminated string views for C interop. Based on P3655                    |
| defer         | Complete | RAII scope guards for cleaning up dynamic resources from C side               |
| diagnostic    | Planned  | Ergonomic assertions, exceptions, with stacktraces for debugging              |
| static_string | Complete | Compile-time string container                                                 |
| variant       | Complete | Type-safe union with pattern matching and type-safe index access              |

## License

MIT License. See [LICENSE.txt](LICENSE.txt) for details.
s