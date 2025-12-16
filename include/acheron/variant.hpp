/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <algorithm>
#include <new>
#include <tuple>
#include <variant>

/* warning: do not use anything in `ach::d`; they are implementation details and may
 * change without notice */
namespace ach::d
{
	/* note: helpers function for FUN overload resolution per standard
	 * https://eel.is/c++draft/variant.ctor#14 and https://eel.is/c++draft/variant.assign#11 */
	template<typename T>
	struct array_init_test
	{
		T s[1];
	};

	template<std::size_t /* I */, typename /* T */, typename /* U */, typename = void>
	struct build_fun
	{
		void fun() = delete; /* this means it's not a viable overload */
	};

	/* specialization for types where Ti x[] = { std::forward<T>(t) }; is well-formed */
	template<std::size_t I, typename T, typename U>
		requires requires { array_init_test<U>{ { std::declval<T>() } }; }
	struct build_fun<I, T, U>
	{
		// ReSharper disable once CppFunctionIsNotImplemented
		static std::integral_constant<std::size_t, I> fun(U);
	};

	template<typename T, typename... E>
	struct build_funs2;

	/* then inherit from all `build_fun` specializations to bring all
	 * FUN overload into scope */
	template<typename T, typename... E, std::size_t... Is>
	struct build_funs2<T, std::index_sequence<Is...>, E...> : build_fun<Is, T, E>...
	{
		using build_fun<Is, T, E>::fun...;
	};

	template<typename T, typename... E>
	using build_funs = build_funs2<T, std::make_index_sequence<sizeof...(E)>, E...>;

	template<typename T, typename... E>
	constexpr std::size_t resolve_alternative_index() noexcept
	{
		/* dev-note: with some template metaprogramming, it is possible to produce
		 * way better error messages compared to 20KB wall of text by the c++ compiler */
		if constexpr (requires { build_funs<T, E...>::fun(std::declval<T>()); })
		{
			using result_type = decltype(build_funs<T, E...>::fun(std::declval<T>()));
			return result_type::value;
		}
		else
		{
			return sizeof...(E); /* invalid index */
		}
	}
} // namespace ach::d

namespace ach
{
	/** @brief Invalid variant index constant */
	inline constexpr auto variant_npos = static_cast<std::size_t>(-1);

	/**
	 * @brief Empty type for variants that need a default-constructible first alternative
	 *
	 * monostate can serve as the first alternative type for a variant to make the variant
	 * type default constructible when the actual alternatives are not default constructible.
	 */
	struct monostate
	{
		constexpr monostate() noexcept = default;
		constexpr monostate(const monostate &) noexcept = default;
		constexpr monostate(monostate &&) noexcept = default;
		constexpr monostate &operator=(const monostate &) noexcept = default;
		constexpr monostate &operator=(monostate &&) noexcept = default;
		constexpr ~monostate() = default;
	};

	/** @brief monostate equality comparison - always true */
	constexpr bool operator==(monostate, monostate) noexcept
	{
		return true;
	}

	/** @brief monostate inequality comparison - always false */
	constexpr bool operator!=(monostate, monostate) noexcept
	{
		return false;
	}

	/** @brief monostate three-way comparison - always equivalent */
	constexpr std::strong_ordering operator<=>(monostate, monostate) noexcept
	{
		return std::strong_ordering::equivalent;
	}

	/**
	 * @brief Type-safe union with switch case support
	 */
	template<typename... Ts>
	class variant
	{
		/* note: fold expressions like ((C ? foo() : bar()), ...)
		 *	execute foo() only when C is true for the matching type; */
		static_assert(sizeof...(Ts) > 0, "variant must have at least one alternative");
		static_assert((std::is_destructible_v<Ts> && ...), "all alternatives must be destructible");

		/** @brief Maximum size among all alternatives */
		static constexpr std::size_t MAX_SIZE = std::max({ sizeof(Ts)... });
		/** @brief Maximum alignment among all alternatives */
		static constexpr std::size_t MAX_ALIGN = std::max({ alignof(Ts)... });

		/** @brief Storage for the variant data */
		alignas(MAX_ALIGN) char storage[MAX_SIZE];

		/** @brief Index of currently active alternative */
		std::size_t i = 0;

		/** @brief Conditional noexcept specification for move operations */
		static constexpr bool nothrow_move_constructible = (std::is_nothrow_move_constructible_v<Ts> && ...);
		/** @brief Conditional noexcept specification for copy operations */
		static constexpr bool nothrow_copy_constructible = (std::is_nothrow_copy_constructible_v<Ts> && ...);

