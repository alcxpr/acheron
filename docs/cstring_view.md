# CString View

`ach::cstring_view` provides a non-owning view over null-terminated character sequences, similar to `std::string_view` but with the guarantee that `data()[size()] == '\0'`. This allows safe C interop without additional overhead. The implementation is based on the [P3655](https://wg21.link/P3655) proposal.

The key advantage is guaranteed null-termination, which enables zero-copy C API interop via `c_str()` without needing to create temporary strings or manage additional buffers

### Usage

```cpp
void c_api_call(const char* str); // C function expecting null-terminated string

void safe_wrapper()
{
    ach::cstring_view csv = "hello world";
    c_api_call(csv.c_str()); // Safe, guaranteed null-terminated
}
```

All APIs of `ach::cstring_view` follow the P3655 proposal specification.
