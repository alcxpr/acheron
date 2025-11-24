/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <compare>
#include <cstddef>
#include <cstring>
#include <format>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>
#include "allocator.hpp"
#include "codecvt.hpp"

#if defined(_WIN32) || defined(_WIN64)
#define ACH_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <io.h>
#include <fcntl.h>
#elif defined(__APPLE__)
#define ACH_PLATFORM_MACOS
#include <crt_externs.h>
#else
#define ACH_PLATFORM_LINUX
#include <cstdio>
#endif

namespace ach
{
    /**
     * @brief Individual command-line argument with encoding conversion support
     *
     * @details
     * Represents a single command-line argument in the platform's native encoding.
     * Provides conversion methods to various character encodings e.g. UTF-8, UTF-16, UTF-32
     *
     * The native encoding varies by platform:
     * - Windows: UTF-16 
     * - Unix-like systems: Implementation-defined though it's typically UTF-8
     *
     * @note It is implementation-defined whether modifications to argv in main()
     * are reflected by this class.
     */
    class argument
    {
    public:
#ifdef ACH_PLATFORM_WINDOWS
        using value_type = wchar_t;
#else
        using value_type = char;
#endif
        using string_type = std::basic_string<value_type>;
        using string_view_type = std::basic_string_view<value_type>;

        constexpr argument(string_view_type str) noexcept : dt(str) {}

        [[nodiscard]] constexpr string_view_type native() const noexcept
        {
            return dt;
        }

        [[nodiscard]] string_type native_string() const
        {
            return string_type(dt);
        }

        [[nodiscard]] constexpr const value_type* c_str() const noexcept
        {
            return dt.data();
        }

        explicit operator string_type() const
        {
            return native_string();
        }

        constexpr explicit operator string_view_type() const noexcept
        {
            return native();
        }

        [[nodiscard]] std::string string() const
        {
#ifdef ACH_PLATFORM_WINDOWS
            std::u16string_view u16_view(reinterpret_cast<const char16_t*>(dt.data()), dt.size());
            return ach::utf16_to_utf8(u16_view);
#else
            return std::string(dt);
#endif
        }

        [[nodiscard]] std::wstring wstring() const
        {
#ifdef ACH_PLATFORM_WINDOWS
            return std::wstring(dt);
#else
            auto utf32 = ach::utf8_to_utf32(std::string_view(dt.data(), dt.size()));
            
            if constexpr (sizeof(wchar_t) == sizeof(char32_t))
            {
                return std::wstring(reinterpret_cast<const wchar_t*>(utf32.data()), utf32.size());
            }
            else if constexpr (sizeof(wchar_t) == sizeof(char16_t))
            {
                auto utf16 = ach::utf32_to_utf16(std::u32string_view(utf32));
                return std::wstring(reinterpret_cast<const wchar_t*>(utf16.data()), utf16.size());
            }
            else
            {
                static_assert(sizeof(wchar_t) == sizeof(char32_t) || sizeof(wchar_t) == sizeof(char16_t),
                             "unsupported wchar_t size");
            }
#endif
        }

        [[nodiscard]] std::u8string u8string() const
        {
            auto str = string();
            return std::u8string(reinterpret_cast<const char8_t*>(str.data()), str.size());
        }

        [[nodiscard]] std::u16string u16string() const
        {
#ifdef ACH_PLATFORM_WINDOWS
            return std::u16string(reinterpret_cast<const char16_t*>(dt.data()), dt.size());
#else
            return ach::utf8_to_utf16(std::string_view(dt.data(), dt.size()));
#endif
        }

        [[nodiscard]] std::u32string u32string() const
        {
#ifdef ACH_PLATFORM_WINDOWS
            std::u16string_view u16_view(reinterpret_cast<const char16_t*>(dt.data()), dt.size());
            return ach::utf16_to_utf32(u16_view);
#else
            return ach::utf8_to_utf32(std::string_view(dt.data(), dt.size()));
#endif
        }

