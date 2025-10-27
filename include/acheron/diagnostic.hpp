/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <exception>
#include <iostream>
#include <source_location>
#ifndef NDEBUG
#include <stacktrace>
#endif
#include <string>
#include <string_view>
#include <utility>

namespace ach
{
	/**
	 * @brief Exception class for fatal, unrecoverable errors
	 * 
	 * @details
	 * `fatal_error` represents a critical program state where continuation is impossible.
	 * It automatically captures source location and, in debug builds, full stack trace
	 * at construction. Diagnostic information is printed to stderr before being thrown.
	 * 
	 * This is intended for programmer errors and invariant violations, not for
	 * expected runtime errors (use `std::expected` or `std::optional` for those).
	 * 
	 * @par Example
	 * @code
	 * void initialize_system()
	 * {
	 *     if (!load_critical_config())
	 *         throw ach::fatal_error("failed to load critical configuration");
	 * }
	 * @endcode
	 * 
	 * @note All exceptions in Acheron are fatal. There is no catch-and-recover pattern
	 */
	class fatal_error : public std::exception
	{
	public:
		/**
		 * @brief Constructs a fatal error with diagnostic information
		 * @param message Error message describing what went wrong
		 * @param location Auto-captured source location where the error occurred
		 * 
		 * @note Immediately prints diagnostic information to stderr upon construction
		 */
		explicit fatal_error(std::string message, std::source_location location = std::source_location::current()) 
            : msg(std::move(message)), loc(location)
		{
			print_diagnostic();
		}

		/**
		 * @brief Returns the error message
		 * @return C-string containing the error message
		 */
		[[nodiscard]] const char* what() const noexcept override
		{
			return msg.c_str();
		}

		/**
		 * @brief Returns the source location where the error occurred
		 * @return Source location information
		 */
		[[nodiscard]] const std::source_location& location() const noexcept
		{
			return loc;
		}

	private:
		std::string msg;
		std::source_location loc;

		void print_diagnostic() const
		{
			std::cerr << "\n" << std::string(80, '=') << "\n";
			std::cerr << "FATAL ERROR: program will terminate\n";
			std::cerr << std::string(80, '=') << "\n";
			std::cerr << "message: " << msg << "\n";
			std::cerr << "location: " << loc.file_name() 
			          << ":" << loc.line() 
			          << ":" << loc.column() << "\n";
			std::cerr << "function: " << loc.function_name() << "\n";
			
			#ifndef NDEBUG
			std::cerr << "\nstack trace:\n" << std::to_string(std::stacktrace::current()) << "\n";
			#endif
			
			std::cerr << std::string(80, '=') << "\n";
		}
	};

	/**
	 * @brief Asserts a condition and terminates if false
	 * @param condition Condition to check
	 * @param message Pre-formatted error message by caller
	 * @param location Auto-captured source location
	 * 
	 * @details
	 * If the condition is false, prints diagnostic information and calls
	 * `std::terminate()`. In debug builds (NDEBUG not defined), includes
	 * full stack trace. In release builds, only prints message and location.
	 * 
	 * Caller is responsible for formatting the message string if needed.
	 * This keeps the fast path fast and allows conditional formatting.
	 * 
	 * @par Example
	 * @code
	 * void process(int* ptr, std::size_t size)
	 * {
	 *     ach::assert(ptr != nullptr, "pointer is null");
	 *     ach::assert(size > 0, std::format("invalid size: {}", size));
	 * }
	 * @endcode
	 * 
	 * @note This function always checks the condition regardless of build type.
	 * Only the stack trace capture is conditional on NDEBUG.
	 */
	inline void assert(bool condition, std::string_view message, std::source_location location = std::source_location::current()) 
    {
		if (!condition) [[unlikely]]
		{
			std::cerr << "\n" << std::string(80, '=') << "\n";
			std::cerr << "ASSERTION FAILED\n";
			std::cerr << std::string(80, '=') << "\n";
			std::cerr << "message: " << message << "\n";
			std::cerr << "location: " << location.file_name() 
			          << ":" << location.line() 
			          << ":" << location.column() << "\n";
			std::cerr << "function: " << location.function_name() << "\n";
			
			#ifndef NDEBUG
			std::cerr << "\nstack trace:\n" << std::to_string(std::stacktrace::current()) << "\n";
			#endif
			
			std::cerr << std::string(80, '=') << "\n";
			std::terminate();
		}
	}

