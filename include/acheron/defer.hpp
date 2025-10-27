/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace ach
{
    /**
     * @brief Concept for types that can be deferred
     * @details A deferrable type must be invocable with no arguments and nothrow move constructible
     * to prevent exceptions during scope guard transfer
     */
    template <typename F>
    concept is_deferrable = std::invocable<F> && std::is_nothrow_move_constructible_v<F>;

    /**
     * @brief Alias for `is_deferrable` concept in PascalCase style
     */
    template <typename F>
    concept Deferrable = is_deferrable<F>;

    /**
     * @brief RAII scope guard that executes a callable on destruction
     * @tparam F Callable type satisfying Deferrable concept
     *
     * @details
     * `defer` ensures a cleanup action is executed when leaving scope, either
     * normally or through exception. The guard can be moved to transfer ownership,
     * cancelled to prevent execution, or executed early.
     *
     * It is also worth noting that while C++ has RAII as a feature, `defer` is made
     * to integrate with dynamic resources that are returned from C-style functions.
     *
     * @par Example
     * @code
     * void proc_file()
     * {
     *    int fd = open("file.txt", O_RDONLY);
     *    auto guard = defer([fd]{ close(fd); });
     *    ...
     *    // `close()` is called automatically on scope exit.
     * }
     * @endcode
     *
     * @par Thread Safety
     * defer is not thread-safe. Each instance should be owned by a single thread.
     */
    template <Deferrable F>
    class defer
    {
    public:
        /**
        * @brief Constructs a defer guard with the given callable
        * @param fn Callable to execute on scope exit
        * @throws any Any exception thrown by F's constructor
        */
        explicit constexpr defer(F&& fn) noexcept(std::is_nothrow_constructible_v<F, F&&>)
            : fn(std::forward<F>(fn)) {}

        /**
        * @brief Deleted copy constructor
        */
        defer(const defer&) = delete;

        /**
        * @brief Deleted copy assignment
        */
        defer& operator=(const defer&) = delete;

        /**
        * @brief Move constructor; transfers ownership of the deferred action
        * @param other Source defer guard which will be deactivated
        * @throws any Any exception thrown by F's move constructor
        * @post `other.is_active() == false`
        */
        constexpr defer(defer&& other) noexcept(std::is_nothrow_move_constructible_v<F>)
            : fn(std::move(other.fn)), active(std::exchange(other.active, false))
        {
        }

        /**
        * @brief Move assignment; executes current action, if active, then takes ownership
        * @param other Source defer guard which will be deactivated
        * @return Reference to `*this`
        * @throws any Any exception thrown by F's move assignment or invocation
        * @post `other.is_active() == false`
        */
        constexpr defer& operator=(defer&& other) noexcept(std::is_nothrow_move_assignable_v<F>
            && std::is_nothrow_invocable_v<F>)
        {
            if (this != &other)
            {
                if (active)
                    fn();

                fn = std::move(other.fn);
                active = other.active;
                other.active = false;
            }
            return *this;
        }

        /**
        * @brief Destructor; executes the deferred action if still active
        * @note If F's invocation throws during stack unwinding, `std::terminate` is called
        */
        ~defer() noexcept(std::is_nothrow_invocable_v<F>)
        {
            if (active)
                fn();
        }

        /**
        * @brief Cancels the deferred action (prevents execution on destruction)
        * @post `is_active() == false`
        */
        constexpr void cancel() noexcept
        {
            active = false;
        }

        /**
        * @brief Executes the deferred action immediately and deactivates the guard
        * @throws any Any exception thrown by F's invocation
        * @post `is_active() == false`
        * @note If already inactive, this is a no-op
        */
        constexpr void execute() noexcept(std::is_nothrow_invocable_v<F>)
        {
            if (active)
            {
            	active = false;
                fn();
            }
        }

        /**
        * @brief Checks if the deferred action is still active
        * @return `true` if the action will execute on destruction, `false` otherwise
        */
        [[nodiscard]] constexpr bool is_active() const noexcept
        {
            return active;
        }

    private:
        F fn; ///< The callable to execute
        bool active = true; ///< Whether the action is still pending
    };

    /**
     * @brief Deduction guide for defer
     */
    template <typename F>
    defer(F) -> defer<F>;
}