		template<typename T>
		static constexpr std::size_t type_index() noexcept
		{
			std::size_t idx = 0;
			std::size_t result = sizeof...(Ts);
			((std::is_same_v<T, Ts> ? (result = idx, 0) : ++idx), ...);
			return result;
		}

		template<typename T>
		T *ptr() noexcept
		{
			return std::launder(reinterpret_cast<T *>(storage));
		}

		template<typename T>
		const T *ptr() const noexcept
		{
			return std::launder(reinterpret_cast<const T *>(storage));
		}

		template<typename T>
		void construct(auto &&...args)
		{
			std::construct_at(reinterpret_cast<T *>(storage), std::forward<decltype(args)>(args)...);
		}

		void destroy() noexcept
		{
			if (i != variant_npos)
			{
				std::size_t idx = 0;
				((++idx - 1 == i ? destroy_impl<Ts>() : void()), ...);
			}
		}

		template<typename T>
		void destroy_impl() noexcept
		{
			ptr<T>()->~T();
		}

		template<typename T>
		void copy_construct(const variant &other)
		{
			construct<T>(*other.ptr<T>());
		}

		template<typename T>
		void move_construct(variant &&other)
		{
			construct<T>(std::move(*other.ptr<T>()));
		}

		template<typename T>
		void move_construct_no_invalidate(const variant &other)
		{
			construct<T>(std::move(*other.ptr<T>()));
		}

		void move_from_no_invalidate(variant &other)
		{
			if (other.i != variant_npos)
			{
				std::size_t idx = 0;
				((++idx - 1 == other.i ? move_construct_no_invalidate<Ts>(other) : void()), ...);
				i = other.i;
				/* deliberately don't invalidate `other` for swap purposes */
			}
		}

		template<typename T>
		static constexpr std::size_t resolve_alternative_index() noexcept
		{
			/* dev-note: with some template metaprogramming, it is possible to produce
			 * way better error messages compared to 20KB wall of text by the c++ compiler */
			return d::resolve_alternative_index<T, Ts...>();
		}

		template<typename Ti>
		void emplace_move_alternative(variant &&other)
		{
			constexpr std::size_t alt_index = type_index<Ti>();
			destroy();
			construct<Ti>(std::move(*other.ptr<Ti>()));
			i = alt_index;

			/* invalidate `other */
			other.destroy();
			other.i = variant_npos;
		}

		template<typename Ti>
		void emplace_copy_alternative(const variant &other)
		{
			constexpr std::size_t alt_index = type_index<Ti>();
			if constexpr (std::is_nothrow_copy_constructible_v<Ti> || !std::is_nothrow_move_constructible_v<Ti>)
			{
				/* use emplace directly for performance */
				destroy();
				construct<Ti>(*other.ptr<Ti>());
				i = alt_index;
			}
			else
			{
				/* use copy-and-swap for strong exception safety */
				variant temp(other);
				swap(temp);
			}
		}

		template<typename T>
		static constexpr bool is_in_place_type_v = false;

		template<typename T>
		static constexpr bool is_in_place_type_v<std::in_place_type_t<T>> = true;

		template<typename T>
		static constexpr bool is_in_place_index_v = false;

		template<std::size_t I>
		static constexpr bool is_in_place_index_v<std::in_place_index_t<I>> = true;

	public:
		/** @brief Compile-time type index constants for switch statements */
		template<typename T>
		static constexpr std::size_t of = type_index<T>();

		/** @brief Default constructor; constructs first alternative */
		constexpr variant() noexcept(std::is_nothrow_default_constructible_v<std::tuple_element_t<0, std::tuple<Ts...>>>)
			requires(std::is_default_constructible_v<std::tuple_element_t<0, std::tuple<Ts...>>>)
				: i(variant_npos) /* start invalid for exception safety */
		{
			/* if default construction of first type throws, `*this` will remain in valid
			 * empty state rather than having a valid index with uninitialized storage */
			using first_t = std::tuple_element_t<0, std::tuple<Ts...>>;
			construct<first_t>();
			/* only set valid index after successful construction */
			i = 0;
		}

		/** @brief Converting constructor with perfect forwarding */
		template<typename T>
			requires(!std::is_same_v<std::decay_t<T>, variant> && !is_in_place_type_v<std::decay_t<T>> &&
							 !is_in_place_index_v<std::decay_t<T>> && (resolve_alternative_index<T>() < sizeof...(Ts)))
		constexpr variant(T &&t) noexcept(std::is_nothrow_constructible_v<
																			std::tuple_element_t<resolve_alternative_index<T>(), std::tuple<Ts...>>, T>) :
				i(variant_npos) /* start invalid for exception safety */
		{
			/* if construction throws, `*this` will remain in valid empty state
			 * rather than having a valid index with uninitialized storage */
			constexpr std::size_t selected_index = resolve_alternative_index<T>();
			using selected_type = std::tuple_element_t<selected_index, std::tuple<Ts...>>;
			construct<selected_type>(std::forward<T>(t));
			/* only set valid index after successful construction */
			i = selected_index;
		}