	/**
	* @brief Debug-only assertion that is compiled out in release builds
	* @param condition Condition to check
	* @param message Pre-formatted error message by caller
	* @param location Auto-captured source location
	* 
	* @details
	* Behaves like `ach::assert()` in debug builds (NDEBUG not defined),
	* but becomes a no-op in release builds for zero runtime overhead.
	* Use this for performance-critical hot paths where the check has
	* measurable impact but still want validation during development.
	* 
	* @par Example
	* @code
	* void foo(int* ptr, std::size_t size)
	* {
	*     ach::debug_assert(ptr != nullptr, "pointer is null");
	*     ach::debug_assert(size > 0, std::format("invalid size: {}", size));
	* }
	* @endcode
	* 
	* @note For library invariants that should always be checked, use `ach::assert()` instead.
	*/
	inline void debug_assert([[maybe_unused]] bool condition, 
							[[maybe_unused]] std::string_view message, 
							[[maybe_unused]] std::source_location location = std::source_location::current())
	{
#if defined(DEBUG) || !defined(NDEBUG)
		::ach::assert(condition, message, location);
#endif
	}

	/**
	 * @brief Immediately terminates the program with a message
	 * @param message Error message
	 * @param location Auto-captured source location
	 * 
	 * @details
	 * Alias for `assert(false, message)`. Use when the condition itself
	 * is "this code path executing". Useful for marking unreachable code
	 * or unimplemented features.
	 * 
	 * @par Example
	 * @code
	 * enum class color { red, green, blue };
	 * 
	 * const char* color_name(color c)
	 * {
	 *     switch (c)
	 *     {
	 *         case color::red:   return "red";
	 *         case color::green: return "green";
	 *         case color::blue:  return "blue";
	 *     }
	 *     ach::panic(std::format("invalid color: {}", static_cast<int>(c)));
	 * }
	 * @endcode
	 */
	[[noreturn]] inline void panic(std::string_view message, std::source_location location = std::source_location::current()) 
    {
		::ach::assert(false, message, location);
		std::unreachable();
	}

	/**
	 * @brief Prints a warning message in yellow to stderr
	 * @param message Warning message
	 * @param location Auto-captured source location
	 * 
	 * @details
	 * Warnings indicate potential issues that don't warrant termination
	 * but should be brought to the developer's attention. Output is
	 * colored yellow on terminals that support ANSI escape codes.
	 * 
	 * @par Example
	 * @code
	 * void allocate(std::size_t size)
	 * {
	 *     if (size > 1024 * 1024)
	 *         ach::warn(std::format("large allocation: {} bytes", size));
	 * }
	 * @endcode
	 */
	inline void warn(std::string_view message, std::source_location location = std::source_location::current()) 
    {
		std::cerr << "\033[33m";
		std::cerr << "[WARN] " << message;
		std::cerr << " (at " << location.file_name() << ":" << location.line() << ")";
		std::cerr << "\033[0m\n";
	}

	/**
	 * @brief Prints an info message in blue to stderr
	 * @param message Info message
	 * @param location Auto-captured source location
	 * 
	 * @details
	 * Info messages provide diagnostic information during program execution.
	 * Useful for tracking program flow or state during development.
	 * Output is colored blue on terminals that support ANSI escape codes.
	 * 
	 * @par Example
	 * @code
	 * void initialize()
	 * {
	 *     ach::info("initializing subsystems");
	 *     // ...
	 *     ach::info(std::format("initialization complete in {}ms", elapsed));
	 * }
	 * @endcode
	 */
	inline void info(std::string_view message, std::source_location location = std::source_location::current()) 
    {
		std::cerr << "\033[34m";
		std::cerr << "[INFO] " << message;
		std::cerr << " (at " << location.file_name() << ":" << location.line() << ")";
		std::cerr << "\033[0m\n";
	}

    /**
	 * @brief Prints an info message in blue to stderr in debug build
	 * @param message Info message
	 * @param location Auto-captured source location
	 * 
	 * @details
	 * Info messages provide diagnostic information during program execution.
	 * Useful for tracking program flow or state during development.
	 * Output is colored blue on terminals that support ANSI escape codes.
	 * 
	 * @par Example
	 * @code
	 * void initialize()
	 * {
	 *     ach::info("initializing subsystems");
	 *     // ...
	 *     ach::info(std::format("initialization complete in {}ms", elapsed));
	 * }
	 * @endcode
	 */
    inline void debug(std::string_view message, std::source_location location = std::source_location::current())
    {
#if defined(DEBUG) || !defined(NDEBUG)
        ::ach::info(message, location);
#endif
        return;
    }
}

/* macro alias for shorthand uses */
#define ACH_ASSERT(cond, msg)     ::ach::assert((cond), (msg), std::source_location::current())
#define ACH_PANIC(msg)            ::ach::panic((msg), std::source_location::current())
#define ACH_WARN(msg)             ::ach::warn((msg), std::source_location::current())
#define ACH_INFO(msg)             ::ach::info((msg), std::source_location::current())
#define ACH_DEBUG(msg)            ::ach::debug((msg), std::source_location::current())
#define ACH_UNREACHABLE()         ::ach::panic("unreachable code reached", std::source_location::current())
#define ACH_TODO(msg)             ::ach::panic(std::format("TODO: {}", (msg)), std::source_location::current())
#define ACH_DEBUG_ASSERT(cond, msg)   ::ach::debug_assert((cond), (msg), std::source_location::current())
