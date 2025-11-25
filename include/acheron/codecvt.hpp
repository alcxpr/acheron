/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include "diagnostic.hpp"

namespace ach
{
    namespace d
    {
        /* lookup tables */
        inline constexpr std::array<std::uint8_t, 7> first_byte_mark = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
        
        inline constexpr std::array<std::uint8_t, 256> utf8_bytes = {
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
        };
        
        inline constexpr std::array<std::uint32_t, 6> utf8_offsets = {
            0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL
        };

        /* unicode replacement character for invalid sequences */
        inline constexpr std::uint32_t replacement_char = 0x0000FFFD;

        /* utf-32 to utf-16 conversion */
        template <typename Iter16>
        constexpr void utf32_to_utf16_char(std::uint32_t c, Iter16& begin, Iter16 end)
        {
            constexpr std::uint32_t max_utf32 = 0x0010FFFF;
            constexpr std::uint32_t high_begin = 0xD800;
            [[maybe_unused]] constexpr std::uint32_t high_end = 0xDBFF;
            constexpr std::uint32_t low_begin = 0xDC00;
            constexpr std::uint32_t low_end = 0xDFFF;
            constexpr std::uint32_t max_bmp = 0x0000FFFF;
            constexpr int shift = 10;
            constexpr std::uint32_t base = 0x0010000UL;
            constexpr std::uint32_t mask = 0x3FFUL;

            if (c <= max_bmp)
            {
                if (c >= high_begin && c <= low_end)
                    *begin++ = static_cast<std::uint16_t>(replacement_char);
                else
                    *begin++ = static_cast<std::uint16_t>(c);
            }
            else if (c > max_utf32)
                *begin++ = static_cast<std::uint16_t>(replacement_char);
            else
            {
                ach::expect(begin + 1 < end, "utf32_to_utf16: buffer overflow");
                c -= base;
                *begin++ = static_cast<std::uint16_t>((c >> shift) + high_begin);
                *begin++ = static_cast<std::uint16_t>((c & mask) + low_begin);
            }
        }

        /* utf-32 to utf-8 conversion */
        template <typename Iter8>
        constexpr void utf32_to_utf8_char(std::uint32_t c, Iter8& begin, Iter8 end)
        {
            constexpr std::uint32_t max_utf32 = 0x0010FFFF;
            constexpr std::uint32_t bytemark = 0x80;
            constexpr std::uint32_t bytemask = 0xBF;

            std::uint8_t bytes;
            if (c < 0x80)
                bytes = 1;
            else if (c < 0x800)
                bytes = 2;
            else if (c < 0x10000)
                bytes = 3;
            else if (c <= max_utf32)
                bytes = 4;
            else
            {
                bytes = 3;
                c = replacement_char;
            }

            ach::expect(begin + bytes <= end, "utf32_to_utf8: buffer overflow");

            begin += bytes;
            switch (bytes)
            {
                case 4:
                    *--begin = static_cast<std::uint8_t>((c | bytemark) & bytemask);
                    c >>= 6;
                    [[fallthrough]];
                case 3:
                    *--begin = static_cast<std::uint8_t>((c | bytemark) & bytemask);
                    c >>= 6;
                    [[fallthrough]];
                case 2:
                    *--begin = static_cast<std::uint8_t>((c | bytemark) & bytemask);
                    c >>= 6;
                    [[fallthrough]];
                case 1:
                    *--begin = static_cast<std::uint8_t>(c | first_byte_mark[bytes]);
            }
            begin += bytes;
        }

        /* utf-16 to utf-32 conversion */
        template <typename Iter16>
        constexpr std::uint32_t utf16_to_utf32_char(Iter16& begin, Iter16 end)
        {
            constexpr std::uint32_t high_begin = 0xD800;
            constexpr std::uint32_t high_end = 0xDBFF;
            constexpr std::uint32_t low_begin = 0xDC00;
            constexpr std::uint32_t low_end = 0xDFFF;
            constexpr int shift = 10;
            constexpr std::uint32_t base = 0x0010000UL;

            const std::uint32_t c1 = *begin++;
            if (c1 >= high_begin && c1 <= high_end)
            {
                ach::expect(begin < end, "utf16_to_utf32: incomplete surrogate pair");
                const std::uint32_t c2 = *begin++;
                if (c2 >= low_begin && c2 <= low_end)
                    return ((c1 - high_begin) << shift) + (c2 - low_begin) + base;
                else
                    return replacement_char;
            }
            else if (c1 >= low_begin && c1 <= low_end)
                return replacement_char;
            else
                return c1;
        }