		/** @brief In-place constructor by type */
		template<typename T, typename... Args>
			requires((std::is_same_v<T, Ts> || ...) && std::is_constructible_v<T, Args...>)
		constexpr explicit variant(std::in_place_type_t<T>,
															 Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) :
				i(variant_npos) /* start invalid for exception safety */
		{
			/* if in-place construction throws, `*this` will remain in valid empty state
			 * rather than having a valid index with uninitialized storage */
			construct<T>(std::forward<Args>(args)...);
			/* only set valid index after successful construction */
			i = type_index<T>();
		}

		/** @brief In-place constructor by type with initializer_list */
		template<typename T, typename U, typename... Args>
			requires((std::is_same_v<T, Ts> || ...) && std::is_constructible_v<T, std::initializer_list<U> &, Args...>)
		constexpr explicit variant(std::in_place_type_t<T>, std::initializer_list<U> il, Args &&...args) noexcept(
						std::is_nothrow_constructible_v<T, std::initializer_list<U> &, Args...>) :
				i(variant_npos) /* start invalid for exception safety */
		{
			/* if in-place construction throws, `*this` will remain in valid empty state
			 * rather than having a valid index with uninitialized storage */
			construct<T>(il, std::forward<Args>(args)...);
			/* only set valid index after successful construction */
			i = type_index<T>();
		}

		/** @brief In-place constructor by index */
		template<std::size_t I, typename... Args>
			requires(I < sizeof...(Ts))
		constexpr explicit variant(std::in_place_index_t<I>, Args &&...args) noexcept(
						std::is_nothrow_constructible_v<std::tuple_element_t<I, std::tuple<Ts...>>, Args...>) :
				i(variant_npos) /* start invalid for exception safety */
		{
			/* if in-place construction throws, `*this` will remain in valid empty state
			 * rather than having a valid index with uninitialized storage */
			using type = std::tuple_element_t<I, std::tuple<Ts...>>;
			static_assert(std::is_constructible_v<type, Args...>, "Type must be constructible from provided arguments");
			construct<type>(std::forward<Args>(args)...);
			/* only set valid index after successful construction */
			i = I;
		}

		/** @brief In-place constructor by index with initializer_list */
		template<std::size_t I, typename U, typename... Args>
			requires(I < sizeof...(Ts))
		constexpr explicit variant(std::in_place_index_t<I>, std::initializer_list<U> il, Args &&...args) noexcept(
						std::is_nothrow_constructible_v<std::tuple_element_t<I, std::tuple<Ts...>>, std::initializer_list<U> &

																						,
																						Args...>) : i(variant_npos) /* start invalid for exception safety */
		{
			/* if in-place construction throws, `*this` will remain in valid empty state
			 * rather than having a valid index with uninitialized storage */
			using type = std::tuple_element_t<I, std::tuple<Ts...>>;
			static_assert(std::is_constructible_v<type, std::initializer_list<U> &, Args...>,
										"Type must be constructible from initializer_list and provided arguments");
			construct<type>(il, std::forward<Args>(args)...);
			/* only set valid index after successful construction */
			i = I;
		}

		/** @brief Copy constructor */
		constexpr variant(const variant &other) noexcept(nothrow_copy_constructible) :
				i(variant_npos) /* start invalid for exception safety */
		{
			/* if the copy construction throws, `*this` would remain in a valid empty
			 * state rather than having a valid index with uninitialized storage */
			if (other.i != variant_npos)
			{
				std::size_t idx = 0;
				((++idx - 1 == other.i ? copy_construct<Ts>(other) : void()), ...);
				/* a valid index shall be set only after successful construction */
				i = other.i;
			}
			/* note: `other` remains unchanged regardless of success or failure */
		}

		/** @brief Move constructor */
		constexpr variant(variant &&other) noexcept(nothrow_move_constructible) :
				i(variant_npos) /* start invalid for exception safety */
		{
			const auto other_index = other.i;
			if (other_index != variant_npos)
			{
				if constexpr (nothrow_move_constructible)
				{
					std::size_t idx = 0;
					((++idx - 1 == other_index ? move_construct<Ts>(std::move(other)) : void()), ...);
					i = other_index;
					other.destroy();
					other.i = variant_npos;
				}
				else
				{
					/* `std::move` in `move_construct` may throw early so we invalidate the
					 * source first */
					other.i = variant_npos;
					std::size_t idx = 0;
					((++idx - 1 == other_index ? move_construct<Ts>(std::move(other)) : void()), ...);
					i = other_index;
					other.destroy();
				}
			}
		}

