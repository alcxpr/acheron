/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <bit>
#include <concepts>

namespace ach
{
	template<std::unsigned_integral T>
	constexpr int countrz(T x) noexcept
	{
		return std::countr_zero(x);
	}

	template<std::unsigned_integral T>
	constexpr int countlz(T x) noexcept
	{
		return std::countl_zero(x);
	}

	template<std::unsigned_integral T>
	constexpr int countr_one(T x) noexcept
	{
		return std::countr_one(x);
	}

	template<std::unsigned_integral T>
	constexpr int countl_one(T x) noexcept
	{
		return std::countl_one(x);
	}

	template<std::unsigned_integral T>
	constexpr int popcount(T x) noexcept
	{
		return std::popcount(x);
	}
	template<std::unsigned_integral T>
	constexpr int ffs(T x) noexcept
	{
		return x ? countrz(x) + 1 : 0;
	}

	template<std::unsigned_integral T>
	constexpr int fls(T x) noexcept
	{
		return x ? (std::bit_width(x)) : 0;
	}

	template<std::unsigned_integral T>
	constexpr bool is_power_of_2(T x) noexcept
	{
		return std::has_single_bit(x);
	}

	template<std::unsigned_integral T>
	constexpr T prev_power_of_2(T x) noexcept
	{
		return x == 0 ? 0 : std::bit_floor(x);
	}

	template<std::unsigned_integral T>
	constexpr T rotl(T x, int s) noexcept
	{
		return std::rotl(x, s);
	}

	template<std::unsigned_integral T>
	constexpr T rotr(T x, int s) noexcept
	{
		return std::rotr(x, s);
	}

	template<std::unsigned_integral T>
	constexpr T extract(T value, int start, int width) noexcept
	{
		T mask = (T{ 1 } << width) - 1;
		return (value >> start) & mask;
	}

	template<std::unsigned_integral T>
	constexpr T deposit(T target, T value, int start, int width) noexcept
	{
		T mask = ((T{ 1 } << width) - 1) << start;
		return (target & ~mask) | ((value << start) & mask);
	}

	template<std::unsigned_integral T>
	constexpr T pdep(T value, T mask) noexcept
	{
#if defined(__BMI2__) && defined(__x86_64__)
		if constexpr (sizeof(T) == 8)
			return _pdep_u64(value, mask);
		else if constexpr (sizeof(T) == 4)
			return _pdep_u32(value, mask);
#endif

		/* software fallback */
		T result = 0;
		for (T bb = 1; mask; bb += bb)
		{
			if (value & bb)
				result |= mask & -mask;
			mask &= mask - 1;
		}
		return result;
	}

	template<std::unsigned_integral T>
	constexpr T pext(T value, T mask) noexcept
	{
#if defined(__BMI2__) && defined(__x86_64__)
		if constexpr (sizeof(T) == 8)
			return _pext_u64(value, mask);
		else if constexpr (sizeof(T) == 4)
			return _pext_u32(value, mask);
#endif

		/* software fallback */
		T result = 0;
		for (T bb = 1; mask; bb += bb)
		{
			if (value & mask & -mask)
				result |= bb;
			mask &= mask - 1;
		}
		return result;
	}

	template<std::unsigned_integral T>
	constexpr T reverse(T x) noexcept
	{
		T result = 0;
		constexpr int bits = sizeof(T) * 8;
		for (int i = 0; i < bits; ++i)
		{
			result <<= 1;
			result |= x & 1;
			x >>= 1;
		}
		return result;
	}

	template<std::unsigned_integral T>
  constexpr T byteswap(T x) noexcept
	{
		return std::byteswap(x);
	}
} // namespace ach
