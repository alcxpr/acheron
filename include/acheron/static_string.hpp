/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <cassert>
#include <cstddef>
#include <format>
#include <string_view>
#include <acheron/cstring_view.hpp>

namespace ach
{
    /**
     * @brief Compile-time fixed-capacity string with null-termination guarantee
     * @tparam N Maximum capacity (excluding null terminator)
     *
     * @details
     * `static_string` provides a compile-time string container with a fixed maximum capacity.
     * Unlike `std::string`, all operations are constexpr and the storage is inline.
     * The string is always null-terminated, making it safe for C interop.
     *
     * @par Example
     * @code
     * constexpr static_string<32> greeting = "Hello";
     * constexpr auto message = greeting + ", World!";
     * static_assert(message == "Hello, World!");
     * @endcode
     */
    template <std::size_t N>
    class static_string
    {
    public:
        using value_type = char;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = char&;
        using const_reference = const char&;
        using pointer = char*;
        using const_pointer = const char*;
        using iterator = char*;
        using const_iterator = const char*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        /** @brief Special value indicating "not found" in search operations */
        static constexpr size_type npos = static_cast<size_type>(-1);

        /**
         * @brief Default constructor; constructs an empty string
         * @post `size() == 0` and `data()[0] == '\0'`
         */
        constexpr static_string() noexcept : len(0)
        {
            dt[0] = '\0';
        }

        /**
         * @brief Constructs from a null-terminated character array
         * @param str Pointer to null-terminated character array
         * @throws std::length_error if string length exceeds capacity N
         */
        constexpr static_string(const char (&str)[N + 1]) : len(0)
        {
            while (str[len] != '\0' && len < N)
            {
                dt[len] = str[len];
                ++len;
            }
            
            assert(str[len] == '\0' && "static_string: input string exceeds capacity");
            
            dt[len] = '\0';
        }

        /**
         * @brief Constructs from a character array with explicit length
         * @param str Pointer to character array
         * @param length Number of characters to copy
         * @throws std::length_error if length exceeds capacity N
         */
        constexpr static_string(const char* str, size_type length) : len(0)
        {
            assert(length <= N && "static_string: input length exceeds capacity");
            
            for (size_type i = 0; i < length; ++i)
            {
                dt[i] = str[i];
            }
            len = length;
            dt[len] = '\0';
        }

        /**
         * @brief Constructs from a string_view
         * @param sv String view to copy from
         * @throws std::length_error if string_view length exceeds capacity N
         */
        constexpr static_string(std::string_view sv) : static_string(sv.data(), sv.size())
        {
        }

        /**
         * @brief Copy constructor from different capacity
         * @tparam M Capacity of source string
         * @param other Source static_string
         * @throws std::length_error if other's length exceeds capacity N
         */
        template <std::size_t M>
        constexpr static_string(const static_string<M>& other) : len(0)
        {
            assert(other.size() <= N && "static_string: source string exceeds capacity");
            
            for (size_type i = 0; i < other.size(); ++i)
            {
                dt[i] = other[i];
            }
            len = other.size();
            dt[len] = '\0';
        }

        /** @brief Copy constructor */
        constexpr static_string(const static_string&) noexcept = default;

        /** @brief Copy assignment operator */
        constexpr static_string& operator=(const static_string&) noexcept = default;

        /** @brief Returns iterator to the beginning */
        [[nodiscard]] constexpr iterator begin() noexcept
        {
            return dt;
        }

        /** @brief Returns const iterator to the beginning */
        [[nodiscard]] constexpr const_iterator begin() const noexcept
        {
            return dt;
        }

        /** @brief Returns iterator to the end */
        [[nodiscard]] constexpr iterator end() noexcept
        {
            return dt + len;
        }

        /** @brief Returns const iterator to the end */
        [[nodiscard]] constexpr const_iterator end() const noexcept
        {
            return dt + len;
        }

        /** @brief Returns const iterator to the beginning */
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept
        {
            return begin();
        }

        /** @brief Returns const iterator to the end */
        [[nodiscard]] constexpr const_iterator cend() const noexcept
        {
            return end();
        }