		/** @brief Copy assignment operator */
		constexpr variant &operator=(const variant &other)
		{
			/* https://eel.is/c++draft/variant.assign#1 */
			static_assert(((std::is_copy_constructible_v<Ts> && std::is_copy_assignable_v<Ts>) && ...),
										"all alternatives must be copy constructible and copy assignable");

			if (this == &other)
				return *this;

			const std::size_t j = other.index();

			/* (2.1) if neither `*this` nor `other` holds a value, there is no effect */
			if (i == variant_npos && j == variant_npos)
				return *this;

			/* (2.2) if `*this` holds a value but `other` does not, destroy and set to not hold value */
			if (i != variant_npos && j == variant_npos)
			{
				destroy();
				i = variant_npos;
				return *this;
			}

			/* (2.3) if index() == j, assign the value contained in `other` to `*this` */
			if (i == j && i != variant_npos)
			{
				std::size_t idx = 0;
				((++idx - 1 == i ? (void) (*ptr<Ts>() = *other.ptr<Ts>()) : void()), ...);
				return *this;
			}

			std::size_t idx = 0;
			((++idx - 1 == j ? emplace_copy_alternative<Ts>(other) : void()), ...);
			return *this;
		}

		/**
		 * @brief Move assignment operator
		 *
		 * If both variants hold on the same active alternative, the contained value is move-assigned,
		 * and the source remains valid but in a moved-from state
		 */
		constexpr variant &operator=(variant &&rhs) noexcept(
						((std::is_nothrow_move_constructible_v<Ts> && std::is_nothrow_move_assignable_v<Ts>) && ...))
		{
			/* https://eel.is/c++draft/variant.assign#6 */
			static_assert(((std::is_move_constructible_v<Ts> && std::is_move_assignable_v<Ts>) && ...),
										"all alternatives must be move constructible and move assignable");

			if (this == &rhs)
				return *this;

			const std::size_t j = rhs.index();

			/* (8.1) if neither *this nor rhs holds a value, there is no effect */
			if (i == variant_npos && j == variant_npos)
				return *this;

			/* (8.2) if *this holds a value but rhs does not, destroy and set to not hold value */
			if (i != variant_npos && j == variant_npos)
			{
				destroy();
				i = variant_npos;
				return *this;
			}

			/* (8.3) if index() == j, assign GET<j>(std::move(rhs)) to the contained value */
			if (i == j && i != variant_npos)
			{
				std::size_t idx = 0;
				((++idx - 1 == i ? (void) (*ptr<Ts>() = std::move(*rhs.ptr<Ts>())) : void()), ...);
				return *this;
			}

			/* (8.4) otherwise, equivalent to emplace<j>(GET<j>(std::move(rhs))) */
			std::size_t idx = 0;
			((++idx - 1 == j ? emplace_move_alternative<Ts>(std::move(rhs)) : void()), ...);
			return *this;
		}

		/** @brief Converting assignment operator */
		template<typename T>
			requires(!std::is_same_v<std::decay_t<T>, variant> && (resolve_alternative_index<T>() < sizeof...(Ts)))
		constexpr variant &operator=(T &&t)
		{
			/* https://eel.is/c++draft/variant.assign#11 */
			constexpr std::size_t j = resolve_alternative_index<T>();
			using Tj = std::tuple_element_t<j, std::tuple<Ts...>>;

			static_assert(std::is_assignable_v<Tj &, T> && std::is_constructible_v<Tj, T>,
										"selected alternative must be assignable and constructible from T");

			/* (13.1) if *this holds a Tj, assigns std::forward<T>(t) to the contained value */
			if (i == j && i != variant_npos)
			{
				*ptr<Tj>() = std::forward<T>(t);
				return *this;
			}

			/* (13.2) if is_nothrow_constructible_v<Tj, T> || !is_nothrow_move_constructible_v<Tj> */
			if constexpr (std::is_nothrow_constructible_v<Tj, T> || !std::is_nothrow_move_constructible_v<Tj>)
			{
				destroy();
				construct<Tj>(std::forward<T>(t));
				i = j;
			}
			/* (13.3) otherwise, equivalent to emplace<j>(Tj(std::forward<T>(t))) */
			else
			{
				destroy();
				construct<Tj>(Tj(std::forward<T>(t)));
				i = j;
			}
			return *this;
		}

