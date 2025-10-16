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
| defer         | Complete | RAII scope guards for cleaning up dynamic resources from C side               |
| variant       | Complete | Type-safe union with pattern matching and type-safe index access              |
| cstring_view  | Complete | Null-terminated string views for C interop. Based on P3655                    |
| allocator     | Complete | Thread-safe, and efficient memory allocator                                   |
| bitfield      | Planned  | A target-independent bitfield                                                 |
| static_string | Planned  | Compile-time string container                                                 |
| algorithm     | Planned  | Hash combining and search utilities                                           |
| arguments     | Planned  | An encoding-friendly and modern interface for accessing command-line argument |
| codeconv      | Planned  | UTF-8/16/32 conversion utilities                                              |
| diagnostic    | Planned  | Logging and diagnostics infrastructure                                        |
| exception     | Planned  | Exceptions with stack traces                                                  |
| task          | Planned  | Stackful cooperative multitasking                                             |

## License

MIT License. See [LICENSE.txt](LICENSE.txt) for details.
