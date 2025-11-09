/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <cstddef>
#include <cstring>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include "allocator.hpp"
#include "diagnostic.hpp"

namespace ach
{
    /**
     * @brief Customization point for value storage
     *
     * Users specialize this to define how types are stored in tvm::value.
     * The nested 'value' type defines the actual storage representation.
     *
     * @tparam T The logical type to store
     *
     * @par Example
     * @code
     * template<>
     * struct value_traits<std::uint32_t>
     * {
     *     struct value
     *     {
     *         std::uint32_t data;
     *     };
     * };
     * @endcode
     */
    template<typename T>
    struct value_traits;

    /**
     * @brief Type-erased value container with small buffer optimization
     *
     * Stores values through value_traits customization point. Small objects
     * use inline storage; larger objects are heap-allocated.
     *
     * @tparam Alloc Allocator type for heap allocations
     *
     * @par Example
     * @code
     * tvm::value val;
     * val.emplace<std::uint32_t>(42);
     * std::uint32_t x = val.get<std::uint32_t>();
     * @endcode
     */
    template<typename Allocator = ach::allocator<std::byte>>
    class value
    {
    public:
        using size_type = std::size_t;
        using allocator_type = Allocator;
        using allocator_traits = std::allocator_traits<allocator_type>;
        using byte_type = std::byte;
        using pointer = void*;
        using const_pointer = const void*;

        static constexpr size_type buffer_size = 32;
        static constexpr size_type buffer_align = alignof(std::max_align_t);

    private:
        struct vtable_t
        {
            using destroy_fn = void (*)(pointer);
            using copy_fn = void (*)(pointer dst, const_pointer src);
            using move_fn = void (*)(pointer dst, pointer src);
            using type_info_ptr = const std::type_info*;

            destroy_fn destroy;
            copy_fn copy;
            move_fn move;
            type_info_ptr type;
            size_type size;
            size_type alignment;
        };

        using vtable_ptr = const vtable_t*;
        using buffer_type = byte_type[buffer_size];

        template<typename T>
        static constexpr vtable_t vtable_for =
        {
            .destroy = [](pointer ptr) noexcept(std::is_nothrow_destructible_v<T>)
            {
                std::destroy_at(static_cast<T*>(ptr));
            },
            .copy = [](pointer dst, const_pointer src)
                noexcept(std::is_nothrow_copy_constructible_v<T>)
            {
                std::construct_at(static_cast<T*>(dst), *static_cast<const T*>(src));
            },
            .move = [](pointer dst, pointer src)
                noexcept(std::is_nothrow_move_constructible_v<T>)
            {
                std::construct_at(static_cast<T*>(dst), std::move(*static_cast<T*>(src)));
            },
            .type = &typeid(T),
            .size = sizeof(T),
            .alignment = alignof(T)
        };

        alignas(buffer_align) buffer_type buffer;
        pointer data;
        vtable_ptr vtable;
        [[no_unique_address]] allocator_type allocator;

        template<typename T>
        using decay_t = std::decay_t<T>;

        template<typename T>
        static constexpr bool fits_in_buffer =
            sizeof(T) <= buffer_size && alignof(T) <= buffer_align;

        [[nodiscard]] bool uses_buffer() const noexcept
        {
            /* Discriminate between inline buffer and heap storage by checking if the
             * data pointer falls within the buffer's address range. We use pointer
             * arithmetic and cast to integer for a branchless comparison. If data
             * points to heap memory, the subtraction wraps and produces a large value
             * that fails the bounds check. This compiles to subtract-and-compare. */
            auto offset = static_cast<size_type>(
                static_cast<const byte_type*>(data) - buffer
            );
            return offset < buffer_size;
        }

        void clear() noexcept
        {
            if (data && vtable)
            {
                /* Invoke destructor through vtable, then deallocate heap memory if
                 * the object wasn't stored inline. The vtable's destroy function is
                 * always noexcept regardless of the stored type's destructor, so this
                 * operation never throws. */
                vtable->destroy(data);
                if (!uses_buffer())
                {
                    auto byte_ptr = static_cast<byte_type*>(data);
                    allocator_traits::deallocate(allocator, byte_ptr, vtable->size);
                }
            }
            data = nullptr;
            vtable = nullptr;
        }

        template<typename T>
        pointer allocate_for_type()
        {
            constexpr size_type align = alignof(T);
            constexpr size_type size = sizeof(T);
            /* For over-aligned types, we require the type to fit in the inline buffer.
             * Users needing heap-allocated over-aligned types should provide an
             * appropriately aligned allocator. Ideally, there should not be such need. */
            if constexpr (align > alignof(typename allocator_traits::value_type))
            {
                static_assert(fits_in_buffer<T>,
                    "Over-aligned types must fit in buffer or use aligned allocator");
                return static_cast<pointer>(buffer);
            }
            else
            {
                return allocator_traits::allocate(allocator, size);
            }
        }

    public:
        /**
         * @brief Default constructor
         */
        value() noexcept : buffer(), data(nullptr), vtable(nullptr) {}