		/* note: noexcept specification per standard https://eel.is/c++draft/variant.assign#16:
		 * is_nothrow_assignable_v<Tj&, T> && is_nothrow_constructible_v<Tj, T> */

		/** @brief Destructor */
		constexpr ~variant()
		{
			destroy();
		}

		/** @brief Get the index of the currently active alternative */
		[[nodiscard]] constexpr std::size_t index() const noexcept
		{
			return i;
		}

		/** @brief Check if the variant is in an invalid state */
		[[nodiscard]] constexpr bool valueless_by_exception() const noexcept
		{
			return i == variant_npos;
		}

		/** @brief Construct a value of type T in place */
		template<typename T, typename... Args>
			requires((std::is_same_v<T, Ts> || ...) && std::is_constructible_v<T, Args...>)
		constexpr T &emplace(Args &&...args)
		{
			destroy();
			i = variant_npos; /* set to valueless before attempting construction for exception safety */
			construct<T>(std::forward<Args>(args)...);
			i = type_index<T>();
			return *ptr<T>();
		}

		/** @brief Construct a value of the I-th alternative in place */
		template<std::size_t I, typename... Args>
			requires(I < sizeof...(Ts))
		constexpr auto &emplace(Args &&...args)
		{
			using type = std::tuple_element_t<I, std::tuple<Ts...>>;
			return emplace<type>(std::forward<Args>(args)...);
		}

		/** @brief Construct a value of type T in place with initializer_list */
		template<typename T, typename U, typename... Args>
			requires((std::is_same_v<T, Ts> || ...) && std::is_constructible_v<T, std::initializer_list<U> &, Args...>)
		T &emplace(std::initializer_list<U> il, Args &&...args)
		{
			destroy();
			construct<T>(il, std::forward<Args>(args)...);
			i = type_index<T>();
			return *ptr<T>();
		}

		/** @brief Construct a value of the I-th alternative in place with initializer_list */
		template<std::size_t I, typename U, typename... Args>
			requires(I < sizeof...(Ts))
		auto &emplace(std::initializer_list<U> il, Args &&...args)
		{
			using type = std::tuple_element_t<I, std::tuple<Ts...>>;
			return emplace<type>(il, std::forward<Args>(args)...);
		}

		/** @brief Swap contents with another variant */
		constexpr void swap(variant &other) noexcept(
						((std::is_nothrow_move_constructible_v<Ts> && std::is_nothrow_swappable_v<Ts>) && ...))
		{
			static_assert((std::is_swappable_v<Ts> && ...), "all alternatives must be swappable");
			if (this == &other)
				return;

			if (i == variant_npos && other.i == variant_npos)
				return; /* both empty, nothing to do */

			/* `*this` is empty, move `other` into `*this` and clear `other` */
			if (i == variant_npos)
			{
				move_from_no_invalidate(other);
				other.destroy();
				other.i = variant_npos;
				return;
			}

			if (other.i == variant_npos)
			{
				/* `other` is empty, move `*this` into `other` and clear `*this` */
				other.move_from_no_invalidate(*this);
				destroy();
				i = variant_npos;
				return;
			}

			/* mandated by https://eel.is/c++draft/variant.swap#3.2 */
			if (i == other.i)
			{
				std::size_t idx = 0;
				((++idx - 1 == i ? std::swap(*ptr<Ts>(), *other.ptr<Ts>()) : void()), ...);
				return;
			}

			/* note: raw storage swap if safe when all alternatives are trivially relocatable.
			 * the type trait checks exclude most dangerous cases so creating trivially
			 * movable types with internal pointers should ensure proper relocatable semantics */
			if constexpr ((std::is_trivially_move_constructible_v<Ts> && ...) &&
										(std::is_trivially_move_assignable_v<Ts> && ...) && (std::is_trivially_destructible_v<Ts> && ...) &&
										(std::is_nothrow_move_constructible_v<Ts> && ...))
			{
				std::swap_ranges(storage, storage + MAX_SIZE, other.storage);
				std::swap(i, other.i);
				return;
			}

			/* standard three-way move for non-trivially-relocatable types;
			 * note: this is the slowest by far */
			variant temp;
			temp.move_from_no_invalidate(*this);

			destroy();
			i = variant_npos;
			move_from_no_invalidate(other);

			other.destroy();
			other.i = variant_npos;
			other.move_from_no_invalidate(temp);
		}

		/** @brief Check if the variant currently holds a value of type T */
		template<typename T>
		[[nodiscard]] constexpr bool holds_alternative() const noexcept
		{
			return i == type_index<T>();
		}