        friend bool operator==(const argument& lhs, const argument& rhs) noexcept
        {
            return lhs.dt == rhs.dt;
        }

        friend std::strong_ordering operator<=>(const argument& lhs, const argument& rhs) noexcept
        {
            return lhs.dt <=> rhs.dt;
        }

        template <typename CharT, typename Traits>
        friend std::basic_ostream<CharT, Traits>& operator<<(
            std::basic_ostream<CharT, Traits>& os, const argument& arg)
        {
            if constexpr (std::is_same_v<CharT, char>)
                return os << arg.string();
            else if constexpr (std::is_same_v<CharT, wchar_t>)
                return os << arg.wstring();
            else if constexpr (std::is_same_v<CharT, char8_t>)
                return os << arg.u8string();
            else if constexpr (std::is_same_v<CharT, char16_t>)
                return os << arg.u16string();
            else if constexpr (std::is_same_v<CharT, char32_t>)
                return os << arg.u32string();
            else
                return os << arg.string();
        }

    private:
        string_view_type dt;
    };

    namespace d
    {
        struct args_data
        {
#ifdef ACH_PLATFORM_WINDOWS
            std::vector<std::wstring> strings;
#else
            std::vector<std::string> strings;
#endif
            std::vector<argument> args;
        };

        inline args_data& get_args_data()
        {
            static args_data data;
            return data;
        }

