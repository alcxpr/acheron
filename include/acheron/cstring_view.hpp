/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <cassert>
#include <format>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ach
{
    /**
	 * @brief Class template for null-terminated string views
	 * @tparam CharT Character type
	 * @tparam Traits Character traits type
	 *
	 * @details
	 * `basic_cstring_view` provides a non-owning view over a null-terminated character sequence,
	 * similar to `std::basic_string_view` but with the guarantee that the string is null-terminated.
	 * This allows safe C interop via `c_str()` without additional overhead.
	 *
	 * The class guarantees that `data()[size()] == charT()` as the null terminator exists.
	 *
	 * @par Example
	 * @code
	 * void c_api_call(const char* str); // C function expecting null-terminated string
	 *
	 * void safe_wrapper()
	 * {
	 *     ach::cstring_view csv = "hello world";
	 *     c_api_call(csv.c_str()); // Safe. This is guaranteed to be null-terminated
	 * }
	 * @endcode
	 *
	 * @note Based on [P3655](https://wg21.link/P3655)
	 */
    template <class CharT, class Traits = std::char_traits<CharT>>
    class basic_cstring_view
    {
    public:
        using traits_type = Traits;
        using value_type = CharT;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;
        using const_iterator = const CharT*;
        using iterator = const_iterator;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator = const_reverse_iterator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        /** @brief Special value indicating "not found" in search operations */
        static constexpr size_type npos = static_cast<size_type>(-1);

        /**
		 * @brief Default constructor; constructs an empty cstring_view
		 * @post `size() == 0` and `data()` points to a null-terminated empty string
		 */
        constexpr basic_cstring_view() noexcept :
            sz()
        {
            static const CharT empty_string[1]{};
            dt = std::data(empty_string);
        }

        /** @brief Copy constructor */
        constexpr basic_cstring_view(const basic_cstring_view&) noexcept = default;

        /** @brief Copy assignment operator */
        constexpr basic_cstring_view& operator=(const basic_cstring_view&) noexcept = default;

        /**
		 * @brief Constructs from a null-terminated C string
		 * @param str Pointer to null-terminated character array
		 * @pre `str` must point to a null-terminated string
		 */
        constexpr basic_cstring_view(const CharT* str) :
            basic_cstring_view(str, Traits::length(str))
        {
        }

        /**
		 * @brief Constructs from a character array with explicit length
		 * @param str Pointer to character array
		 * @param len Length of the string (excluding null terminator)
		 * @pre `str[len] == charT()` (null terminator must exist at position len)
		 */
        constexpr basic_cstring_view(const CharT* str, size_type len) :
            dt(str), sz(len)
        {
            assert(str[len] == CharT());
        }

        /** @brief Deleted constructor from nullptr */
        basic_cstring_view (std::nullptr_t) = delete;

        /**
		 * @brief Constructs from std::basic_string
		 * @param str Standard string object
		 * @note Provided for convenience; delegates to `c_str()` and `size()`
		 */
        template <typename Traits2, typename Allocator>
        constexpr basic_cstring_view(const std::basic_string<CharT, Traits2, Allocator>& str) :
            basic_cstring_view(str.c_str(), str.size())
        {
        }

        /** @brief Returns iterator to the beginning */
        constexpr const_iterator begin() const noexcept
        {
            return dt;
        }

        /** @brief Returns iterator to the end */
        constexpr const_iterator end() const noexcept
        {
            return dt + sz;
        }

        /** @brief Returns const iterator to the beginning */
        constexpr const_iterator cbegin() const noexcept
        {
            return begin();
        }

        /** @brief Returns const iterator to the end */
        constexpr const_iterator cend() const noexcept
        {
            return end();
        }

        /** @brief Returns reverse iterator to the beginning */
        constexpr const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        /** @brief Returns reverse iterator to the end */
        constexpr const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        /** @brief Returns const reverse iterator to the beginning */
        constexpr const_reverse_iterator crbegin() const noexcept
        {
            return rbegin();
        }

        /** @brief Returns const reverse iterator to the end */
        constexpr const_reverse_iterator crend() const noexcept
        {
            return rend();
        }

        /** @brief Returns the number of characters (excluding null terminator) */
        [[nodiscard]] constexpr size_type size() const noexcept
        {
            return sz;
        }

        /** @brief Returns the number of characters (same as size()) */
        [[nodiscard]] constexpr size_type length() const noexcept
        {
            return sz;
        }

        /** @brief Returns the maximum possible number of characters */
        [[nodiscard]] constexpr size_type max_size() const noexcept
        {
            return std::basic_string_view<CharT, Traits>{}.max_size() - 1;
        }

        /** @brief Checks if the view is empty */
        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return sz == 0;
        }

        /**
		 * @brief Accesses character at specified position (no bounds checking)
		 * @param pos Position of character to access
		 * @return Reference to the character at position `pos`
		 * @note Allows access to null terminator at position `size()`
		 */
        constexpr const_reference operator[](size_type pos) const
        {
            assert(pos <= sz);
            return dt[pos];
        }

        /**
		 * @brief Accesses character at specified position (with bounds checking)
		 * @param pos Position of character to access
		 * @return Reference to the character at position `pos`
		 * @throws std::out_of_range if `pos > size()`
		 */
        constexpr const_reference at(size_type pos) const
        {
            if (pos > sz)
                throw std::out_of_range(std::format("basic_cstring_view::at: pos ({}) > size() ({})", pos, sz));
            return dt[pos];
        }

        /**
		 * @brief Accesses the first character
		 * @return Reference to the first character
		 * @pre `!empty()`
		 */
        constexpr const_reference front() const
        {
            assert(!empty());
            return dt[0];
        }

        /**
		 * @brief Accesses the last character
		 * @return Reference to the last character
		 * @pre `!empty()`
		 */
        constexpr const_reference back() const
        {
            assert(!empty());
            return dt[sz - 1];
        }

        /**
		 * @brief Returns pointer to the underlying character array
		 * @return Pointer to the null-terminated character array
		 */
        [[nodiscard]] constexpr const_pointer data() const noexcept
        {
            return dt;
        }

        /**
		 * @brief Returns pointer to null-terminated C string
		 * @return Pointer to the null-terminated character array
		 * @note This is the primary advantage over std::string_view
		 */
        [[nodiscard]] constexpr const_pointer c_str() const noexcept
        {
            return dt;
        }

        /**
		 * @brief Implicit conversion to std::basic_string_view
		 * @return string_view covering the same character sequence
		 */
        constexpr operator std::basic_string_view<CharT, Traits>() const noexcept
        {
            return std::basic_string_view<CharT, Traits>{dt, sz};
        }

        /**
		 * @brief Moves the start of the view forward by n characters
		 * @param n Number of characters to remove from the beginning
		 * @pre `n <= size()`
		 */
        constexpr void remove_prefix(size_type n)
        {
            assert(n <= size());
            dt += n;
            sz -= n;
        }

        /**
		 * @brief Swaps the contents with another cstring_view
		 * @param s cstring_view to swap with
		 */
        constexpr void swap(basic_cstring_view& s) noexcept
        {
            std::swap(dt, s.dt);
            std::swap(sz, s.sz);
        }

        /**
		 * @brief Copies characters to destination buffer
		 * @param s Destination buffer
		 * @param n Maximum number of characters to copy
		 * @param pos Starting position
		 * @return Number of characters copied
		 */
        constexpr size_type copy(CharT* s, size_type n, size_type pos = 0) const
        {
            return std::basic_string_view<CharT, Traits>(*this).copy(s, n, pos);
        }

        /**
		 * @brief Returns a substring as string_view
		 * @param pos Starting position
		 * @param n Length of substring (npos for entire remaining string)
		 * @return string_view of the substring
		 */
        [[nodiscard]] constexpr std::basic_string_view<CharT, Traits> substr(
            size_type pos = 0, size_type n = npos) const
        {
            return std::basic_string_view<CharT, Traits>(*this).substr(pos, n);
        }

        /** @brief Compares with a string_view */
        constexpr int compare(std::basic_string_view<CharT, Traits> s) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).compare(s);
        }

        /** @brief Compares substring with another cstring_view */
        constexpr int compare(size_type pos1, size_type n1, basic_cstring_view s) const
        {
            return std::basic_string_view<CharT, Traits>(*this).compare(
                pos1, n1, std::basic_string_view<CharT, Traits>(s));
        }

        /** @brief Compares substring with substring of another cstring_view */
        constexpr int compare(size_type pos1, size_type n1, basic_cstring_view s,
                              size_type pos2, size_type n2) const
        {
            return std::basic_string_view<CharT, Traits>(*this).compare(
                pos1, n1, std::basic_string_view<CharT, Traits>(s), pos2, n2);
        }

        /** @brief Compares with C string */
        constexpr int compare(const CharT* s) const
        {
            return std::basic_string_view<CharT, Traits>(*this).compare(s);
        }

        /** @brief Compares substring with C string */
        constexpr int compare(size_type pos1, size_type n1, const CharT* s) const
        {
            return std::basic_string_view<CharT, Traits>(*this).compare(pos1, n1, s);
        }

        /** @brief Compares substring with prefix of C string */
        constexpr int compare(size_type pos1, size_type n1, const CharT* s, size_type n2) const
        {
            return std::basic_string_view<CharT, Traits>(*this).compare(pos1, n1, s, n2);
        }

        /** @brief Checks if the string starts with the given prefix */
        [[nodiscard]] constexpr bool starts_with(std::basic_string_view<CharT, Traits> x) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).starts_with(x);
        }

        /** @brief Checks if the string starts with the given character */
        [[nodiscard]] constexpr bool starts_with(CharT x) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).starts_with(x);
        }

        /** @brief Checks if the string starts with the given C string */
        [[nodiscard]] constexpr bool starts_with(const CharT* x) const
        {
            return std::basic_string_view<CharT, Traits>(*this).starts_with(x);
        }

        /** @brief Checks if the string ends with the given suffix */
        [[nodiscard]] constexpr bool ends_with(std::basic_string_view<CharT, Traits> x) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).ends_with(x);
        }

        /** @brief Checks if the string ends with the given character */
        [[nodiscard]] constexpr bool ends_with(CharT x) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).ends_with(x);
        }

        /** @brief Checks if the string ends with the given C string */
        [[nodiscard]] constexpr bool ends_with(const CharT* x) const
        {
            return std::basic_string_view<CharT, Traits>(*this).ends_with(x);
        }

        /** @brief Checks if the string contains the given substring */
        [[nodiscard]] constexpr bool contains(std::basic_string_view<CharT, Traits> x) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).contains(x);
        }

        /** @brief Checks if the string contains the given character */
        [[nodiscard]] constexpr bool contains(CharT x) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).contains(x);
        }

        /** @brief Checks if the string contains the given C string */
        [[nodiscard]] constexpr bool contains(const CharT* x) const
        {
            return std::basic_string_view<CharT, Traits>(*this).contains(x);
        }

        /** @brief Finds the first occurrence of substring */
        [[nodiscard]] constexpr size_type find(std::basic_string_view<CharT, Traits> s,
                                               size_type pos = 0) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).find(s, pos);
        }

        /** @brief Finds the first occurrence of character */
        [[nodiscard]] constexpr size_type find(CharT c, size_type pos = 0) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).find(c, pos);
        }

        /** @brief Finds the first occurrence of character array */
        [[nodiscard]] constexpr size_type find(const CharT* s, size_type pos, size_type n) const
        {
            return std::basic_string_view<CharT, Traits>(*this).find(s, pos, n);
        }

        /** @brief Finds the first occurrence of C string */
        [[nodiscard]] constexpr size_type find(const CharT* s, size_type pos = 0) const
        {
            return std::basic_string_view<CharT, Traits>(*this).find(s, pos);
        }

        /** @brief Finds the last occurrence of substring */
        [[nodiscard]] constexpr size_type rfind(std::basic_string_view<CharT, Traits> s,
                                                size_type pos = npos) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).rfind(s, pos);
        }

        /** @brief Finds the last occurrence of character */
        [[nodiscard]] constexpr size_type rfind(CharT c, size_type pos = npos) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).rfind(c, pos);
        }

        /** @brief Finds the last occurrence of character array */
        [[nodiscard]] constexpr size_type rfind(const CharT* s, size_type pos, size_type n) const
        {
            return std::basic_string_view<CharT, Traits>(*this).rfind(s, pos, n);
        }

        /** @brief Finds the last occurrence of C string */
        [[nodiscard]] constexpr size_type rfind(const CharT* s, size_type pos = npos) const
        {
            return std::basic_string_view<CharT, Traits>(*this).rfind(s, pos);
        }

        /** @brief Finds first occurrence of any character from the given set */
        [[nodiscard]] constexpr size_type find_first_of(std::basic_string_view<CharT, Traits> s,
                                                        size_type pos = 0) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).find_first_of(s, pos);
        }

        /** @brief Finds first occurrence of character */
        [[nodiscard]] constexpr size_type find_first_of(CharT c, size_type pos = 0) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).find_first_of(c, pos);
        }

        /** @brief Finds first occurrence of any character from character array */
        [[nodiscard]] constexpr size_type find_first_of(const CharT* s, size_type pos, size_type n) const
        {
            return std::basic_string_view<CharT, Traits>(*this).find_first_of(s, pos, n);
        }

        /** @brief Finds first occurrence of any character from C string */
        [[nodiscard]] constexpr size_type find_first_of(const CharT* s, size_type pos = 0) const
        {
            return std::basic_string_view<CharT, Traits>(*this).find_first_of(s, pos);
        }

        /** @brief Finds last occurrence of any character from the given set */
        [[nodiscard]] constexpr size_type find_last_of(std::basic_string_view<CharT, Traits> s,
                                                       size_type pos = npos) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).find_last_of(s, pos);
        }

        /** @brief Finds last occurrence of character */
        [[nodiscard]] constexpr size_type find_last_of(CharT c, size_type pos = npos) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).find_last_of(c, pos);
        }

        /** @brief Finds last occurrence of any character from character array */
        [[nodiscard]] constexpr size_type find_last_of(const CharT* s, size_type pos, size_type n) const
        {
            return std::basic_string_view<CharT, Traits>(*this).find_last_of(s, pos, n);
        }

        /** @brief Finds last occurrence of any character from C string */
        [[nodiscard]] constexpr size_type find_last_of(const CharT* s, size_type pos = npos) const
        {
            return std::basic_string_view<CharT, Traits>(*this).find_last_of(s, pos);
        }

        /** @brief Finds first character not in the given set */
        [[nodiscard]] constexpr size_type find_first_not_of(std::basic_string_view<CharT, Traits> s,
                                                            size_type pos = 0) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).find_first_not_of(s, pos);
        }

        /** @brief Finds first character not equal to given character */
        [[nodiscard]] constexpr size_type find_first_not_of(CharT c, size_type pos = 0) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).find_first_not_of(c, pos);
        }

        /** @brief Finds first character not in character array */
        [[nodiscard]] constexpr size_type find_first_not_of(const CharT* s, size_type pos, size_type n) const
        {
            return std::basic_string_view<CharT, Traits>(*this).find_first_not_of(s, pos, n);
        }

        /** @brief Finds first character not in C string */
        [[nodiscard]] constexpr size_type find_first_not_of(const CharT* s, size_type pos = 0) const
        {
            return std::basic_string_view<CharT, Traits>(*this).find_first_not_of(s, pos);
        }

        /** @brief Finds last character not in the given set */
        [[nodiscard]] constexpr size_type find_last_not_of(std::basic_string_view<CharT, Traits> s,
                                                           size_type pos = npos) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).find_last_not_of(s, pos);
        }

        /** @brief Finds last character not equal to given character */
        [[nodiscard]] constexpr size_type find_last_not_of(CharT c, size_type pos = npos) const noexcept
        {
            return std::basic_string_view<CharT, Traits>(*this).find_last_not_of(c, pos);
        }

        /** @brief Finds last character not in character array */
        [[nodiscard]] constexpr size_type find_last_not_of(const CharT* s, size_type pos, size_type n) const
        {
            return std::basic_string_view<CharT, Traits>(*this).find_last_not_of(s, pos, n);
        }

        /** @brief Finds last character not in C string */
        [[nodiscard]] constexpr size_type find_last_not_of(const CharT* s, size_type pos = npos) const
        {
            return std::basic_string_view<CharT, Traits>(*this).find_last_not_of(s, pos);
        }

    private:
        const_pointer dt; ///< Pointer to null-terminated character array
        size_type sz; ///< Length of string (excluding null terminator)
    };

    /** @brief Null-terminated string view of char */
    using cstring_view = basic_cstring_view<char>;

    /** @brief Null-terminated string view of char8_t */
    using u8cstring_view = basic_cstring_view<char8_t>;

    /** @brief Null-terminated string view of char16_t */
    using u16cstring_view = basic_cstring_view<char16_t>;

    /** @brief Null-terminated string view of char32_t */
    using u32cstring_view = basic_cstring_view<char32_t>;

    /** @brief Null-terminated string view of wchar_t */
    using wcstring_view = basic_cstring_view<wchar_t>;

    /** @brief Equality comparison */
    template <class charT, class traits>
    constexpr bool operator==(
        basic_cstring_view<charT, traits> x,
        std::type_identity_t<basic_cstring_view<charT, traits>> y
    ) noexcept
    {
        return std::basic_string_view<charT, traits>(x) == std::basic_string_view<charT, traits>(y);
    }

    /** @brief Three-way comparison */
    template <class charT, class traits>
    constexpr auto operator<=>(
        basic_cstring_view<charT, traits> x,
        std::type_identity_t<basic_cstring_view<charT, traits>> y
    ) noexcept
    {
        return std::basic_string_view<charT, traits>(x) <=> std::basic_string_view<charT, traits>(y);
    }

    /** @brief Stream insertion operator */
    template <class charT, class traits>
    std::basic_ostream<charT, traits>& operator<<(
        std::basic_ostream<charT, traits>& os,
        basic_cstring_view<charT, traits> str
    )
    {
        return os << std::basic_string_view<charT, traits>(str);
    }

    /** @brief Swaps two cstring_views */
    template <class charT, class traits>
    constexpr void swap(
        basic_cstring_view<charT, traits>& lhs,
        basic_cstring_view<charT, traits>& rhs
    ) noexcept
    {
        lhs.swap(rhs);
    }

    inline namespace literals
    {
        inline namespace cstring_view_literals
        {
#ifndef _MSC_VER
#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wuser-defined-literals"
#else
#pragma GCC diagnostic ignored "-Wliteral-suffix"
#endif
#else
#pragma warning(push)
#pragma warning(disable: 4455)
#endif

            /** @brief User-defined literal for cstring_view */
            constexpr cstring_view operator""_csv(const char* str, std::size_t len) noexcept
            {
                return basic_cstring_view(str, len);
            }

            /** @brief User-defined literal for u8cstring_view */
            constexpr u8cstring_view operator""_csv(const char8_t* str, std::size_t len) noexcept
            {
                return basic_cstring_view(str, len);
            }

            /** @brief User-defined literal for u16cstring_view */
            constexpr u16cstring_view operator""_csv(const char16_t* str, std::size_t len) noexcept
            {
                return basic_cstring_view(str, len);
            }

            /** @brief User-defined literal for u32cstring_view */
            constexpr u32cstring_view operator""_csv(const char32_t* str, std::size_t len) noexcept
            {
                return basic_cstring_view(str, len);
            }

            /** @brief User-defined literal for string_view */
            constexpr wcstring_view operator""_csv(const wchar_t* str, std::size_t len) noexcept
            {
                return basic_cstring_view(str, len);
            }

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#else
#pragma warning(pop)
#endif
        }
    }
}