		/** @brief Get a reference to the contained value of type T */
		template<typename T>
		T &get()
		{
			if (!holds_alternative<T>())
				throw std::bad_variant_access();
			return *ptr<T>();
		}

		/** @brief Get a const reference to the contained value of type T */
		template<typename T>
		const T &get() const
		{
			if (!holds_alternative<T>())
				throw std::bad_variant_access();
			return *ptr<T>();
		}

		/** @brief Get a pointer to the contained value of type T, or nullptr */
		template<typename T>
		T *get_if() noexcept
		{
			return holds_alternative<T>() ? ptr<T>() : nullptr;
		}

		/** @brief Get a const pointer to the contained value of type T, or nullptr */
		template<typename T>
		const T *get_if() const noexcept
		{
			return holds_alternative<T>() ? ptr<T>() : nullptr;
		}

		/** @brief Member visit for single variant */
		template<class Self, class Visitor>
		constexpr decltype(auto) visit(this Self &&self, Visitor &&vis);

		/** @brief Member visit for single variant with explicit return type */
		template<class R, class Self, class Visitor>
		constexpr R visit(this Self &&self, Visitor &&vis);
	};

	/** @brief Check if variant holds alternative of type T */
	template<typename T, typename... Ts>
	constexpr bool holds_alternative(const variant<Ts...> &v) noexcept
	{
		return v.template holds_alternative<T>();
	}

	/** @brief Get reference to contained value of type T */
	template<typename T, typename... Ts>
	constexpr T &get(variant<Ts...> &v)
	{
		return v.template get<T>();
	}

	/** @brief Get const reference to contained value of type T */
	template<typename T, typename... Ts>
	constexpr const T &get(const variant<Ts...> &v)
	{
		return v.template get<T>();
	}

	/** @brief Get rvalue reference to contained value of type T */
	template<typename T, typename... Ts>
	constexpr T &&get(variant<Ts...> &&v)
	{
		return std::move(v.template get<T>());
	}

	/** @brief Get const rvalue reference to contained value of type T */
	template<typename T, typename... Ts>
	constexpr const T &&get(const variant<Ts...> &&v)
	{
		return std::move(v.template get<T>());
	}

	/** @brief Get reference to I-th alternative */
	template<std::size_t I, typename... Ts>
	constexpr auto &get(variant<Ts...> &v)
	{
		using type = std::tuple_element_t<I, std::tuple<Ts...>>;
		return get<type>(v);
	}

	/** @brief Get const reference to I-th alternative */
	template<std::size_t I, typename... Ts>
	constexpr const auto &get(const variant<Ts...> &v)
	{
		using type = std::tuple_element_t<I, std::tuple<Ts...>>;
		return get<type>(v);
	}

	/** @brief Get rvalue reference to I-th alternative */
	template<std::size_t I, typename... Ts>
	constexpr auto &&get(variant<Ts...> &&v)
	{
		using type = std::tuple_element_t<I, std::tuple<Ts...>>;
		return get<type>(std::move(v));
	}

	/** @brief Get const rvalue reference to I-th alternative */
	template<std::size_t I, typename... Ts>
	constexpr const auto &&get(const variant<Ts...> &&v)
	{
		using type = std::tuple_element_t<I, std::tuple<Ts...>>;
		return get<type>(std::move(v));
	}

	/** @brief Get pointer to contained value of type T, or nullptr */
	template<typename T, typename... Ts>
	constexpr T *get_if(variant<Ts...> *v) noexcept
	{
		return v ? v->template get_if<T>() : nullptr;
	}

	/** @brief Get const pointer to contained value of type T, or nullptr */
	template<typename T, typename... Ts>
	constexpr const T *get_if(const variant<Ts...> *v) noexcept
	{
		return v ? v->template get_if<T>() : nullptr;
	}

	/** @brief Get pointer to I-th alternative, or nullptr */
	template<std::size_t I, typename... Ts>
	constexpr auto *get_if(variant<Ts...> *v) noexcept
	{
		using type = std::tuple_element_t<I, std::tuple<Ts...>>;
		return get_if<type>(v);
	}

	/** @brief Get const pointer to I-th alternative, or nullptr */
	template<std::size_t I, typename... Ts>
	constexpr const auto *get_if(const variant<Ts...> *v) noexcept
	{
		using type = std::tuple_element_t<I, std::tuple<Ts...>>;
		return get_if<type>(v);
	}

	/** @brief Swap two variants */
	template<typename... Ts>
	void swap(variant<Ts...> &lhs, variant<Ts...> &rhs) noexcept(noexcept(lhs.swap(rhs)))
	{
		lhs.swap(rhs);
	}

