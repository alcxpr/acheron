/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <thread>
#include <type_traits>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace ach
{
	/**
	 * @enum allocation_policy
	 * @brief Allocation policy for the allocator
	 */
	enum class allocation_policy
	{
		local, ///< Thread-local allocation.
		shared ///< Thread-safe allocation.
	};

	namespace detail
	{
		/**
		 * @class arena
		 * @brief Memory arena managing a fixed-size region with bump and bitmap allocation
		 * @tparam P Allocation policy
		 * @note Arena size is 64MB by default, tuned based on empirical testing
		 */
		template<allocation_policy P>
		class arena
		{
		public:
			static constexpr std::size_t arena_size = 64 * 1024 * 1024; ///< Size of each arena in bytes (64MB)
			static constexpr std::size_t l2_per_l1 = 64; ///< Number of L2 bitmap words per L1 bitmap bit

		private:
			template<typename T>
			using maybe_atomic = std::conditional_t<P == allocation_policy::shared, std::atomic<T>, T>;

			void *base_addr; ///< Base address of the arena memory region
			std::size_t block_size; ///< Size of each block in bytes
			std::uint8_t block_shift; ///< Log2(block_size) for fast division
			std::size_t num_blocks; ///< Total number of blocks in this arena
			std::size_t usable_capacity; ///< Usable capacity
			alignas(std::hardware_destructive_interference_size) maybe_atomic<std::size_t> bump_offset; ///< Bump offset
			alignas(std::hardware_destructive_interference_size) maybe_atomic<std::size_t> alloc_count; ///> Alloc count

			std::size_t l2_words; ///< Number of 64-bit words in L2 bitmap
			std::size_t l1_bits; ///< Number of bits in L1 bitmap
			std::size_t l1_words; ///< Number of 64-bit words in L1 bitmap
			maybe_atomic<std::uint64_t> *l1_bitmap; ///< L1 bitmap
			maybe_atomic<std::uint64_t> *l2_bitmap; ///< L2 bitmap

		public:
			/**
			 * @brief Constructs an arena for the given block size
			 * @param block_sz Size of each block (must be power of 2)
			 * @throws std::bad_alloc if memory allocation fails
			 *
			 * Allocates a 64MB region using `VirtualAlloc` or `mmap`.
			 * Initializes bitmaps at the end of the arena to keep allocations contiguous.
			 */
			explicit arena(std::size_t block_sz) :
				block_size(block_sz)
				, block_shift(std::countr_zero(block_sz))
			{
				if constexpr (P == allocation_policy::shared)
				{
					bump_offset.store(0, std::memory_order_relaxed);
					alloc_count.store(0, std::memory_order_relaxed);
				}
				else
				{
					bump_offset = 0;
					alloc_count = 0;
				}

#if defined(_WIN32) || defined(_WIN64)
				base_addr = VirtualAlloc(nullptr, arena_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
				if (!base_addr)
				{
					throw std::bad_alloc();
				}
#else
				base_addr = mmap(nullptr, arena_size, PROT_READ | PROT_WRITE,
				                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				if (base_addr == MAP_FAILED)
					throw std::bad_alloc();
#endif

				std::size_t theoretical_blocks = arena_size / block_size;
				l2_words = (theoretical_blocks + 63) / 64;
				l1_bits = (l2_words + l2_per_l1 - 1) / l2_per_l1;
				l1_words = (l1_bits + 63) / 64;

				std::size_t bitmap_bytes = (l2_words + l1_words) * sizeof(maybe_atomic<std::uint64_t>);
				usable_capacity = arena_size - bitmap_bytes;
				num_blocks = usable_capacity / block_size;

				char *bitmap_start = static_cast<char *>(base_addr) + usable_capacity;
				l2_bitmap = reinterpret_cast<maybe_atomic<std::uint64_t> *>(bitmap_start);
				l1_bitmap = l2_bitmap + l2_words;

				if constexpr (P == allocation_policy::shared)
				{
					for (std::size_t i = 0; i < l2_words; ++i)
						l2_bitmap[i].store(0, std::memory_order_relaxed);
					for (std::size_t i = 0; i < l1_words; ++i)
						l1_bitmap[i].store(0, std::memory_order_relaxed);
				}
				else
				{
					std::memset(l2_bitmap, 0, l2_words * sizeof(std::uint64_t));
					std::memset(l1_bitmap, 0, l1_words * sizeof(std::uint64_t));
				}
			}

			/**
			 * @brief Destructor. This releases arena memory back to OS
			 */
			~arena() noexcept
			{
#if defined(_WIN32) || defined(_WIN64)
				if (base_addr)
				{
					VirtualFree(base_addr, 0, MEM_RELEASE);
				}
#else
				if (base_addr != MAP_FAILED)
				{
					munmap(base_addr, arena_size);
				}
#endif
			}

			arena(const arena &) = delete;
			arena &operator=(const arena &) = delete;
			arena(arena &&) = delete;
			arena &operator=(arena &&) = delete;

			/**
			 * @brief Allocates a block from this arena
			 * @return Pointer to allocated block, or nullptr if arena is full
			 *
			 * @note Returns nullptr instead of throwing to allow pool to try next arena
			 */
			[[nodiscard]] void *allocate() noexcept
			{
				if constexpr (P == allocation_policy::shared)
				{
					std::size_t offset = bump_offset.load(std::memory_order_relaxed);
					while (offset < usable_capacity)
					{
						std::size_t new_offset = offset + block_size;
						if (bump_offset.compare_exchange_weak(offset, new_offset,
						                                      std::memory_order_acquire, std::memory_order_relaxed))
						{
							return static_cast<char *>(base_addr) + offset;
						}
					}
				}
				else
				{
					if (bump_offset < usable_capacity) [[likely]]
					{
						void *ptr = static_cast<char *>(base_addr) + bump_offset;
						bump_offset += block_size;
						return ptr;
					}
				}

				return try_bitmap_allocate();
			}

			/**
			 * @brief Deallocates a block, marking it free in the bitmap
			 * @param ptr Pointer to block allocated from this arena
			 *
			 * Updates the L2 bitmap and propagates to L1 bitmap for fast future searches.
			 *
			 * @warning Behavior is undefined if ptr was not allocated from this arena
			 */
			void deallocate(void *ptr) noexcept
			{
				const std::size_t block_index = pointer_to_block_index(ptr);
				mark_free(block_index);
			}

			/**
			 * @brief Checks if a pointer was allocated from this arena
			 * @param ptr Pointer to check
			 * @return true if ptr is within this arena's usable region
			 *
			 * Uses unsigned wraparound trick for single-comparison ownership test.
			 */
			[[nodiscard]] bool owns(void *ptr) const noexcept
			{
				/* note: this relies on unsigned number behavior wraparound just so we can simply do a single
				 * comparison instead of multiple cmps like below.
				 *
				 * return ptr >= base_addr && ptr < (static_cast<char *>(base_addr) + usable_capacity);
				 */
				std::size_t offset = static_cast<char*>(ptr) - static_cast<char*>(base_addr);
    			return offset < usable_capacity;
			}

			/**
			 * @brief Checks if the arena is full (no blocks available)
			 * @return true if bump allocator exhausted and no free blocks in bitmap
			 */
			[[nodiscard]] bool is_full() const noexcept
			{
				std::size_t offset;
				if constexpr (P == allocation_policy::shared)
					offset = bump_offset.load(std::memory_order_relaxed);
				else
					offset = bump_offset;

				if (offset < usable_capacity)
					return false;

				for (std::size_t i = 0; i < l1_words; ++i)
				{
					std::uint64_t word;
					if constexpr (P == allocation_policy::shared)
						word = l1_bitmap[i].load(std::memory_order_relaxed);
					else
						word = l1_bitmap[i];

					if (word != 0)
						return false;
				}
				return true;
			}

			/**
			 * @brief Returns the block size for this arena
			 * @return Block size in bytes
			 */
			[[nodiscard]] std::size_t get_block_size() const noexcept
			{
				return block_size;
			}

		private:
			/**
			 * @brief Tries to allocate from bitmap (L1/L2 hierarchical search)
			 * @return Pointer to allocated block, or nullptr if no free blocks
			 *
			 * @details This makes uses of counter-based round-robin as the starting
			 * point to distribute allocations and reduce contention.
			 */
			[[nodiscard]] void *try_bitmap_allocate() noexcept
			{
				std::size_t counter;
				if constexpr (P == allocation_policy::shared)
					counter = alloc_count.fetch_add(1, std::memory_order_relaxed);
				else
					counter = alloc_count++;

				std::size_t start_l1_bit = counter % l1_bits;
				std::size_t l1_word_idx = start_l1_bit / 64;
				std::size_t l1_bit_offset = start_l1_bit % 64;

				for (std::size_t i = 0; i < l1_words; ++i)
				{
					std::size_t idx = (l1_word_idx + i) % l1_words;
					std::uint64_t l1_word;

					if constexpr (P == allocation_policy::shared)
						l1_word = l1_bitmap[idx].load(std::memory_order_acquire);
					else
						l1_word = l1_bitmap[idx];

					if (i == 0 && l1_bit_offset != 0)
						l1_word &= (~0ULL << l1_bit_offset);

					if (l1_word != 0)
					{
						int l1_bit = std::countr_zero(l1_word);
						std::size_t l1_index = idx * 64 + l1_bit;
						std::size_t l2_region_start = l1_index * l2_per_l1;

						for (std::size_t j = 0; j < l2_per_l1; ++j)
						{
							std::size_t l2_idx = l2_region_start + j;
							if (l2_idx >= l2_words)
								break;

							if constexpr (P == allocation_policy::shared)
							{
								std::uint64_t l2_word = l2_bitmap[l2_idx].load(std::memory_order_acquire);
								while (l2_word != 0)
								{
									int bit = std::countr_zero(l2_word);
									std::size_t block_index = l2_idx * 64 + bit;

									if (block_index >= num_blocks)
										break;

									std::uint64_t new_word = l2_word & ~(1ULL << bit);
									if (l2_bitmap[l2_idx].compare_exchange_weak(l2_word, new_word,
										std::memory_order_release, std::memory_order_acquire))
									{
										if (new_word == 0)
											update_l1_for_region(l1_index);
										return static_cast<char *>(base_addr) + (block_index << block_shift);
									}
								}
							}
							else
							{
								std::uint64_t l2_word = l2_bitmap[l2_idx];
								if (l2_word != 0)
								{
									int bit = std::countr_zero(l2_word);
									std::size_t block_index = l2_idx * 64 + bit;

									if (block_index >= num_blocks)
										break;

									l2_bitmap[l2_idx] &= ~(1ULL << bit);
									if (l2_bitmap[l2_idx] == 0)
										update_l1_for_region(l1_index);
									return static_cast<char *>(base_addr) + (block_index << block_shift);
								}
							}
						}
					}
				}

				return nullptr;
			}

			/**
			 * @brief Marks a block as free in the bitmap
			 * @param block_index Index of the block to free
			 *
			 * Updates both L2 and L1 bitmaps using atomic operations if shared policy.
			 */
			void mark_free(std::size_t block_index) noexcept
			{
				std::size_t l2_idx = block_index / 64;
				std::size_t bit = block_index % 64;

				if constexpr (P == allocation_policy::shared)
					l2_bitmap[l2_idx].fetch_or(1ULL << bit, std::memory_order_release);
				else
					l2_bitmap[l2_idx] |= (1ULL << bit);

				std::size_t l1_bit = l2_idx / l2_per_l1;
				std::size_t l1_word = l1_bit / 64;
				std::size_t l1_offset = l1_bit % 64;

				if constexpr (P == allocation_policy::shared)
					l1_bitmap[l1_word].fetch_or(1ULL << l1_offset, std::memory_order_release);
				else
					l1_bitmap[l1_word] |= (1ULL << l1_offset);
			}

			/**
			 * @brief Updates L1 bitmap bit for an L2 region
			 * @param l1_bit_index Bit index in L1 bitmap
			 *
			 * Scans the corresponding L2 region to determine if any blocks are free,
			 * then updates the L1 summary bit accordingly.
			 */
			void update_l1_for_region(std::size_t l1_bit_index) noexcept
			{
				std::size_t l2_region_start = l1_bit_index * l2_per_l1;

				bool has_free = false;
				for (std::size_t i = 0; i < l2_per_l1 && (l2_region_start + i) < l2_words; ++i)
				{
					std::uint64_t word;
					if constexpr (P == allocation_policy::shared)
						word = l2_bitmap[l2_region_start + i].load(std::memory_order_acquire);
					else
						word = l2_bitmap[l2_region_start + i];

					if (word != 0)
					{
						has_free = true;
						break;
					}
				}

				std::size_t l1_word = l1_bit_index / 64;
				std::size_t l1_bit = l1_bit_index % 64;

				if constexpr (P == allocation_policy::shared)
				{
					if (has_free)
						l1_bitmap[l1_word].fetch_or(1ULL << l1_bit, std::memory_order_release);
					else
						l1_bitmap[l1_word].fetch_and(~(1ULL << l1_bit), std::memory_order_release);
				}
				else
				{
					if (has_free)
						l1_bitmap[l1_word] |= (1ULL << l1_bit);
					else
						l1_bitmap[l1_word] &= ~(1ULL << l1_bit);
				}
			}

			/**
			 * @brief Converts pointer to block index
			 * @param ptr Pointer within arena
			 * @return Block index
			 */
			[[nodiscard]] std::size_t pointer_to_block_index(const void *ptr) const noexcept
			{
				std::size_t offset = static_cast<const char *>(ptr) - static_cast<const char *>(base_addr);
				return offset >> block_shift;
			}
		};

		/**
		 * @class size_class_manager
		 * @brief Manages power-of-2 size class bucketing
		 *
		 * Rounds allocation sizes to the next power of 2 for efficient pooling.
		 * Size classes: 8, 16, 32, 64, 128, 256, 512, 1024, ... up to 4MB
		 */
		class size_class_manager
		{
		public:
			static constexpr std::size_t min_size = 8;
			static constexpr std::size_t max_size = 4 * 1024 * 1024;
			static constexpr std::size_t num_size_classes = 20;

			/**
			 * @brief Rounds size to next power-of-2 size class
			 * @param size Requested size in bytes
			 * @return Rounded size, or 0 if exceeds max_size
			 */
			[[nodiscard]] static constexpr std::size_t round_to_size_class(std::size_t size) noexcept
			{
				if (size <= min_size)
					return min_size;

				if (size > max_size)
					return 0;

				return static_cast<std::size_t>(1) << (std::bit_width(size - 1));
			}

			/**
			 * @brief Converts size class to pool array index
			 * @param size Size class. Must be a power-of-two
			 * @return Index in [0, num_size_classes)
			 */
			[[nodiscard]] static constexpr std::size_t size_to_index(std::size_t size) noexcept
			{
				if (size <= min_size)
					return 0;
				return std::bit_width(size - 1) - std::bit_width(min_size - 1);
			}
		};
	}

	/**
	 * @class arena_pool
	 * @brief Pool of arenas for a specific block size
	 *
	 * @tparam P Allocation policy (local or shared)
	 *
	 * Manages up to 16 arenas, allocating new ones as previous ones fill up.
	 * Searches arenas in round-robin fashion to distribute allocations.
	 *
	 * @note Cache-line alignment on hot data prevents false sharing in shared policy
	 */
	template<allocation_policy P>
	class arena_pool
	{
	private:
		static constexpr std::size_t max_arenas = 16;

		using arena_type = detail::arena<P>;
		template<typename T>
		using maybe_atomic = std::conditional_t<P == allocation_policy::shared, std::atomic<T>, T>;

		alignas(std::hardware_destructive_interference_size) maybe_atomic<arena_type *> arenas[max_arenas];
		alignas(std::hardware_destructive_interference_size) maybe_atomic<std::size_t> num_arenas;
		alignas(std::hardware_destructive_interference_size) maybe_atomic<std::size_t> current_arena;
		std::size_t block_size;

	public:
		/**
		 * @brief Constructs an empty arena pool
		 * @param block_sz Block size for arenas in this pool
		 */
		explicit arena_pool(std::size_t block_sz) noexcept :
			block_size(block_sz)
		{
			if constexpr (P == allocation_policy::shared)
			{
				for (auto &arena: arenas)
					arena.store(nullptr, std::memory_order_relaxed);
				num_arenas.store(0, std::memory_order_relaxed);
				current_arena.store(0, std::memory_order_relaxed);
			}
			else
			{
				for (auto &arena: arenas)
					arena = nullptr;
				num_arenas = 0;
				current_arena = 0;
			}
		}

		~arena_pool() noexcept
		{
			std::size_t count;
			if constexpr (P == allocation_policy::shared)
				count = num_arenas.load(std::memory_order_relaxed);
			else
				count = num_arenas;

			for (std::size_t i = 0; i < count; ++i)
			{
				arena_type *arena;
				if constexpr (P == allocation_policy::shared)
					arena = arenas[i].load(std::memory_order_relaxed);
				else
					arena = arenas[i];

				if (arena)
				{
					arena->~arena_type();
#if defined(_WIN32) || defined(_WIN64)
					VirtualFree(arena, 0, MEM_RELEASE);
#else
					munmap(arena, sizeof(arena_type));
#endif
				}
			}
		}

		arena_pool(const arena_pool &) = delete;
		arena_pool &operator=(const arena_pool &) = delete;

		[[nodiscard]] void *allocate() noexcept
		{
			std::size_t current_idx, num;

			if constexpr (P == allocation_policy::shared)
			{
				current_idx = current_arena.load(std::memory_order_acquire);
				num = num_arenas.load(std::memory_order_acquire);
			}
			else
			{
				current_idx = current_arena;
				num = num_arenas;
			}

			if (current_idx < num)
			{
				arena_type *arena;
				if constexpr (P == allocation_policy::shared)
					arena = arenas[current_idx].load(std::memory_order_acquire);
				else
					arena = arenas[current_idx];

				if (void *ptr = arena->allocate())
					return ptr;
			}

			for (std::size_t i = 0; i < num; ++i)
			{
				if (i == current_idx)
					continue;

				arena_type *arena;
				if constexpr (P == allocation_policy::shared)
					arena = arenas[i].load(std::memory_order_acquire);
				else
					arena = arenas[i];

				if (void *ptr = arena->allocate())
				{
					if constexpr (P == allocation_policy::shared)
						current_arena.store(i, std::memory_order_release);
					else
						current_arena = i;
					return ptr;
				}
			}

			if constexpr (P == allocation_policy::shared)
			{
				std::size_t expected = num;
				if (expected >= max_arenas)
					return nullptr;

				if (num_arenas.compare_exchange_strong(expected, expected + 1,
				                                       std::memory_order_acq_rel, std::memory_order_acquire))
				{
#if defined(_WIN32) || defined(_WIN64)
					void *arena_mem = VirtualAlloc(nullptr, sizeof(arena_type),
					                               MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
					if (!arena_mem)
					{
						num_arenas.fetch_sub(1, std::memory_order_release);
						return nullptr;
					}
#else
					void *arena_mem = mmap(nullptr, sizeof(arena_type),
					                       PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
					if (arena_mem == MAP_FAILED)
					{
						num_arenas.fetch_sub(1, std::memory_order_release);
						return nullptr;
					}
#endif

					arenas[expected].store(new(arena_mem) arena_type(block_size),
					                       std::memory_order_release);
					current_arena.store(expected, std::memory_order_release);

					return arenas[expected].load(std::memory_order_acquire)->allocate();
				}
				return allocate();
			}
			else
			{
				if (num >= max_arenas)
					return nullptr;

#if defined(_WIN32) || defined(_WIN64)
				void *arena_mem = VirtualAlloc(nullptr, sizeof(arena_type),
				                               MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
				if (!arena_mem)
					return nullptr;

#else
				void *arena_mem = mmap(nullptr, sizeof(arena_type),
				                       PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				if (arena_mem == MAP_FAILED)
					return nullptr;

#endif

				arenas[num] = new(arena_mem) arena_type(block_size);
				current_arena = num;
				++num_arenas;

				return arenas[current_arena]->allocate();
			}
		}

		void deallocate(void *ptr) noexcept
		{
			std::size_t num;
			if constexpr (P == allocation_policy::shared)
				num = num_arenas.load(std::memory_order_acquire);
			else
				num = num_arenas;

			for (std::size_t i = 0; i < num; ++i)
			{
				arena_type *arena;
				if constexpr (P == allocation_policy::shared)
					arena = arenas[i].load(std::memory_order_acquire);
				else
					arena = arenas[i];

				if (arena && arena->owns(ptr))
				{
					arena->deallocate(ptr);
					return;
				}
			}
		}
	};

	template<typename T, allocation_policy P = allocation_policy::local>
	class allocator
	{
	public:
		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using propagate_on_container_move_assignment = std::true_type;
		using is_always_equal = std::true_type;

		template<typename U>
		struct rebind
		{
			using other = allocator<U, P>;
		};

	private:
		static constexpr std::size_t num_size_classes = detail::size_class_manager::num_size_classes;
		using pool_type = arena_pool<P>;

		static auto &get_pools() noexcept
		{
			if constexpr (P == allocation_policy::local)
			{
				thread_local pool_type *pools[num_size_classes] = {};
				thread_local bool initialized = false;

				if (!initialized)
				{
					for (std::size_t i = 0; i < num_size_classes; ++i)
					{
						std::size_t size_class = detail::size_class_manager::min_size << i;
#if defined(_WIN32) || defined(_WIN64)
						void *mem = VirtualAlloc(nullptr, sizeof(pool_type),
						                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
						if (mem)
							pools[i] = new(mem) pool_type(size_class);
#else
						void *mem = mmap(nullptr, sizeof(pool_type),
						                 PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
						if (mem != MAP_FAILED)
							pools[i] = new(mem) pool_type(size_class);
#endif
					}
					initialized = true;
				}
				return pools;
			}
			else
			{
				static pool_type *pools[num_size_classes] = {};
				static std::atomic<bool> initialized = { false };
				static std::atomic_flag init_lock = ATOMIC_FLAG_INIT;

				if (initialized.load(std::memory_order_acquire))
				{
					return pools;
				}

				while (init_lock.test_and_set(std::memory_order_acquire))
				{}

				if (!initialized.load(std::memory_order_relaxed))
				{
					for (std::size_t i = 0; i < num_size_classes; ++i)
					{
						std::size_t size_class = detail::size_class_manager::min_size << i;
#if defined(_WIN32) || defined(_WIN64)
						void *mem = VirtualAlloc(nullptr, sizeof(pool_type),
						                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
						if (mem)
							pools[i] = new(mem) pool_type(size_class);
#else
						void *mem = mmap(nullptr, sizeof(pool_type),
						                 PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
						if (mem != MAP_FAILED)
							pools[i] = new(mem) pool_type(size_class);
#endif
					}
					initialized.store(true, std::memory_order_release);
				}

				init_lock.clear(std::memory_order_release);
				return pools;
			}
		}

	public:
		allocator() noexcept = default;

		template<typename U>
		allocator(const allocator<U, P> &) noexcept {}

		[[nodiscard]] T *allocate(size_type n)
		{
			if (n == 0)
				return nullptr;

			std::size_t bytes = n * sizeof(T);
			bytes = (bytes + alignof(T) - 1) & ~(alignof(T) - 1);

			std::size_t size_class = detail::size_class_manager::round_to_size_class(bytes);

			if (size_class == 0 || size_class > detail::size_class_manager::max_size)
			{
#if defined(_WIN32) || defined(_WIN64)
				void *ptr = VirtualAlloc(nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
				if (!ptr)
					throw std::bad_alloc();
#else
				void *ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
				                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				if (ptr == MAP_FAILED)
					throw std::bad_alloc();
#endif
				return static_cast<T *>(ptr);
			}

			std::size_t index = detail::size_class_manager::size_to_index(size_class);
			auto &pools = get_pools();
			void *ptr = pools[index]->allocate();

			if (!ptr)
			{
				throw std::bad_alloc();
			}
			return static_cast<T *>(ptr);
		}

		void deallocate(T *ptr, size_type n) noexcept
		{
			if (!ptr || n == 0)
			{
				return;
			}

			std::size_t bytes = n * sizeof(T);
			bytes = (bytes + alignof(T) - 1) & ~(alignof(T) - 1);

			std::size_t size_class = detail::size_class_manager::round_to_size_class(bytes);
			if (size_class == 0 || size_class > detail::size_class_manager::max_size)
			{
#if defined(_WIN32) || defined(_WIN64)
				VirtualFree(ptr, 0, MEM_RELEASE);
#else
				munmap(ptr, bytes);
#endif
				return;
			}

			std::size_t index = detail::size_class_manager::size_to_index(size_class);
			auto &pools = get_pools();
			pools[index]->deallocate(ptr);
		}

		template<typename U>
		bool operator==(const allocator<U, P> &) const noexcept
		{
			return true;
		}

		template<typename U>
		bool operator!=(const allocator<U, P> &) const noexcept
		{
			return false;
		}
	};
}