        /**
         * @brief Copy constructor
         * @param other Value to copy from
         */
        value(const value& other) : vtable(other.vtable)
        {
            if (!other.data)
            {
                data = nullptr;
                return;
            }

            /* Allocate matching storage and invoke copy construction
             * through the vtable. This may throw if the stored type's copy constructor
             * throws, in which case the allocated memory is leaked but the value remains
             * in a valid empty state. */
            data = other.uses_buffer()
                ? static_cast<pointer>(buffer)
                : allocator_traits::allocate(allocator, other.vtable->size);

            vtable->copy(data, other.data);
        }

        /**
         * @brief Move constructor
         * @param other Value to move from
         */
        value(value&& other) noexcept : vtable(std::exchange(other.vtable, nullptr))
        {
            if (other.uses_buffer())
            {
                /* For inline storage, we must move-construct into our buffer since we
                 * can't transfer ownership of space. The source is left empty. */
                data = buffer;
                if (vtable)
                    vtable->move(data, other.data);
            }
            else
            {
                /* For heap storage, simply transfer pointer ownership. No reallocation
                 * or object movement needed. */
                data = std::exchange(other.data, nullptr);
            }
        }

        /**
         * @brief Copy assignment operator
         * @param other Value to copy from
         * @return Reference to *this
         */
        value& operator=(const value& other)
        {
            if (this != &other)
            {
                const bool same_type = (vtable == other.vtable);

                /* When assigning the same type, reuse existing storage by copy-assigning
                 * in place. This avoids expensive deallocate-allocate cycles and is a
                 * common case in IR manipulation where instruction payloads change but
                 * types remain stable. */
                if (same_type && data && other.data)
                {
                    vtable->copy(data, other.data);
                    return *this;
                }

                clear();
                vtable = other.vtable;

                if (!other.data)
                {
                    data = nullptr;
                    return *this;
                }

                data = other.uses_buffer()
                    ? static_cast<pointer>(buffer)
                    : allocator_traits::allocate(allocator, vtable->size);

                vtable->copy(data, other.data);
            }
            return *this;
        }

        /**
         * @brief Move assignment operator
         * @param other Value to move from
         * @return Reference to *this
         */
        value& operator=(value&& other) noexcept
        {
            if (this != &other)
            {
                const bool same_type = (vtable == other.vtable);
                /* Move-assign in place without touching the allocator if both types
                 * are the same. This is the fastest path for repeated assignments. */
                if (same_type && data && other.data)
                {
                    vtable->move(data, other.data);
                    return *this;
                }

                clear();
                vtable = std::exchange(other.vtable, nullptr);

                if (other.uses_buffer())
                {
                    data = buffer;
                    if (vtable)
                        vtable->move(data, other.data);
                }
                else
                {
                    data = std::exchange(other.data, nullptr);
                }
            }
            return *this;
        }

        /**
         * @brief Destructor
         */
        ~value() noexcept
        {
            clear();
        }

        /**
         * @brief Construct a value in-place using value_traits
         *
         * Stores value_traits<T>::value internally.
         * Reuses existing storage when emplacing the same type.
         *
         * @tparam T Logical type to store
         * @tparam Args Constructor argument types for value_traits<T>::value
         * @param args Arguments forwarded to wrapper constructor
         */
        template<typename T, typename... Args>
            requires requires { typename value_traits<T>::value; } &&
                     std::constructible_from<typename value_traits<T>::value, Args...>
        void emplace(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<typename value_traits<T>::value, Args...> &&
                     fits_in_buffer<typename value_traits<T>::value>)
        {
            using wrapper_type = typename value_traits<T>::value;

            constexpr vtable_ptr new_vtable = &vtable_for<wrapper_type>;
            const bool same_type = (vtable == new_vtable);

            /* When emplacing the same type, reuse storage via destroy-then-construct.
             * This is the most efficient path for frequently-changed values like those
             * in constant folding or instruction combining passes. */
            if (same_type && data) [[likely]]
            {
                vtable->destroy(data);
                std::construct_at(static_cast<wrapper_type*>(data), std::forward<Args>(args)...);
                return;
            }

            clear();
            vtable = new_vtable;

            data = fits_in_buffer<wrapper_type>
                ? static_cast<pointer>(buffer)
                : allocate_for_type<wrapper_type>();

            std::construct_at(static_cast<wrapper_type*>(data), std::forward<Args>(args)...);
        }

        /**
         * @brief Get stored value
         *
         * Returns the actual data, unwrapping value_traits<T>::value.
         *
         * @tparam T Logical type to retrieve
         * @return Reference to stored data
         * @throws std::bad_cast if type mismatch
         */
        template<typename T>
        decltype(auto) get()
        {
            using wrapper_type = typename value_traits<T>::value;
            [[assume(data != nullptr)]];

            if (vtable != &vtable_for<wrapper_type>)
                throw std::bad_cast();

            auto& wrapper = *static_cast<wrapper_type*>(data);

            /* Unwrap the stored value. If the wrapper has a .data member, return it;
             * otherwise the wrapper itself is the data. This allows both simple wrappers
             * like { uint32_t data; } and complex ones where the entire struct is the value. */
            if constexpr (requires { wrapper.data; })
                return (wrapper.data);
            else
                return wrapper;
        }