	/** @brief Equality comparison */
	template<typename... Ts>
	constexpr bool operator==(const variant<Ts...> &lhs, const variant<Ts...> &rhs)
	{
		if (lhs.index() != rhs.index())
			return false;
		if (lhs.valueless_by_exception())
			return true;

		std::size_t idx = 0;
		return ((++idx - 1 == lhs.index() ? (lhs.template get<Ts>() == rhs.template get<Ts>()) : false) || ...);
	}

	/** @brief Inequality comparison */
	template<typename... Ts>
	constexpr bool operator!=(const variant<Ts...> &lhs, const variant<Ts...> &rhs)
	{
		return !(lhs == rhs);
	}

	/** @brief Three-way comparison */
	template<typename... Ts>
	constexpr auto operator<=>(const variant<Ts...> &lhs, const variant<Ts...> &rhs)
	{
		using comparison_t = std::common_comparison_category_t<std::compare_three_way_result_t<Ts>...>;

		if (lhs.valueless_by_exception() && rhs.valueless_by_exception())
			return comparison_t::equivalent;
		if (lhs.valueless_by_exception())
			return comparison_t::less;
		if (rhs.valueless_by_exception())
			return comparison_t::greater;
		if (auto cmp = lhs.index() <=> rhs.index(); cmp != 0)
			return static_cast<comparison_t>(cmp);

		std::size_t idx = 0;
		comparison_t result = comparison_t::equivalent;

		/* we use a fold expression to compare each alternative
		 * in the order they are defined; the matching index
		 * will perform the actual comparison operation;
		 * wrap the result in `void()` to avoid warnings */
		((void(++idx - 1 == lhs.index() ? void(result = lhs.template get<Ts>() <=> rhs.template get<Ts>()) : void())), ...);
		return result;
	}

	/** @brief Get the number of alternatives in a variant */
	template<typename T>
	struct variant_size;

	template<typename... Ts>
	struct variant_size<variant<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)>
	{};

	/** @brief Convenience variable template for variant_size */
	template<typename T>
	inline constexpr std::size_t variant_size_v = variant_size<T>::value;

	/** @brief Get the type of the I-th alternative */
	template<std::size_t I, typename T>
	struct variant_alternative;

	template<std::size_t I, typename... Ts>
	struct variant_alternative<I, variant<Ts...>>
	{
		using type = std::tuple_element_t<I, std::tuple<Ts...>>;
	};

	template<std::size_t I, typename... Ts>
	struct variant_alternative<I, const variant<Ts...>>
	{
		using type = std::add_const_t<typename variant_alternative<I, variant<Ts...>>::type>;
	};

	template<std::size_t I, typename... Ts>
	struct variant_alternative<I, volatile variant<Ts...>>
	{
		using type = std::add_volatile_t<typename variant_alternative<I, variant<Ts...>>::type>;
	};

	template<std::size_t I, typename... Ts>
	struct variant_alternative<I, const volatile variant<Ts...>>
	{
		using type = std::add_cv_t<typename variant_alternative<I, variant<Ts...>>::type>;
	};

	/** @brief Convenience alias template for variant_alternative */
	template<std::size_t I, typename T>
	using variant_alternative_t = variant_alternative<I, T>::type;

	/** @brief Visit a variant with a callable; for `std::visit` compatibility */
	template<typename Visitor, typename Variant>
	constexpr decltype(auto) visit(Visitor &&vis, Variant &&var)
	{
		if (var.valueless_by_exception())
			throw std::bad_variant_access();

		using variant_t = std::decay_t<Variant>;
		constexpr std::size_t variant_size = variant_size_v<variant_t>;

		/* this is a silly little thing we shouldn't do in modern C++
		 * but it's okay as we gain around 7x performance improvement
		 * as of https://github.com/alpluspluss/libach/blob/e61201e664673d3c68a477690d3904ae1b526665/include/ach#L512 */
		return visit_recursive<0, variant_size>(std::forward<Visitor>(vis), std::forward<Variant>(var));
	}

	/**
	 * @brief Recursive template implementation for visit
	 * @note This is meant to be called only from `ach::visit`
	 */
	template<std::size_t I, std::size_t N, typename Visitor, typename Variant>
	constexpr decltype(auto) visit_recursive(Visitor &&vis, Variant &&var)
	{
		if (var.index() == I)
			return std::forward<Visitor>(vis)(ach::get<I>(std::forward<Variant>(var)));

		if constexpr (I + 1 < N)
		{
			return visit_recursive<I + 1, N>(std::forward<Visitor>(vis), std::forward<Variant>(var));
		}
		else
		{
			/* note: this path should never be reached due to the check above */
#ifdef __GNUC__
			__builtin_unreachable();
#elif defined(_MSC_VER)
			__assume(false);
#endif
		}
	}

	/** @brief Overload helper for `visit` */
	template<typename... Ts>
	struct overload : Ts...
	{
		using Ts::operator()...;
	};

	/** @brief Deduction guide for `overload` */
	template<typename... Ts>
	overload(Ts...) -> overload<Ts...>;


	/** @brief Pattern matching builder for clean syntax */
	template<typename Variant>
	class match_builder
	{
	public:
		constexpr match_builder(Variant &&v) : var(std::forward<Variant>(v))
		{}

		template<typename Lambda>
		constexpr auto operator|(Lambda &&lambda)
		{
			return lambda(std::forward<Variant>(var));
		}

	private:
		Variant &&var;
	};

	/** @brief Deduction guide for `match_builder` */
	template<typename Variant>
	match_builder(Variant &&) -> match_builder<Variant>;

	/** @brief Create a pattern match builder */
	template<typename Variant>
	constexpr auto match(Variant &&v)
	{
		return match_builder{ std::forward<Variant>(v) };
	}
} // namespace ach

