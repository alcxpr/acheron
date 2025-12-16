/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <cstdint>

namespace ach
{
	template<std::size_t Align, typename T>
	constexpr T *assume_aligned(T *ptr)
	{
#if defined(__clang__) || defined(__GNUC__)
		return static_cast<T *>(__builtin_assume_aligned(ptr, Align));
#else
		return ptr;
#endif
	}
} // namespace ach
