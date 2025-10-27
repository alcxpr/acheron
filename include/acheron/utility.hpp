/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>

namespace ach
{
    /**
     * @brief Thread-safe counter
     *
     * Provides a simple, efficient counter that conditionally selects
     * optimal memory ordering based on the target architecture.
     *
     * On x86/x64 (TSO architectures), uses relaxed memory ordering for better
     * performance since the hardware provides strong ordering guarantees.
     * On weakly-ordered architectures (ARM, AArch64, PowerPC), uses acquire-release
     * semantics to ensure proper synchronization.
     *
     * @tparam I Integral type. Defaults to `std::size_t`
     */
    template<std::integral I = std::size_t>
    class counter
    {
    public:
        /** @brief The underlying integral type used for the counter */
        using value_type = I;

    private:
        /** @brief The atomic value storage */
        std::atomic<value_type> val;

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86) /* TSO */
        /** @brief Memory ordering for increment operations on TSO architectures */
        static constexpr auto increment_order = std::memory_order_relaxed;
        /** @brief Memory ordering for decrement operations on TSO architectures */
        static constexpr auto decrement_order = std::memory_order_relaxed;
        /** @brief Memory ordering for load operations on TSO architectures */
        static constexpr auto load_order = std::memory_order_relaxed;
        /** @brief Memory ordering for store operations on TSO architectures */
        static constexpr auto store_order = std::memory_order_relaxed;
#else /* weakly-ordered architectures like ARM, Aarch64, PPC */
        /** @brief Memory ordering for increment operations on weakly-ordered architectures */
        static constexpr auto increment_order = std::memory_order_acq_rel;
        /** @brief Memory ordering for decrement operations on weakly-ordered architectures */
        static constexpr auto decrement_order = std::memory_order_acq_rel;
        /** @brief Memory ordering for load operations on weakly-ordered architectures */
        static constexpr auto load_order = std::memory_order_acquire;
        /** @brief Memory ordering for store operations on weakly-ordered architectures */
        static constexpr auto store_order = std::memory_order_release;
#endif

    public:
        /**
         * @brief Default constructor
         * 
         * Initializes the counter to zero.
         */
        constexpr counter() noexcept : val(0) {}
        
        /**
         * @brief Constructs a counter with an initial value
         * 
         * @param initial The initial value for the counter
         */
        constexpr explicit counter(value_type initial) noexcept : val(initial) {}

        /** @brief Copy constructor (deleted - counters are not copyable) */
        counter(const counter&) = delete;
        
        /** @brief Copy assignment operator (deleted - counters are not copyable) */
        counter& operator=(const counter&) = delete;
        
        /** @brief Move constructor (deleted - counters are not movable) */
        counter(counter&&) = delete;
        
        /** @brief Move assignment operator (deleted - counters are not movable) */
        counter& operator=(counter&&) = delete;

        /**
         * @brief Pre-increment operator
         * 
         * Atomically increments the counter by 1.
         * 
         * @return The value before the increment
         */
        value_type operator++() noexcept
        {
            return val.fetch_add(1, increment_order);
        }

        /**
         * @brief Post-increment operator
         * 
         * Atomically increments the counter by 1.
         * 
         * @return The value before the increment
         */
        value_type operator++(int) noexcept
        {
            return val.fetch_add(1, increment_order);
        }

        /**
         * @brief Addition assignment operator
         * 
         * Atomically adds a delta value to the counter.
         * 
         * @param delta The value to add to the counter
         * @return The value before the addition
         */
        value_type operator+=(value_type delta) noexcept
        {
            return val.fetch_add(delta, increment_order);
        }
        
        /**
         * @brief Pre-decrement operator
         * 
         * Atomically decrements the counter by 1.
         * 
         * @return The value before the decrement
         */
        value_type operator--() noexcept
        {
            return val.fetch_sub(1, decrement_order);
        }

        /**
         * @brief Post-decrement operator
         * 
         * Atomically decrements the counter by 1.
         * 
         * @return The value before the decrement
         */
        value_type operator--(int) noexcept
        {
            return val.fetch_sub(1, increment_order);
        }

        /**
         * @brief Subtraction assignment operator
         * 
         * Atomically subtracts a delta value from the counter.
         * 
         * @param delta The value to subtract from the counter
         * @return The value before the subtraction
         */
        value_type operator-=(value_type delta) noexcept
        {
            return val.fetch_sub(delta, increment_order);
        }

        /**
         * @brief Atomically loads the current value
         * 
         * @return The current value of the counter
         */
        [[nodiscard]] value_type load() const noexcept
        {
            return val.load(load_order);
        }

        /**
         * @brief Gets the current value of the counter
         * 
         * @return The current value of the counter
         */
        [[nodiscard]] value_type value() const noexcept
        {
            return load();
        }

        /**
         * @brief Explicit conversion to the underlying value type
         * 
         * @return The current value of the counter
         */
        [[nodiscard]] explicit operator value_type() const noexcept
        {
            return load();
        }

        /**
         * @brief Atomically stores a new value
         * 
         * @param desired The value to store in the counter
         */
        void store(value_type desired) noexcept
        {
            val.store(desired, store_order);
        }

        /**
         * @brief Resets the counter to a specified value
         * 
         * @param desired The value to reset the counter to (defaults to 0)
         */
        void reset(value_type desired = 0) noexcept
        {
            store(desired);
        }

        /**
         * @brief Query if the counter object is lock-free
         * 
         * Determines whether atomic operations on this counter are lock-free
         * (i.e., using native atomic instructions rather than locks).
         * 
         * @return `true` if the counter is lock-free, `false` otherwise
         */
        [[nodiscard]] bool is_lock_free() const noexcept
        {
            return val.is_lock_free();
        }
    };

    /**
     * @brief Template deduction guide for `ach::counter`
     * 
     * Allows construction of a counter from an integral value without
     * explicitly specifying the template parameter.
     * 
     * @tparam I The deduced integral type
     */
    template<std::integral I>
    counter(I) -> counter<I>;
}