/* standard library integration;
 * required support on:
 * - `std::hash`: https://eel.is/c++draft/variant.hash
 * - `std::variant_size` and cv-qualified: https://eel.is/c++draft/variant.helper#1
 * - `std::variant_alternative` and cv-qualified: https://eel.is/c++draft/variant.helper#3
 */
namespace std
{
	template<typename... Ts>
	struct variant_size<ach::variant<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)>
	{};

	template<std::size_t I, typename... Ts>
	struct variant_alternative<I, ach::variant<Ts...>>
	{
		using type = std::tuple_element_t<I, std::tuple<Ts...>>;
	};

	template<typename... Ts>
	struct variant_size<const ach::variant<Ts...>> : variant_size<ach::variant<Ts...>>
	{};

	template<typename... Ts>
	struct variant_size<volatile ach::variant<Ts...>> : variant_size<ach::variant<Ts...>>
	{};

	template<typename... Ts>
	struct variant_size<const volatile ach::variant<Ts...>> : variant_size<ach::variant<Ts...>>
	{};

	template<std::size_t I, typename... Ts>
	struct variant_alternative<I, const ach::variant<Ts...>>
	{
		using type = std::add_const_t<typename variant_alternative<I, ach::variant<Ts...>>::type>;
	};

	template<std::size_t I, typename... Ts>
	struct variant_alternative<I, volatile ach::variant<Ts...>>
	{
		using type = std::add_volatile_t<typename variant_alternative<I, ach::variant<Ts...>>::type>;
	};

	template<std::size_t I, typename... Ts>
	struct variant_alternative<I, const volatile ach::variant<Ts...>>
	{
		using type = std::add_cv_t<typename variant_alternative<I, ach::variant<Ts...>>::type>;
	};

	template<typename... Ts>
	struct hash<ach::variant<Ts...>>
	{
		std::size_t operator()(const ach::variant<Ts...> &v) const
		{
			if (v.valueless_by_exception())
				return std::hash<std::size_t>{}(v.index());

			std::size_t result = 0;
			ach::visit(
							[&result, &v]<typename U>(const U &value)
							{
								using T = std::decay_t<U>;
								result = std::hash<std::size_t>{}(v.index()) ^ (std::hash<T>{}(value) << 1);
							},
							v);
			return result;
		}
	};

	template<>
	struct hash<ach::monostate>
	{
		constexpr std::size_t operator()(ach::monostate) const noexcept
		{
			/* note: all monostate objects are equivalent and valueless, thus
			 * a fixed arbitrary value can be used */
			return 0x3C18A8B7; /* some random arbitrary value i got from wheel of fortune */
		}
	};
} // namespace std

/* implementation of all fwd-declared entities of `ach` namespace */
namespace ach
{
	/** @brief Member visit for single variant */
	template<typename... Ts>
	template<class Self, class Visitor>
	constexpr decltype(auto) variant<Ts...>::visit(this Self &&self, Visitor &&vis)
	{
		return ach::visit(std::forward<Visitor>(vis), std::forward<Self>(self));
	}

	/** @brief Member visit for single variant with explicit return type */
	template<typename... Ts>
	template<class R, class Self, class Visitor>
	constexpr R variant<Ts...>::visit(this Self &&self, Visitor &&vis)
	{
		return ach::visit<R>(std::forward<Visitor>(vis), std::forward<Self>(self));
	}
} // namespace ach