        /** @brief Returns reverse iterator to the beginning */
        [[nodiscard]] constexpr reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        /** @brief Returns const reverse iterator to the beginning */
        [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        /** @brief Returns reverse iterator to the end */
        constexpr reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        /** @brief Returns const reverse iterator to the end */
        [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        /** @brief Returns const reverse iterator to the beginning */
        [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
        {
            return rbegin();
        }

        /** @brief Returns const reverse iterator to the end */
        [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
        {
            return rend();
        }

        /** @brief Returns the number of characters (excluding null terminator) */
        [[nodiscard]] constexpr size_type size() const noexcept
        {
            return len;
        }

        /** @brief Returns the number of characters (same as size()) */
        [[nodiscard]] constexpr size_type length() const noexcept
        {
            return len;
        }

        /** @brief Returns the maximum possible number of characters */
        [[nodiscard]] constexpr size_type max_size() const noexcept
        {
            return N;
        }

        /** @brief Returns the capacity (same as N) */
        [[nodiscard]] constexpr size_type capacity() const noexcept
        {
            return N;
        }

        /** @brief Checks if the string is empty */
        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return len == 0;
        }

        /**
         * @brief Accesses character at specified position (no bounds checking)
         * @param pos Position of character to access
         * @return Reference to the character at position `pos`
         * @note Allows access to null terminator at position `size()`
         */
        constexpr reference operator[](size_type pos) noexcept
        {
            return dt[pos];
        }

        /**
         * @brief Accesses character at specified position (no bounds checking)
         * @param pos Position of character to access
         * @return Const reference to the character at position `pos`
         */
        constexpr const_reference operator[](size_type pos) const noexcept
        {
            return dt[pos];
        }

        /**
         * @brief Accesses character at specified position (with bounds checking)
         * @param pos Position of character to access
         * @return Reference to the character at position `pos`
         * @pre `pos < size()`
         */
        constexpr reference at(size_type pos)
        {
            assert(pos < len && "static_string::at: pos >= size()");
            return dt[pos];
        }

        /**
         * @brief Accesses character at specified position (with bounds checking)
         * @param pos Position of character to access
         * @return Const reference to the character at position `pos`
         * @pre `pos < size()`
         */
        [[nodiscard]] constexpr const_reference at(size_type pos) const
        {
            assert(pos < len && "static_string::at: pos >= size()");
            return dt[pos];
        }

        /**
         * @brief Accesses the first character
         * @return Reference to the first character
         * @pre `!empty()`
         */
        [[nodiscard]] constexpr reference front() noexcept
        {
            return dt[0];
        }

        /**
         * @brief Accesses the first character
         * @return Const reference to the first character
         * @pre `!empty()`
         */
        [[nodiscard]] constexpr const_reference front() const noexcept
        {
            return dt[0];
        }

        /**
         * @brief Accesses the last character
         * @return Reference to the last character
         * @pre `!empty()`
         */
        constexpr reference back() noexcept
        {
            return dt[len - 1];
        }

        /**
         * @brief Accesses the last character
         * @return Const reference to the last character
         * @pre `!empty()`
         */
        [[nodiscard]] constexpr const_reference back() const noexcept
        {
            return dt[len - 1];
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
         */
        [[nodiscard]] constexpr const_pointer c_str() const noexcept
        {
            return dt;
        }

        /**
         * @brief Implicit conversion to std::string_view
         * @return string_view covering the same character sequence
         */
        constexpr operator std::string_view() const noexcept
        {
            return std::string_view{dt, len};
        }

        /**
         * @brief Clears the string
         * @post `size() == 0` and `data()[0] == '\0'`
         */
        constexpr void clear() noexcept
        {
            len = 0;
            dt[0] = '\0';
        }

        /**
         * @brief Appends a character to the end
         * @param ch Character to append
         * @pre `size() < capacity()`
         */
        constexpr void push_back(char ch)
        {
            assert(len < N && "static_string::push_back: capacity exceeded");
            
            dt[len] = ch;
            ++len;
            dt[len] = '\0';
        }

        /**
         * @brief Removes the last character
         * @pre `!empty()`
         */
        constexpr void pop_back() noexcept
        {
            if (len > 0)
            {
                --len;
                dt[len] = '\0';
            }
        }

        /**
         * @brief Appends a string
         * @param str String to append
         * @pre `size() + str.size() <= capacity()`
         */
        constexpr static_string& operator+=(std::string_view str)
        {
            assert(len + str.size() <= N && "static_string::operator+=: capacity exceeded");
            
            for (size_type i = 0; i < str.size(); ++i)
            {
                dt[len + i] = str[i];
            }
            len += str.size();
            dt[len] = '\0';
            return *this;
        }

        /**
         * @brief Appends a character
         * @param ch Character to append
         * @pre `size() < capacity()`
         */
        constexpr static_string& operator+=(char ch)
        {
            push_back(ch);
            return *this;
        }

        /**
         * @brief Appends another static_string
         * @tparam M Capacity of other string
         * @param other String to append
         * @pre `size() + other.size() <= capacity()`
         */
        template <std::size_t M>
        constexpr static_string& operator+=(const static_string<M>& other)
        {
            return *this += std::string_view(other);
        }

        /**
         * @brief Compares with a string_view
         * @param sv String view to compare with
         * @return Negative if less, 0 if equal, positive if greater
         */
        [[nodiscard]] constexpr int compare(std::string_view sv) const noexcept
        {
            return std::string_view(*this).compare(sv);
        }

        /**
         * @brief Checks if the string starts with the given prefix
         * @param sv Prefix to check
         * @return true if string starts with sv
         */
        [[nodiscard]] constexpr bool starts_with(std::string_view sv) const noexcept
        {
            return std::string_view(*this).starts_with(sv);
        }

        /**
         * @brief Checks if the string starts with the given character
         * @param ch Character to check
         * @return true if string starts with ch
         */
        [[nodiscard]] constexpr bool starts_with(char ch) const noexcept
        {
            return std::string_view(*this).starts_with(ch);
        }

        /**
         * @brief Checks if the string ends with the given suffix
         * @param sv Suffix to check
         * @return true if string ends with sv
         */
        [[nodiscard]] constexpr bool ends_with(std::string_view sv) const noexcept
        {
            return std::string_view(*this).ends_with(sv);
        }

        /**
         * @brief Checks if the string ends with the given character
         * @param ch Character to check
         * @return true if string ends with ch
         */
        [[nodiscard]] constexpr bool ends_with(char ch) const noexcept
        {
            return std::string_view(*this).ends_with(ch);
        }

        /**
         * @brief Checks if the string contains the given substring
         * @param sv Substring to search for
         * @return true if string contains sv
         */
        [[nodiscard]] constexpr bool contains(std::string_view sv) const noexcept
        {
            return std::string_view(*this).contains(sv);
        }

        /**
         * @brief Checks if the string contains the given character
         * @param ch Character to search for
         * @return true if string contains ch
         */
        [[nodiscard]] constexpr bool contains(char ch) const noexcept
        {
            return std::string_view(*this).contains(ch);
        }

        /**
         * @brief Returns a substring as string_view
         * @param pos Starting position
         * @param count Length of substring (npos for entire remaining string)
         * @return string_view of the substring
         */
        [[nodiscard]] constexpr std::string_view substr(size_type pos = 0, size_type count = npos) const
        {
            return std::string_view(*this).substr(pos, count);
        }

        /**
         * @brief Finds the first occurrence of substring
         * @param sv Substring to search for
         * @param pos Starting position
         * @return Position of first occurrence, or npos if not found
         */
        [[nodiscard]] constexpr size_type find(std::string_view sv, size_type pos = 0) const noexcept
        {
            return std::string_view(*this).find(sv, pos);
        }

        /**
         * @brief Finds the first occurrence of character
         * @param ch Character to search for
         * @param pos Starting position
         * @return Position of first occurrence, or npos if not found
         */
        [[nodiscard]] constexpr size_type find(char ch, size_type pos = 0) const noexcept
        {
            return std::string_view(*this).find(ch, pos);
        }

        /**
         * @brief Finds the last occurrence of substring
         * @param sv Substring to search for
         * @param pos Starting position (searches backwards from here)
         * @return Position of last occurrence, or npos if not found
         */
        [[nodiscard]] constexpr size_type rfind(std::string_view sv, size_type pos = npos) const noexcept
        {
            return std::string_view(*this).rfind(sv, pos);
        }

        /**
         * @brief Finds the last occurrence of character
         * @param ch Character to search for
         * @param pos Starting position (searches backwards from here)
         * @return Position of last occurrence, or npos if not found
         */
        [[nodiscard]] constexpr size_type rfind(char ch, size_type pos = npos) const noexcept
        {
            return std::string_view(*this).rfind(ch, pos);
        }

    private:
        char dt[N + 1]; ///< Character storage (includes null terminator)
        size_type len; ///< Current length (excluding null terminator)
    };

    /** @brief Deduction guide for string literals */
    template <std::size_t N>
    static_string(const char (&)[N]) -> static_string<N - 1>;

    /** @brief Equality comparison */
    template <std::size_t N1, std::size_t N2>
    constexpr bool operator==(const static_string<N1>& lhs, const static_string<N2>& rhs) noexcept
    {
        return std::string_view(lhs) == std::string_view(rhs);
    }

    /** @brief Equality comparison with string_view */
    template <std::size_t N>
    constexpr bool operator==(const static_string<N>& lhs, std::string_view rhs) noexcept
    {
        return std::string_view(lhs) == rhs;
    }

    /** @brief Equality comparison with C string */
    template <std::size_t N>
    constexpr bool operator==(const static_string<N>& lhs, const char* rhs) noexcept
    {
        return std::string_view(lhs) == rhs;
    }

    /** @brief Three-way comparison */
    template <std::size_t N1, std::size_t N2>
    constexpr auto operator<=>(const static_string<N1>& lhs, const static_string<N2>& rhs) noexcept
    {
        return std::string_view(lhs) <=> std::string_view(rhs);
    }

    /** @brief Three-way comparison with string_view */
    template <std::size_t N>
    constexpr auto operator<=>(const static_string<N>& lhs, std::string_view rhs) noexcept
    {
        return std::string_view(lhs) <=> rhs;
    }

    /** @brief Three-way comparison with C string */
    template <std::size_t N>
    constexpr auto operator<=>(const static_string<N>& lhs, const char* rhs) noexcept
    {
        return std::string_view(lhs) <=> std::string_view(rhs);
    }

    /** @brief Concatenation of two static_strings */
    template <std::size_t N1, std::size_t N2>
    constexpr auto operator+(const static_string<N1>& lhs, const static_string<N2>& rhs)
    {
        static_string<N1 + N2> result;
        result += std::string_view(lhs);
        result += std::string_view(rhs);
        return result;
    }

    /** @brief Concatenation with string_view */
    template <std::size_t N>
    constexpr auto operator+(const static_string<N>& lhs, std::string_view rhs)
    {
        static_string<N + 256> result;
        result += std::string_view(lhs);
        result += rhs;
        return result;
    }

    /** @brief Concatenation with string_view (reversed) */
    template <std::size_t N>
    constexpr auto operator+(std::string_view lhs, const static_string<N>& rhs)
    {
        static_string<256 + N> result;
        result += lhs;
        result += std::string_view(rhs);
        return result;
    }

    /** @brief Concatenation with C string */
    template <std::size_t N, std::size_t M>
    constexpr auto operator+(const static_string<N>& lhs, const char (&rhs)[M])
    {
        static_string<N + M - 1> result;
        result += std::string_view(lhs);
        result += std::string_view(rhs, M - 1);
        return result;
    }

    /** @brief Concatenation with C string (reversed) */
    template <std::size_t N, std::size_t M>
    constexpr auto operator+(const char (&lhs)[M], const static_string<N>& rhs)
    {
        static_string<M - 1 + N> result;
        result += std::string_view(lhs, M - 1);
        result += std::string_view(rhs);
        return result;
    }

    /** @brief Concatenation with character */
    template <std::size_t N>
    constexpr auto operator+(const static_string<N>& lhs, char rhs)
    {
        static_string<N + 1> result;
        result += std::string_view(lhs);
        result += rhs;
        return result;
    }

    /** @brief Concatenation with character (reversed) */
    template <std::size_t N>
    constexpr auto operator+(char lhs, const static_string<N>& rhs)
    {
        static_string<1 + N> result;
        result += lhs;
        result += std::string_view(rhs);
        return result;
    }

    /** @brief Stream insertion operator */
    template <std::size_t N>
    std::ostream& operator<<(std::ostream& os, const static_string<N>& str)
    {
        return os << std::string_view(str);
    }

    inline namespace literals
    {
        inline namespace static_string_literals
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

            /** @brief User-defined literal for static_string */
            template <typename CharT, CharT... Chars>
            constexpr auto operator""_ss() noexcept
            {
                constexpr char str[] = { Chars..., '\0' };
                return static_string<sizeof...(Chars)>(str, sizeof...(Chars));
            }

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#else
#pragma warning(pop)
#endif
        }
    }
}

namespace std
{
    /** @brief Hash support for ach::static_string */
    template <std::size_t N>
    struct hash<ach::static_string<N>>
    {
        [[nodiscard]] constexpr auto operator()(const ach::static_string<N>& str) const noexcept
        {
            return std::hash<std::string_view>{}(std::string_view(str));
        }
    };

    /** @brief std::format support for ach::static_string */
    template <std::size_t N>
    struct formatter<ach::static_string<N>, char>
    {
        formatter() = default;

        constexpr auto parse(basic_format_parse_context<char>& context)
        {
            return sv_formatter.parse(context);
        }

        template <typename OutputIt>
        auto format(const ach::static_string<N>& str, basic_format_context<OutputIt, char>& context) const
        {
            return sv_formatter.format(std::string_view(str), context);
        }

    private:
        formatter<string_view, char> sv_formatter;
    };
}
