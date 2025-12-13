/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ach
{
	/**
	 * @brief Compile-time bitfield for efficient bit manipulation
	 * @tparam T Underlying integer type for storage
	 *
	 * @details
	 * `bitfield` provides a zero-overhead abstraction for bit manipulation with
	 * compile-time position and width validation. All operations are constexpr
	 * and generate optimal assembly equivalent to manual bit shifting.
	 *
	 * @par Example
	 * @code
	 * // AArch64 ADD immediate encoding
	 * constexpr auto instr = bitfield<std::uint32_t>()
	 *     .set<31, 1>(1)        // sf: 64-bit
	 *     .set<30, 1>(0)        // op: ADD
	 *     .set<29, 1>(0)        // S: no flags
	 *     .set<24, 5>(0b10001)  // opcode
	 *     .set<10, 12>(42)      // immediate
	 *     .set<5, 5>(0)         // Rn
	 *     .set<0, 5>(1)         // Rd
	 *     .raw();
	 * @endcode
	 */
	template<typename T = std::uint32_t>
		requires std::is_unsigned_v<T>
	class bitfield
	{
	public:
		using value_type = T;
		static constexpr std::size_t bit_width = sizeof(T) * 8;

		/** @brief Default constructor; initializes to zero */
		constexpr bitfield() noexcept : value(0)
		{}

		/** @brief Constructs from raw value */
		constexpr explicit bitfield(T val) noexcept : value(val)
		{}

		/**
		 * @brief Sets a bitfield at the specified position
		 * @tparam Pos Starting bit position a.k.a 0-indexed from LSB.
		 * @tparam Width Number of bits to set. Default: 1
		 * @param val Value to set which will be masked to Width bits
		 * @return Reference to *this for chaining
		 */
		template<std::size_t Pos, std::size_t Width = 1>
		constexpr bitfield &set(T val) noexcept
		{
			static_assert(Pos < bit_width, "bit position exceeds type width");
			static_assert(Width > 0, "bit width must be at least 1");
			static_assert(Pos + Width <= bit_width, "bitfield exceeds type width");

			constexpr T mask = make_mask<Width>();
			value &= ~(mask << Pos);
			value |= ((val & mask) << Pos);
			return *this;
		}

		/**
		 * @brief Gets a bitfield at the specified position
		 * @tparam Pos Starting bit position a.k.a 0-indexed from LSB
		 * @tparam Width Number of bits to get. Default: 1
		 * @return Extracted value which is right-aligned
		 */
		template<std::size_t Pos, std::size_t Width = 1>
		[[nodiscard]] constexpr T get() const noexcept
		{
			static_assert(Pos < bit_width, "bit position exceeds type width");
			static_assert(Width > 0, "bit width must be at least 1");
			static_assert(Pos + Width <= bit_width, "bitfield exceeds type width");

			constexpr T mask = make_mask<Width>();
			return (value >> Pos) & mask;
		}

		/**
		 * @brief Clears (zeros) a bitfield at the specified position
		 * @tparam Pos Starting bit position
		 * @tparam Width Number of bits to clear. Default: 1
		 * @return Reference to *this for chaining
		 */
		template<std::size_t Pos, std::size_t Width = 1>
		constexpr bitfield &clear() noexcept
		{
			static_assert(Pos < bit_width, "bit position exceeds type width");
			static_assert(Width > 0, "bit width must be at least 1");
			static_assert(Pos + Width <= bit_width, "bitfield exceeds type width");

			constexpr T mask = make_mask<Width>();
			value &= ~(mask << Pos);
			return *this;
		}

		/**
		 * @brief Tests if any bit in the specified range is set
		 * @tparam Pos Starting bit position
		 * @tparam Width Number of bits to test. Default: 1
		 * @return true if any bit is set, false otherwise
		 */
		template<std::size_t Pos, std::size_t Width = 1>
		[[nodiscard]] constexpr bool test() const noexcept
		{
			static_assert(Pos < bit_width, "bit position exceeds type width");
			static_assert(Width > 0, "bit width must be at least 1");
			static_assert(Pos + Width <= bit_width, "bitfield exceeds type width");

			constexpr T mask = make_mask<Width>();
			return (value & (mask << Pos)) != 0;
		}

		/**
		 * @brief Flips (inverts) bits at the specified position
		 * @tparam Pos Starting bit position
		 * @tparam Width Number of bits to flip. Default: 1
		 * @return Reference to *this for chaining
		 */
		template<std::size_t Pos, std::size_t Width = 1>
		constexpr bitfield &flip() noexcept
		{
			static_assert(Pos < bit_width, "bit position exceeds type width");
			static_assert(Width > 0, "bit width must be at least 1");
			static_assert(Pos + Width <= bit_width, "bitfield exceeds type width");

			constexpr T mask = make_mask<Width>();
			value ^= (mask << Pos);
			return *this;
		}

		/**
		 * @brief Converts the bitfield to the specified endianness
		 * @tparam E Target endianness
		 * @return New bitfield with converted byte order
		 */
		template<std::endian E>
		[[nodiscard]] constexpr bitfield to_endian() const noexcept
		{
			if constexpr (E == std::endian::native)
			{
				return *this;
			}
			else if constexpr (sizeof(T) == 1)
			{
				return *this; /* no conversion needed for single byte */
			}
			else
			{
				return bitfield(std::byteswap(value));
			}
		}

		/** @brief Returns the raw underlying value */
		[[nodiscard]] constexpr T raw() const noexcept
		{
			return value;
		}

		/** @brief Implicit conversion to underlying type */
		constexpr operator T() const noexcept
		{
			return value;
		}

		/** @brief Equality comparison */
		constexpr bool operator==(const bitfield &other) const noexcept = default;

		/** @brief Three-way comparison */
		constexpr auto operator<=>(const bitfield &other) const noexcept = default;

		/** @brief Bitwise AND */
		constexpr bitfield operator&(const bitfield &other) const noexcept
		{
			return bitfield(value & other.value);
		}

		/** @brief Bitwise OR */
		constexpr bitfield operator|(const bitfield &other) const noexcept
		{
			return bitfield(value | other.value);
		}

		/** @brief Bitwise XOR */
		constexpr bitfield operator^(const bitfield &other) const noexcept
		{
			return bitfield(value ^ other.value);
		}

		/** @brief Bitwise NOT */
		constexpr bitfield operator~() const noexcept
		{
			return bitfield(~value);
		}

		/** @brief Bitwise AND assignment */
		constexpr bitfield &operator&=(const bitfield &other) noexcept
		{
			value &= other.value;
			return *this;
		}

		/** @brief Bitwise OR assignment */
		constexpr bitfield &operator|=(const bitfield &other) noexcept
		{
			value |= other.value;
			return *this;
		}

		/** @brief Bitwise XOR assignment */
		constexpr bitfield &operator^=(const bitfield &other) noexcept
		{
			value ^= other.value;
			return *this;
		}

	private:
		T value;

		/** @brief Compile-time mask generation */
		template<std::size_t Width>
		static constexpr T make_mask() noexcept
		{
			if constexpr (Width == bit_width)
				return static_cast<T>(~T{ 0 });
			return (T{ 1 } << Width) - 1;
		}
	};

	/** @brief Deduction guide */
	template<typename T>
	bitfield(T) -> bitfield<T>;
} // namespace ach