        /**
         * @brief Get stored const value
         *
         * @tparam T Logical type to retrieve
         * @return Const reference to stored data
         * @throws std::bad_cast if type mismatch
         */
        template<typename T>
        decltype(auto) get() const
        {
            using wrapper_type = typename value_traits<T>::value;
            [[assume(data != nullptr)]];

            if (vtable != &vtable_for<wrapper_type>)
                throw std::bad_cast();

            const auto& wrapper = *static_cast<const wrapper_type*>(data);

            if constexpr (requires { wrapper.data; })
                return (wrapper.data);
            else
                return wrapper;
        }

        /**
         * @brief Swap contents with another value
         * @param other Value to swap with
         */
        void swap(value& other) noexcept
        {
            if (this == &other)
                return;

            const bool same_type = (vtable == other.vtable);

            /* For same-type values, use optimized byte-level swapping when both values
             * use the same storage. Vtable identity guarantees type identity, so
             * treating objects as raw bytes is safe. */
            if (same_type && data && other.data) [[likely]]
            {
                const bool both_buffer = uses_buffer() && other.uses_buffer();
                const bool both_heap = !uses_buffer() && !other.uses_buffer();

                if (both_buffer)
                {
                    /* Both values use inline storage - simply swap buffer contents through a
                     * temporary. Data pointers already point to their respective buffers. */
                    alignas(buffer_align) byte_type temp[buffer_size];
                    std::memcpy(temp, buffer, vtable->size);
                    std::memcpy(buffer, other.buffer, vtable->size);
                    std::memcpy(other.buffer, temp, vtable->size);
                    return;
                }
                else if (both_heap)
                {
                    /* Both values use heap storage - swap pointers. This is the
                     * fastest case since no data movement occurs. */
                    using std::swap;
                    swap(data, other.data);
                    return;
                }
            }
            /* Use three-way move for correctness and exception safety. This properly handles
             * object lifetimes and ownership transfer in all cases. */
            value temp(std::move(*this));
            *this = std::move(other);
            other = std::move(temp);
        }

        /**
         * @brief Get type information for stored value
         * @return Reference to std::type_info for stored wrapper type, or typeid(void) if empty
         */
        [[nodiscard]] const std::type_info& type() const noexcept
        {
            return vtable ? *vtable->type : typeid(void);
        }

        /**
         * @brief Get size of stored object in bytes
         * @return Size in bytes, or 0 if empty
         */
        [[nodiscard]] size_type stored_size() const noexcept
        {
            return vtable ? vtable->size : 0;
        }

        /**
         * @brief Check if value contains an object
         * @return true if storing a value, false otherwise
         */
        [[nodiscard]] bool has_value() const noexcept
        {
            return data != nullptr;
        }

        /**
         * @brief Check if value is empty
         * @return true if empty, false otherwise
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return !has_value();
        }
    };

    /**
     * @brief Cast value to type T
     * @tparam T Target type
     * @tparam Alloc Allocator type
     * @param v Value to cast
     * @return Reference to stored value
     * @throws std::bad_cast if type mismatch
     */
    template<typename T, typename Alloc>
    decltype(auto) value_cast(value<Alloc>& v)
    {
        return v.template get<T>();
    }

    /**
     * @brief Cast value to type const T
     * @tparam T Target type
     * @tparam Alloc Allocator type
     * @param v Value to cast
     * @return Const reference to stored value
     * @throws std::bad_cast if type mismatch
     */
    template<typename T, typename Alloc>
    decltype(auto) value_cast(const value<Alloc>& v)
    {
        return v.template get<T>();
    }

    /**
     * @brief Cast value to type T&&
     * @tparam T Target type
     * @tparam Alloc Allocator type
     * @param v Value to cast
     * @return Rvalue reference to stored value
     * @throws std::bad_cast if type mismatch
     */
    template<typename T, typename Alloc>
    decltype(auto) value_cast(value<Alloc>&& v)
    {
        return std::move(v.template get<T>());
    }

    /**
     * @brief Cast value pointer to type T pointer
     * @tparam T Target type
     * @tparam Alloc Allocator type
     * @param v Pointer to value
     * @return Pointer to stored value
     * @throws std::bad_cast if type mismatch or v is null
     */
    template<typename T, typename Alloc>
    auto* value_cast(value<Alloc>* v)
    {
        return v ? &v->template get<T>() : throw ach::fatal_error("bad value_cast!");
    }

    /**
     * @brief Cast const value pointer to const type T pointer
     * @tparam T Target type
     * @tparam Alloc Allocator type
     * @param v Pointer to const value
     * @return Pointer to stored value
     * @throws std::bad_cast if type mismatch or v is null
     */
    template<typename T, typename Alloc>
    const auto* value_cast(const value<Alloc>* v)
    {
        return v ? &v->template get<T>() : throw ach::fatal_error("bad value_cast!");
    }

    /**
     * @brief Swap two values
     * @tparam Alloc Allocator type
     * @param lhs First value
     * @param rhs Second value
     */
    template<typename Alloc>
    void swap(value<Alloc>& lhs, value<Alloc>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

}