        inline void initialize_args()
        {
            static std::once_flag flag;
            std::call_once(flag, []()
            {
#ifdef ACH_PLATFORM_WINDOWS
            	_setmode(_fileno(stdout), _O_U16TEXT);
				DWORD mode;
				HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
				if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &mode))
					SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

#endif
                auto& data = get_args_data();

#ifdef ACH_PLATFORM_WINDOWS
                int argc;
                wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
                if (!argv)
                    return;

                data.strings.reserve(argc);
                data.args.reserve(argc);

                for (int i = 0; i < argc; ++i)
                {
                    data.strings.emplace_back(argv[i]);
                    data.args.emplace_back(data.strings.back());
                }

                LocalFree(argv);
#elif defined(ACH_PLATFORM_MACOS)
                char** argv = *_NSGetArgv();
                int argc = *_NSGetArgc();

                if (!argv || argc <= 0)
                    return;

                data.strings.reserve(argc);
                data.args.reserve(argc);

                for (int i = 0; i < argc; ++i)
                {
                    data.strings.emplace_back(argv[i]);
                    data.args.emplace_back(data.strings.back());
                }
#elif defined(ACH_PLATFORM_LINUX)
				std::FILE* f = fopen("/proc/self/cmdline", "rb");
				if (!f)
					return;

				std::vector<char> buffer;
				constexpr std::size_t chunk_size = 4096;
				char temp[chunk_size];

				while (true)
				{
					std::size_t bytes_read = fread(temp, 1, chunk_size, f);
					if (bytes_read == 0)
						break;
					buffer.insert(buffer.end(), temp, temp + bytes_read);
					if (bytes_read < chunk_size)
						break;
				}

				std::fclose(f);
				if (buffer.empty())
					return;

				const char* ptr = buffer.data();
				const char* end = buffer.data() + buffer.size();

				while (ptr < end)
				{
					const char* str_end = ptr;
					while (str_end < end && *str_end != '\0')
						++str_end;

					if (str_end == ptr)
						break;

					data.strings.emplace_back(ptr, str_end - ptr);
					data.args.emplace_back(data.strings.back());

					ptr = str_end + 1;
				}
#endif
            });
        }
    }

    /**
     * @brief Container for command-line arguments
     * @tparam Allocator Allocator type for storing arguments
     *
     * @details
     * Provides access to program command-line arguments with proper encoding support.
     * Arguments are lazily converted to requested encodings on demand.
     *
     * This class can be constructed at any point during program execution and will
     * access the program's arguments. It is implementation-defined whether modifications
     * to argv in main() are reflected.
     *
     * @par Example
     * @code
     * ach::arguments args;
     * if (args.size() > 1 && args[1].string() == "--help")
     * {
     *     print_help();
     * }
     * @endcode
     */
    template <typename Allocator = ach::allocator<argument, allocation_policy::shared>>
    class arguments
    {
    public:
        using value_type = const argument;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using const_pointer = value_type*;
        using reference = value_type&;
        using const_reference = value_type&;
        using allocator_type = Allocator;

    private:
        using storage_type = std::vector<argument, typename std::allocator_traits<Allocator>::template rebind_alloc<argument>>;
        using string_storage_type = std::vector<typename argument::string_type, typename std::allocator_traits<Allocator>::template rebind_alloc<typename argument::string_type>>;

        storage_type args;
        string_storage_type strings;

    public:
        using const_iterator = typename storage_type::const_iterator;
        using iterator = const_iterator;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator = const_reverse_iterator;

        arguments() noexcept(noexcept(Allocator())) : arguments(Allocator()) {}

        explicit arguments(const Allocator& alloc)
            : args(alloc), strings(alloc)
        {
            d::initialize_args();
            auto& data = d::get_args_data();
            
            strings.reserve(data.strings.size());
            args.reserve(data.args.size());

            for (const auto& str : data.strings)
            {
                strings.emplace_back(str);
                args.emplace_back(strings.back());
            }
        }

        arguments(int argc, const typename argument::value_type* const* argv, const Allocator& alloc = Allocator())
            : args(alloc), strings(alloc)
        {
            strings.reserve(argc);
            args.reserve(argc);

            for (int i = 0; i < argc; ++i)
            {
                strings.emplace_back(argv[i]);
                args.emplace_back(strings.back());
            }
        }

        [[nodiscard]] reference operator[](size_type index) const noexcept
        {
            return args[index];
        }

        [[nodiscard]] reference at(size_type index) const
        {
            return args.at(index);
        }

        [[nodiscard]] size_type size() const noexcept
        {
            return args.size();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return args.empty();
        }

        [[nodiscard]] const_iterator begin() const noexcept
        {
            return args.begin();
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            return args.end();
        }

        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return args.cbegin();
        }

        [[nodiscard]] const_iterator cend() const noexcept
        {
            return args.cend();
        }

        [[nodiscard]] const_reverse_iterator rbegin() const noexcept
        {
            return args.rbegin();
        }

        [[nodiscard]] const_reverse_iterator rend() const noexcept
        {
            return args.rend();
        }

        [[nodiscard]] const_reverse_iterator crbegin() const noexcept
        {
            return args.crbegin();
        }

        [[nodiscard]] const_reverse_iterator crend() const noexcept
        {
            return args.crend();
        }
    };

    template <typename Allocator>
    arguments(const Allocator&) -> arguments<Allocator>;
}

namespace std
{
    template <typename CharT>
    struct formatter<ach::argument, CharT>
    {
        constexpr auto parse(basic_format_parse_context<CharT>& ctx)
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const ach::argument& arg, FormatContext& ctx) const
        {
            if constexpr (std::is_same_v<CharT, char>)
                return std::format_to(ctx.out(), "{}", arg.string());
            else if constexpr (std::is_same_v<CharT, wchar_t>)
                return std::format_to(ctx.out(), L"{}", arg.wstring());
            else if constexpr (std::is_same_v<CharT, char8_t>)
                return std::format_to(ctx.out(), u8"{}", arg.u8string());
            else if constexpr (std::is_same_v<CharT, char16_t>)
                return std::format_to(ctx.out(), u"{}", arg.u16string());
            else if constexpr (std::is_same_v<CharT, char32_t>)
                return std::format_to(ctx.out(), U"{}", arg.u32string());
            else
                return std::format_to(ctx.out(), "{}", arg.string());
        }
    };
}

#undef ACH_PLATFORM_WINDOWS
#undef ACH_PLATFORM_MACOS
#undef ACH_PLATFORM_LINUX
