/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <bit>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include "allocator.hpp"
#include "bits/config.h"

namespace ach
{
	template<std::input_iterator It, std::sentinel_for<It> S, typename F>
	constexpr void enumerate(It first, S last, F &&f)
	{
		std::size_t i = 0;
		for (; first != last; ++first, ++i)
			std::invoke(f, i, *first);
	}

	namespace ranges
	{
		template<std::ranges::input_range R, typename F>
		constexpr void enumerate(R &&r, F &&f)
		{
			::ach::enumerate(std::ranges::begin(r), std::ranges::end(r), std::forward<F>(f));
		}
	} // namespace ranges

	template<std::input_iterator It, std::sentinel_for<It> S, typename F>
	[[nodiscard]] constexpr std::size_t hash_combine(It first, S last, F &&f)
	{
		std::size_t h = 0xcbf29ce484222325ULL;
		for (; first != last; ++first)
		{
			auto v = std::invoke(f, *first);
			std::size_t hv = std::hash<std::decay_t<decltype(v)>>{}(v);
			h ^= hv + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
		}
		return h;
	}

	namespace ranges
	{
		template<std::ranges::input_range R, typename F>
		[[nodiscard]] constexpr std::size_t hash_combine(R &&r, F &&f)
		{
			return ::ach::hash_combine(std::ranges::begin(r), std::ranges::end(r), std::forward<F>(f));
		}
	} // namespace ranges

	template<std::input_iterator It, std::sentinel_for<It> S, typename T>
	constexpr bool contains(It first, S last, const T &value)
	{
		return std::ranges::find(first, last, value) != last;
	}

	template<std::input_iterator It, std::sentinel_for<It> S, typename Pred>
	constexpr bool contains_if(It first, S last, Pred &&pred)
	{
		return std::ranges::find_if(first, last, std::forward<Pred>(pred)) != last;
	}

	namespace ranges
	{
		template<std::ranges::input_range R, typename T>
		constexpr bool contains(R &&r, const T &value)
		{
			using elem_t = std::ranges::range_value_t<R>;
			if constexpr (std::convertible_to<T, elem_t>)
				return std::ranges::find(r, static_cast<elem_t>(value)) != std::ranges::end(r);
			else
				return std::ranges::find(r, value) != std::ranges::end(r);
		}

		template<std::ranges::input_range R, typename Pred>
		constexpr bool contains_if(R &&r, Pred &&pred)
		{
			return std::ranges::find_if(r, std::forward<Pred>(pred)) != std::ranges::end(r);
		}
	} // namespace ranges

	template<std::input_iterator It, std::sentinel_for<It> S, typename Pred>
	constexpr std::size_t find_index_if(It first, S last, Pred &&pred)
	{
		std::size_t i = 0;
		for (; first != last; ++first, ++i)
			if (std::invoke(pred, *first))
				return i;
		return static_cast<std::size_t>(-1);
	}

	namespace ranges
	{
		template<std::ranges::input_range R, typename Pred>
		constexpr std::size_t find_index_if(R &&r, Pred &&pred)
		{
			return ::ach::find_index_if(std::ranges::begin(r), std::ranges::end(r), std::forward<Pred>(pred));
		}
	} // namespace ranges

	template<std::forward_iterator It, std::sentinel_for<It> S, typename Comp = std::less<>>
	constexpr std::size_t is_sorted_until_index(It first, S last, Comp comp = {})
	{
		if (first == last)
			return 0;

		std::size_t i = 1;
		auto next = first;
		for (++next; next != last; ++next, ++first, ++i)
			if (std::invoke(comp, *next, *first))
				return i;
		return i;
	}

	namespace ranges
	{
		template<std::ranges::forward_range R, typename Comp = std::less<>>
		constexpr std::size_t is_sorted_until_index(R &&r, Comp comp = {})
		{
			return ::ach::is_sorted_until_index(std::ranges::begin(r), std::ranges::end(r), std::move(comp));
		}
	} // namespace ranges

	template<typename To, typename From>
	[[nodiscard]] constexpr To safe_cast(const From &src) noexcept
	{
		if constexpr (std::is_trivially_copyable_v<To> && std::is_trivially_copyable_v<From> && sizeof(To) == sizeof(From))
			return std::bit_cast<To>(src);
		else if constexpr (std::is_convertible_v<From, To> && std::is_constructible_v<To, From>)
			return static_cast<To>(src);
		else
		{
			static_assert(std::is_trivially_copyable_v<To> && std::is_trivially_copyable_v<From> &&
														sizeof(To) == sizeof(From),
										"safe_cast: types are not bit-castable and not convertible");
		}
		std::unreachable();
	}

	template<std::random_access_iterator Iter, typename Compare = std::less<>>
		requires std::sortable<Iter, Compare>
	void merge(Iter first, Iter last, Compare comp = {})
	{
		const auto len = std::distance(first, last);
		if (len <= 1)
			return;
		using value_type = std::iter_value_t<Iter>;
		std::vector<value_type, commited_allocator<value_type>> buffer(len);
		value_type *ACH_RESTRICT buf_ptr = buffer.data();
		value_type *ACH_RESTRICT arr_ptr = std::to_address(first);
		for (std::size_t width = 1; width < static_cast<std::size_t>(len); width <<= 1)
		{
			const std::size_t width2 = width << 1;
			for (std::size_t i = 0; i < static_cast<std::size_t>(len); i += width2)
			{
				const std::size_t left_end = std::min(i + width, static_cast<std::size_t>(len));
				const std::size_t right_end = std::min(i + width2, static_cast<std::size_t>(len));
				std::size_t l = i, r = left_end, out = i;
				ACH_PREFETCH(&arr_ptr[l + 64], 0, 3);
				ACH_PREFETCH(&arr_ptr[r + 64], 0, 3);
				value_type lv = arr_ptr[l++];
				value_type rv = arr_ptr[r++];
				while (l + 1 < left_end && r + 1 < right_end)
				{
					value_type next_lv = arr_ptr[l];
					value_type next_rv = arr_ptr[r];
					if (comp(lv, rv))
					{
						buf_ptr[out++] = std::move(lv);
						lv = std::move(next_lv);
						l++;
					}
					else
					{
						buf_ptr[out++] = std::move(rv);
						rv = std::move(next_rv);
						r++;
					}
					next_lv = arr_ptr[l];
					next_rv = arr_ptr[r];
					if (comp(lv, rv))
					{
						buf_ptr[out++] = std::move(lv);
						lv = std::move(next_lv);
						l++;
					}
					else
					{
						buf_ptr[out++] = std::move(rv);
						rv = std::move(next_rv);
						r++;
					}
				}
				while (l < left_end && r < right_end)
				{
					if (comp(lv, rv))
					{
						buf_ptr[out++] = std::move(lv);
						lv = arr_ptr[l++];
					}
					else
					{
						buf_ptr[out++] = std::move(rv);
						rv = arr_ptr[r++];
					}
				}
				buf_ptr[out++] = comp(lv, rv) ? std::move(lv) : std::move(rv);
				while (l < left_end)
					buf_ptr[out++] = std::move(arr_ptr[l++]);
				while (r < right_end)
					buf_ptr[out++] = std::move(arr_ptr[r++]);
			}
			std::copy(buf_ptr, buf_ptr + len, arr_ptr);
		}
	}
} // namespace ach