        /* utf-8 to utf-32 conversion */
        template <typename Iter8>
        constexpr std::uint32_t utf8_to_utf32_char(Iter8& begin, Iter8 end)
        {
            std::uint32_t c = 0;
            std::uint8_t bytes = utf8_bytes[static_cast<std::uint8_t>(*begin)];

            ach::expect(begin + bytes < end, "utf8_to_utf32: incomplete sequence");
            switch (bytes)
            {
                case 5:
                    c = replacement_char;
                    c <<= 6;
                    [[fallthrough]];
                case 4:
                    c = replacement_char;
                    c <<= 6;
                    [[fallthrough]];
                case 3:
                    c += static_cast<std::uint8_t>(*begin++);
                    c <<= 6;
                    [[fallthrough]];
                case 2:
                    c += static_cast<std::uint8_t>(*begin++);
                    c <<= 6;
                    [[fallthrough]];
                case 1:
                    c += static_cast<std::uint8_t>(*begin++);
                    c <<= 6;
                    [[fallthrough]];
                case 0:
                    c += static_cast<std::uint8_t>(*begin++);
            }
            c -= utf8_offsets[bytes];

            return c;
        }
    }

    /**
     * @brief Convert UTF-32 to UTF-16
     * @tparam Iter32 Input iterator type (UTF-32)
     * @tparam Iter16 Output iterator type (UTF-16)
     * @param src_begin Beginning of source range
     * @param src_end End of source range
     * @param dst_begin Beginning of destination range
     * @param dst_end End of destination range
     * @return Number of UTF-16 code units written
     */
    template <typename Iter32, typename Iter16>
    constexpr std::size_t utf32_to_utf16(Iter32 src_begin, Iter32 src_end,
                                         Iter16 dst_begin, Iter16 dst_end)
    {
        auto src = src_begin;
        auto dst = dst_begin;
        while (src < src_end && dst < dst_end)
            d::utf32_to_utf16_char(*src++, dst, dst_end);
        return dst - dst_begin;
    }

    /**
     * @brief Convert UTF-16 to UTF-32
     * @tparam Iter16 Input iterator type (UTF-16)
     * @tparam Iter32 Output iterator type (UTF-32)
     * @param src_begin Beginning of source range
     * @param src_end End of source range
     * @param dst_begin Beginning of destination range
     * @param dst_end End of destination range
     * @return Number of UTF-32 code points written
     */
    template <typename Iter16, typename Iter32>
    constexpr std::size_t utf16_to_utf32(Iter16 src_begin, Iter16 src_end,
                                         Iter32 dst_begin, Iter32 dst_end)
    {
        auto src = src_begin;
        auto dst = dst_begin;
        while (src < src_end && dst < dst_end)
            *dst++ = d::utf16_to_utf32_char(src, src_end);
        return dst - dst_begin;
    }

    /**
     * @brief Convert UTF-16 to UTF-8
     * @tparam Iter16 Input iterator type (UTF-16)
     * @tparam Iter8 Output iterator type (UTF-8)
     * @param src_begin Beginning of source range
     * @param src_end End of source range
     * @param dst_begin Beginning of destination range
     * @param dst_end End of destination range
     * @return Number of UTF-8 code units written
     */
    template <typename Iter16, typename Iter8>
    constexpr std::size_t utf16_to_utf8(Iter16 src_begin, Iter16 src_end,
                                        Iter8 dst_begin, Iter8 dst_end)
    {
        auto src = src_begin;
        auto dst = dst_begin;
        while (src < src_end && dst < dst_end)
            d::utf32_to_utf8_char(d::utf16_to_utf32_char(src, src_end), dst, dst_end);
        return dst - dst_begin;
    }

    /**
     * @brief Convert UTF-8 to UTF-16
     * @tparam Iter8 Input iterator type (UTF-8)
     * @tparam Iter16 Output iterator type (UTF-16)
     * @param src_begin Beginning of source range
     * @param src_end End of source range
     * @param dst_begin Beginning of destination range
     * @param dst_end End of destination range
     * @return Number of UTF-16 code units written
     */
    template <typename Iter8, typename Iter16>
    constexpr std::size_t utf8_to_utf16(Iter8 src_begin, Iter8 src_end,
                                        Iter16 dst_begin, Iter16 dst_end)
    {
        auto src = src_begin;
        auto dst = dst_begin;
        while (src < src_end && dst < dst_end)
            d::utf32_to_utf16_char(d::utf8_to_utf32_char(src, src_end), dst, dst_end);
        return dst - dst_begin;
    }

