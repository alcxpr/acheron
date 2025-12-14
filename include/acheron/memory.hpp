/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <memory>
#include <type_traits>
#include "allocator.hpp"
#include "diagnostic.hpp"

namespace ach
{
	namespace d
	{
		template<typename Alloc>
		concept stateless_allocator = std::allocator_traits<Alloc>::is_always_equal::value && std::is_empty_v<Alloc>;

		template<typename T, typename Alloc>
			requires stateless_allocator<Alloc>
		struct stateless_deleter
		{
			void operator()(T *ptr) const noexcept
			{
				using traits = std::allocator_traits<Alloc>;
				using rebound_allocator = typename traits::template rebind_alloc<T>;
				rebound_allocator alloc{};
				traits::destroy(alloc, ptr);
				traits::deallocate(alloc, ptr, 1);
			}
		};

		template<typename T, typename Alloc>
		struct stateful_deleter
		{
			Alloc alloc;
			void operator()(T *ptr) const noexcept
			{
				using traits = std::allocator_traits<Alloc>;
				using rebound_allocator = typename traits::template rebind_alloc<T>;
				rebound_allocator rebound(alloc);
				traits::destroy(rebound, ptr);
				traits::deallocate(rebound, ptr, 1);
			}
		};
	} // namespace d

	template<typename T, typename Alloc, typename... Args>
	auto allocate_unique(Alloc alloc, Args &&...args)
	{
		using traits = std::allocator_traits<Alloc>;
		using rebound_allocator = typename traits::template rebind_alloc<T>;
		rebound_allocator rebound(alloc);
		T *ptr = traits::allocate(rebound, 1);

		try
		{
			traits::construct(rebound, ptr, std::forward<Args>(args)...);
		}
		catch (...)
		{
			traits::deallocate(rebound, ptr, 1);
			throw fatal_error("construction failed");
		}
		if constexpr (d::stateless_allocator<rebound_allocator>)
		{
			return std::unique_ptr<T, d::stateless_deleter<T, rebound_allocator>>(ptr);
		}
		else
		{
			return std::unique_ptr<T, d::stateful_deleter<T, rebound_allocator>>(
							ptr, d::stateful_deleter<T, rebound_allocator>{ std::move(rebound) });
		}
	}

	template<typename T, typename Alloc, typename... Args>
	auto allocate_shared(Alloc alloc, Args &&...args)
	{
		try
		{
			return std::allocate_shared<T>(alloc, std::forward<Args>(args)...);
		}
		catch (...)
		{
			throw fatal_error("shared allocation failed");
		}
	}

	/* default overloads */
	template<typename T, typename... Args>
	auto allocate_unique(Args &&...args)
	{
		return allocate_unique<T>(ach::allocator<T>{}, std::forward<Args>(args)...);
	}

	template<typename T, typename... Args>
	auto allocate_shared(Args &&...args)
	{
		return allocate_shared<T>(ach::allocator<T>{}, std::forward<Args>(args)...);
	}
} // namespace ach