namespace std::ranges
{
    template <class charT, class traits>
    inline constexpr bool enable_view<ach::basic_cstring_view<charT, traits>> = true;

    template <class charT, class traits>
    inline constexpr bool enable_borrowed_range<ach::basic_cstring_view<charT, traits>> = true;
}

namespace std
{
    /** @brief Hash support for ach::cstring_view */
    template <>
    struct hash<ach::cstring_view>
    {
        [[nodiscard]] auto operator()(const ach::cstring_view& sv) const noexcept
        {
            return std::hash<std::string_view>{}(sv);
        }
    };

    /** @brief Hash support for ach::u8cstring_view */
    template <>
    struct hash<ach::u8cstring_view>
    {
        [[nodiscard]] auto operator()(const ach::u8cstring_view& sv) const noexcept
        {
            return std::hash<std::u8string_view>{}(sv);
        }
    };

    /** @brief Hash support for ach::u16cstring_view */
    template <>
    struct hash<ach::u16cstring_view>
    {
        [[nodiscard]] auto operator()(const ach::u16cstring_view& sv) const noexcept
        {
            return std::hash<std::u16string_view>{}(sv);
        }
    };

    /** @brief Hash support for ach::u32cstring_view */
    template <>
    struct hash<ach::u32cstring_view>
    {
        [[nodiscard]] auto operator()(const ach::u32cstring_view& sv) const noexcept
        {
            return std::hash<std::u32string_view>{}(sv);
        }
    };

    /** @brief Hash support for ach::wcstring_view */
    template <>
    struct hash<ach::wcstring_view>
    {
        [[nodiscard]] auto operator()(const ach::wcstring_view& sv) const noexcept
        {
            return std::hash<std::wstring_view>{}(sv);
        }
    };

    /** @brief std::format support for ach::basic_cstring_view */
    template <class charT, class traits>
    struct formatter<ach::basic_cstring_view<charT, traits>, charT>
    {
        formatter() = default;

        constexpr auto parse(basic_format_parse_context<charT>& context)
        {
            return sv_formatter.parse(context);
        }

        template <typename OutputIt>
        auto format(ach::basic_cstring_view<charT, traits> csv, basic_format_context<OutputIt, charT>& context) const
        {
            return sv_formatter.format(std::basic_string_view<charT, traits>(csv), context);
        }

    private:
        formatter<basic_string_view<charT, traits>, charT> sv_formatter;
    };
}