    /**
     * @brief Convert UTF-32 to UTF-8
     * @tparam Iter32 Input iterator type (UTF-32)
     * @tparam Iter8 Output iterator type (UTF-8)
     * @param src_begin Beginning of source range
     * @param src_end End of source range
     * @param dst_begin Beginning of destination range
     * @param dst_end End of destination range
     * @return Number of UTF-8 code units written
     */
    template <typename Iter32, typename Iter8>
    constexpr std::size_t utf32_to_utf8(Iter32 src_begin, Iter32 src_end,
                                        Iter8 dst_begin, Iter8 dst_end)
    {
        auto src = src_begin;
        auto dst = dst_begin;
        while (src < src_end && dst < dst_end)
            d::utf32_to_utf8_char(*src++, dst, dst_end);
        return dst - dst_begin;
    }

    /**
     * @brief Convert UTF-8 to UTF-32
     * @tparam Iter8 Input iterator type (UTF-8)
     * @tparam Iter32 Output iterator type (UTF-32)
     * @param src_begin Beginning of source range
     * @param src_end End of source range
     * @param dst_begin Beginning of destination range
     * @param dst_end End of destination range
     * @return Number of UTF-32 code points written
     */
    template <typename Iter8, typename Iter32>
    constexpr std::size_t utf8_to_utf32(Iter8 src_begin, Iter8 src_end,
                                        Iter32 dst_begin, Iter32 dst_end)
    {
        auto src = src_begin;
        auto dst = dst_begin;
        while (src < src_end && dst < dst_end)
            *dst++ = d::utf8_to_utf32_char(src, src_end);
        return dst - dst_begin;
    }

    /**
     * @brief Convert UTF-8 string to UTF-16
     * @param input UTF-8 encoded string
     * @return UTF-16 encoded string as std::u16string
     */
    inline std::u16string utf8_to_utf16(std::string_view input)
    {
        std::u16string output(input.size(), u'\0');
        auto src = reinterpret_cast<const std::uint8_t*>(input.data());
        auto dst = output.data();
        std::size_t len = utf8_to_utf16(src, src + input.size(), dst, dst + output.size());
        output.resize(len);
        return output;
    }

    /**
     * @brief Convert UTF-8 string to UTF-32
     * @param input UTF-8 encoded string
     * @return UTF-32 encoded string as std::u32string
     */
    inline std::u32string utf8_to_utf32(std::string_view input)
    {
        std::u32string output(input.size(), U'\0');
        auto src = reinterpret_cast<const std::uint8_t*>(input.data());
        auto dst = output.data();
        std::size_t len = utf8_to_utf32(src, src + input.size(), dst, dst + output.size());
        output.resize(len);
        return output;
    }

    /**
     * @brief Convert UTF-16 string to UTF-8
     * @param input UTF-16 encoded string
     * @return UTF-8 encoded string as std::string
     */
    inline std::string utf16_to_utf8(std::u16string_view input)
    {
        std::string output(input.size() * 4, '\0');
        auto src = reinterpret_cast<const char16_t*>(input.data());
        auto dst = reinterpret_cast<std::uint8_t*>(output.data());
        std::size_t len = utf16_to_utf8(src, src + input.size(), dst, dst + output.size());
        output.resize(len);
        return output;
    }

    /**
     * @brief Convert UTF-16 string to UTF-32
     * @param input UTF-16 encoded string
     * @return UTF-32 encoded string as std::u32string
     */
    inline std::u32string utf16_to_utf32(std::u16string_view input)
    {
        std::u32string output(input.size(), U'\0');
        auto src = reinterpret_cast<const char16_t*>(input.data());
        auto dst = reinterpret_cast<char32_t*>(output.data());
        std::size_t len = utf16_to_utf32(src, src + input.size(), dst, dst + output.size());
        output.resize(len);
        return output;
    }

    /**
     * @brief Convert UTF-32 string to UTF-8
     * @param input UTF-32 encoded string
     * @return UTF-8 encoded string as std::string
     */
    inline std::string utf32_to_utf8(std::u32string_view input)
    {
        std::string output(input.size() * 4, '\0');
        auto src = reinterpret_cast<const char32_t*>(input.data());
        auto dst = reinterpret_cast<std::uint8_t*>(output.data());
        std::size_t len = utf32_to_utf8(src, src + input.size(), dst, dst + output.size());
        output.resize(len);
        return output;
    }

    /**
     * @brief Convert UTF-32 string to UTF-16
     * @param input UTF-32 encoded string
     * @return UTF-16 encoded string as std::u16string
     */
    inline std::u16string utf32_to_utf16(std::u32string_view input)
    {
        std::u16string output(input.size() * 2, u'\0');
        auto src = input.data();
        auto dst = output.data();
        std::size_t len = utf32_to_utf16(src, src + input.size(), dst, dst + output.size());
        output.resize(len);
        return output;
    }
}
