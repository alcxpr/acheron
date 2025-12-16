/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <utility>
#include "allocator.hpp"
#include "diagnostic.hpp"

namespace ach
{
	/**
	 * @brief A high-performance open-addressed hash table optimized for lookup speed.
	 * @tparam Key Key type (must be copyable or moveable).
	 * @tparam Value Mapped value type.
	 * @tparam Hash Hash function object type.
	 * @tparam KeyEqual Key equality comparison function object type.
	 * @tparam Allocator Allocator type.
	 */
	template<typename Key, typename Value, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>,
					 typename Allocator = ach::allocator<std::byte>>
	class unique_map
	{
	public:
		using key_type = Key;
		using mapped_type = Value;
		using hasher = Hash;
		using key_equal = KeyEqual;
		using allocator_type = Allocator;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;

		/**
		 * @brief Return type for emplace operations.
		 */
		struct insert_return_type
		{
			key_type *first;
			mapped_type *second;
			bool inserted;
		};

		/**
		 * @brief Iterator for traversing the map.
		 */
		class iterator
		{
			friend class unique_map;

		private:
			unique_map *map;
			size_type index;

			iterator(unique_map *m, size_type idx) noexcept : map(m), index(idx)
			{}

		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = std::pair<const key_type &, mapped_type &>;
			using difference_type = std::ptrdiff_t;
			using pointer = void;
			using reference = value_type;

			iterator() noexcept : map(nullptr), index(0)
			{}

			reference operator*() const noexcept
			{
				return { map->keys[index], map->values[index] };
			}

			iterator &operator++() noexcept
			{
				do
				{
					++index;
				}
				while (index < map->capacity && !map->is_occupied(index));
				return *this;
			}

			iterator operator++(int) noexcept
			{
				iterator tmp = *this;
				++(*this);
				return tmp;
			}

			bool operator==(const iterator &other) const noexcept
			{
				return map == other.map && index == other.index;
			}

			bool operator!=(const iterator &other) const noexcept
			{
				return !(*this == other);
			}
		};

		/**
		 * @brief Const iterator for traversing the map.
		 */
		class const_iterator
		{
			friend class unique_map;

		private:
			const unique_map *map;
			size_type index;

			const_iterator(const unique_map *m, size_type idx) noexcept : map(m), index(idx)
			{}

		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = std::pair<const key_type &, const mapped_type &>;
			using difference_type = std::ptrdiff_t;
			using pointer = void;
			using reference = value_type;

			const_iterator() noexcept : map(nullptr), index(0)
			{}
			const_iterator(const iterator &it) noexcept : map(it.map), index(it.index)
			{}

			reference operator*() const noexcept
			{
				return { map->keys[index], map->values[index] };
			}

			const_iterator &operator++() noexcept
			{
				do
				{
					++index;
				}
				while (index < map->capacity && !map->is_occupied(index));
				return *this;
			}

			const_iterator operator++(int) noexcept
			{
				const_iterator tmp = *this;
				++(*this);
				return tmp;
			}

			bool operator==(const const_iterator &other) const noexcept
			{
				return map == other.map && index == other.index;
			}

			bool operator!=(const const_iterator &other) const noexcept
			{
				return !(*this == other);
			}
		};

	private:
		/**
		 * @brief Metadata byte layout: [occupied:1 | psl:7]
		 *
		 * Keeping metadata separate from key/value data allows scanning
		 * metadata without cache misses on the actual data.
		 */
		static constexpr std::uint8_t EMPTY = 0x00;
		static constexpr std::uint8_t OCCUPIED_BIT = 0x80;
		static constexpr std::uint8_t PSL_MASK = 0x7F;

		static constexpr size_type default_capacity = 16;
		static constexpr size_type max_psl = 127;
		static constexpr double max_load_factor = 0.875;

		[[no_unique_address]] hasher hash_function;
		[[no_unique_address]] key_equal key_eq;
		[[no_unique_address]] allocator_type allocator;

		std::uint8_t *metadata; /* hot: scanned on every lookup */
		key_type *keys;					/* cold: accessed only on metadata match */
		mapped_type *values;		/* cold: accessed only on metadata match */
		size_type capacity;
		size_type occupied_count;

		/**
		 * @brief Check if a slot is occupied.
		 */
		bool is_occupied(size_type idx) const noexcept
		{
			return metadata[idx] & OCCUPIED_BIT;
		}

		/**
		 * @brief Get PSL value from metadata.
		 */
		std::uint8_t get_psl(size_type idx) const noexcept
		{
			return metadata[idx] & PSL_MASK;
		}

		/**
		 * @brief Set metadata to occupied with given PSL.
		 */
		void set_occupied(size_type idx, std::uint8_t psl) noexcept
		{
			metadata[idx] = OCCUPIED_BIT | (psl & PSL_MASK);
		}

		/**
		 * @brief Mark slot as empty.
		 */
		void set_empty(size_type idx) noexcept
		{
			metadata[idx] = EMPTY;
		}

		/**
		 * @brief Round up to the next power of 2.
		 */
		static constexpr size_type next_power_of_2(size_type n) noexcept
		{
			if (n <= default_capacity)
				return default_capacity;
			if (std::has_single_bit(n))
				return n;
			return std::bit_ceil(n);
		}

		/**
		 * @brief Allocate metadata, keys, and values arrays.
		 */
		void allocate_arrays(size_type count)
		{
			/* allocate metadata array (hot path) */
			metadata = reinterpret_cast<std::uint8_t *>(allocator.allocate(count * sizeof(std::uint8_t)));
			std::memset(metadata, EMPTY, count * sizeof(std::uint8_t));

			/* allocate keys array */
			keys = reinterpret_cast<key_type *>(allocator.allocate(count * sizeof(key_type)));

			/* allocate values array */
			values = reinterpret_cast<mapped_type *>(allocator.allocate(count * sizeof(mapped_type)));
		}

		/**
		 * @brief Deallocate all arrays.
		 */
		void deallocate_arrays() noexcept
		{
			if (metadata)
			{
				/* destroy occupied keys and values */
				for (size_type i = 0; i < capacity; ++i)
				{
					if (is_occupied(i))
					{
						if constexpr (!std::is_trivially_destructible_v<key_type>)
							keys[i].~key_type();
						if constexpr (!std::is_trivially_destructible_v<mapped_type>)
							values[i].~mapped_type();
					}
				}

				allocator.deallocate(reinterpret_cast<std::byte *>(metadata), capacity * sizeof(std::uint8_t));
				allocator.deallocate(reinterpret_cast<std::byte *>(keys), capacity * sizeof(key_type));
				allocator.deallocate(reinterpret_cast<std::byte *>(values), capacity * sizeof(mapped_type));

				metadata = nullptr;
				keys = nullptr;
				values = nullptr;
			}
		}

		/**
		 * @brief Find the slot index for a key, or capacity if not found.
		 *
		 * Uses linear probing with Robin Hood early termination.
		 * Linear probing has better cache behavior than quadratic despite clustering.
		 */
		size_type find_index(const key_type &key) const noexcept
		{
			if (capacity == 0)
				return 0;

			size_type mask = capacity - 1;
			size_type idx = hash_function(key) & mask;
			std::uint8_t dist = 0;

			/* linear probing: scan metadata without touching keys/values */
			while (dist <= max_psl)
			{
				/* empty slot means key not found */
				if (!is_occupied(idx))
					return capacity;

				/* Robin Hood invariant: if we've probed further than the slot's PSL,
				 * the key would have been placed earlier */
				if (get_psl(idx) < dist)
					return capacity;

				/* metadata matches, check actual key (cold path) */
				if (key_eq(keys[idx], key))
					return idx;

				idx = (idx + 1) & mask;
				++dist;
			}

			return capacity;
		}

		/**
		 * @brief Insert a key-value pair using move semantics (for rehashing).
		 *
		 * Precondition: capacity must be sufficient to insert the element without exceeding max_load_factor.
		 * This is guaranteed by reserve() which checks capacity before beginning rehashing.
		 */
		void insert_slot_move(key_type &&key, mapped_type &&value)
		{
			size_type mask = capacity - 1;
			size_type idx = hash_function(key) & mask;
			std::uint8_t psl_val = 0;

			key_type k = std::move(key);
			mapped_type v = std::move(value);

			while (psl_val <= max_psl)
			{
				/* empty slot, insert here */
				if (!is_occupied(idx))
				{
					::new (static_cast<void *>(&keys[idx])) key_type(std::move(k));
					::new (static_cast<void *>(&values[idx])) mapped_type(std::move(v));
					set_occupied(idx, psl_val);
					return;
				}

				/* Robin Hood: steal from the rich (swap with longer PSL) */
				std::uint8_t slot_psl = get_psl(idx);
				if (slot_psl < psl_val)
				{
					using std::swap;
					swap(k, keys[idx]);
					swap(v, values[idx]);
					swap(psl_val, slot_psl);
					set_occupied(idx, slot_psl);
				}

				idx = (idx + 1) & mask;
				++psl_val;
			}

			ACH_ASSERT(psl_val > max_psl, "PSL exceeds maximum allowed. Your hash function is very shit.");
		}

		/**
		 * @brief Emplace implementation.
		 */
		template<typename KeyArg, typename... Args>
		insert_return_type emplace_impl(KeyArg &&key_arg, Args &&...args)
		{
			/* check if we need to grow */
			if (capacity == 0 || static_cast<double>(occupied_count + 1) > max_load_factor * capacity)
				reserve(capacity == 0 ? default_capacity : capacity * 2);

			size_type existing_idx = find_index(key_arg);
			if (existing_idx != capacity)
				return { &keys[existing_idx], &values[existing_idx], false };

			size_type mask = capacity - 1;
			size_type idx = hash_function(key_arg) & mask;
			std::uint8_t psl_val = 0;

			key_type k(std::forward<KeyArg>(key_arg));
			mapped_type v(std::forward<Args>(args)...);
			bool inserting = true;

			while (psl_val <= max_psl)
			{
				/* empty slot, insert here */
				if (!is_occupied(idx))
				{
					::new (static_cast<void *>(&keys[idx])) key_type(std::move(k));
					::new (static_cast<void *>(&values[idx])) mapped_type(std::move(v));
					set_occupied(idx, psl_val);
					++occupied_count;
					return { &keys[idx], &values[idx], true };
				}

				/* check if key already exists */
				if (inserting && key_eq(keys[idx], k))
					return { &keys[idx], &values[idx], false };

				/* Robin Hood: steal from the rich */
				std::uint8_t slot_psl = get_psl(idx);
				if (inserting && slot_psl < psl_val)
				{
					using std::swap;
					swap(k, keys[idx]);
					swap(v, values[idx]);
					swap(psl_val, slot_psl);
					set_occupied(idx, slot_psl);
					inserting = false; /* now we're displacing an existing entry */
				}

				idx = (idx + 1) & mask;
				++psl_val;
			}

			/* PSL exceeded maximum, need to grow */
			reserve(capacity * 2);
			return emplace_impl(std::move(k), std::move(v));
		}

		template<typename K>
		size_type find_index(const K &key) const noexcept
		{
			if (capacity == 0)
				return 0;

			size_type mask = capacity - 1;
			size_type idx = hash_function(key) & mask;
			std::uint8_t dist = 0;

			while (dist <= max_psl)
			{
				if (!is_occupied(idx))
					return capacity;

				if (get_psl(idx) < dist)
					return capacity;

				if (key_eq(keys[idx], key))
					return idx;

				idx = (idx + 1) & mask;
				++dist;
			}

			return capacity;
		}

		/**
		 * @brief Erase element at index using backward shift deletion.
		 *
		 * Backward shift maintains Robin Hood invariants and eliminates tombstones.
		 */
		void erase_at(size_type idx)
		{
			if constexpr (!std::is_trivially_destructible_v<key_type>)
				keys[idx].~key_type();
			if constexpr (!std::is_trivially_destructible_v<mapped_type>)
				values[idx].~mapped_type();

			set_empty(idx);
			--occupied_count;

			/* backward shift deletion */
			size_type mask = capacity - 1;
			size_type curr = idx;
			size_type next = (curr + 1) & mask;

			while (is_occupied(next))
			{
				std::uint8_t next_psl = get_psl(next);
				if (next_psl == 0)
					break; /* can't shift an entry at its ideal position */

				/* move next slot backward */
				::new (static_cast<void *>(&keys[curr])) key_type(std::move(keys[next]));
				::new (static_cast<void *>(&values[curr])) mapped_type(std::move(values[next]));
				set_occupied(curr, next_psl - 1);

				if constexpr (!std::is_trivially_destructible_v<key_type>)
					keys[next].~key_type();
				if constexpr (!std::is_trivially_destructible_v<mapped_type>)
					values[next].~mapped_type();

				set_empty(next);

				curr = next;
				next = (curr + 1) & mask;
			}
		}

	public:
		/**
		 * @brief Construct an empty unique_map with the specified capacity.
		 * @param initial_capacity Initial number of slots (will be rounded up to power of 2).
		 * @param hash Hash function object.
		 * @param equal Key equality function object.
		 * @param alloc Allocator object.
		 */
		explicit unique_map(size_type initial_capacity = default_capacity, const hasher &hash = hasher(),
												const key_equal &equal = key_equal(),
												const allocator_type &alloc =
																allocator_type()) noexcept(std::is_nothrow_copy_constructible_v<hasher> &&
																													 std::is_nothrow_copy_constructible_v<key_equal> &&
																													 std::is_nothrow_copy_constructible_v<allocator_type>) :
				hash_function(hash), key_eq(equal), allocator(alloc), metadata(nullptr), keys(nullptr), values(nullptr),
				capacity(0), occupied_count(0)
		{
			reserve(initial_capacity);
		}

		/**
		 * @brief Copy constructor.
		 */
		unique_map(const unique_map &other) :
				hash_function(other.hash_function), key_eq(other.key_eq), allocator(other.allocator), metadata(nullptr),
				keys(nullptr), values(nullptr), capacity(0), occupied_count(0)
		{
			if (other.capacity > 0)
			{
				reserve(other.capacity);
				for (size_type i = 0; i < other.capacity; ++i)
				{
					if (other.is_occupied(i))
					{
						::new (static_cast<void *>(&keys[i])) key_type(other.keys[i]);
						::new (static_cast<void *>(&values[i])) mapped_type(other.values[i]);
						metadata[i] = other.metadata[i];
						++occupied_count;
					}
				}
			}
		}

		/**
		 * @brief Move constructor.
		 */
		unique_map(unique_map &&other) noexcept :
				hash_function(std::move(other.hash_function)), key_eq(std::move(other.key_eq)),
				allocator(std::move(other.allocator)), metadata(other.metadata), keys(other.keys), values(other.values),
				capacity(other.capacity), occupied_count(other.occupied_count)
		{
			other.metadata = nullptr;
			other.keys = nullptr;
			other.values = nullptr;
			other.capacity = 0;
			other.occupied_count = 0;
		}

		/**
		 * @brief Destructor.
		 */
		~unique_map()
		{
			deallocate_arrays();
		}

		/**
		 * @brief Copy assignment operator.
		 */
		unique_map &operator=(const unique_map &other)
		{
			if (this != &other)
			{
				unique_map tmp(other);
				swap(tmp);
			}
			return *this;
		}

		/**
		 * @brief Move assignment operator.
		 */
		unique_map &operator=(unique_map &&other) noexcept
		{
			if (this != &other)
			{
				deallocate_arrays();

				hash_function = std::move(other.hash_function);
				key_eq = std::move(other.key_eq);
				allocator = std::move(other.allocator);
				metadata = other.metadata;
				keys = other.keys;
				values = other.values;
				capacity = other.capacity;
				occupied_count = other.occupied_count;

				other.metadata = nullptr;
				other.keys = nullptr;
				other.values = nullptr;
				other.capacity = 0;
				other.occupied_count = 0;
			}
			return *this;
		}

		/**
		 * @brief Check if the map is empty.
		 * @return True if the map contains no elements, false otherwise.
		 */
		[[nodiscard]] bool empty() const noexcept
		{
			return occupied_count == 0;
		}

		/**
		 * @brief Get the number of elements in the map.
		 * @return Number of occupied slots.
		 */
		[[nodiscard]] size_type size() const noexcept
		{
			return occupied_count;
		}

		/**
		 * @brief Get the maximum possible number of elements.
		 * @return Maximum size.
		 */
		[[nodiscard]] size_type max_size() const noexcept
		{
			return size_type(-1) / (sizeof(std::uint8_t) + sizeof(key_type) + sizeof(mapped_type));
		}

		/**
		 * @brief Get the current capacity (number of slots).
		 * @return Current capacity.
		 */
		[[nodiscard]] size_type bucket_count() const noexcept
		{
			return capacity;
		}

		/**
		 * @brief Get the current load factor.
		 * @return Ratio of occupied slots to total capacity.
		 */
		[[nodiscard]] double load_factor() const noexcept
		{
			return capacity > 0 ? static_cast<double>(occupied_count) / capacity : 0.0;
		}

		/**
		 * @brief Remove all elements from the map.
		 */
		void clear() noexcept
		{
			if (metadata)
			{
				for (size_type i = 0; i < capacity; ++i)
				{
					if (is_occupied(i))
					{
						if constexpr (!std::is_trivially_destructible_v<key_type>)
							keys[i].~key_type();
						if constexpr (!std::is_trivially_destructible_v<mapped_type>)
							values[i].~mapped_type();
						set_empty(i);
					}
				}
			}
			occupied_count = 0;
		}

		/**
		 * @brief Reserve space for at least the specified number of elements.
		 * @param new_capacity Minimum number of slots to allocate.
		 */
		void reserve(size_type new_capacity)
		{
			new_capacity = next_power_of_2(new_capacity);
			if (new_capacity <= capacity)
				return;

			std::uint8_t *old_metadata = metadata;
			key_type *old_keys = keys;
			mapped_type *old_values = values;
			size_type old_capacity = capacity;
			size_type old_occupied = occupied_count;

			allocate_arrays(new_capacity);
			capacity = new_capacity;
			occupied_count = 0; /* reset count, will be rebuilt during rehashing */

			if (old_metadata)
			{
				/* reserve enough capacity for all existing elements to avoid recursive growth */
				if (static_cast<double>(old_occupied) > max_load_factor * new_capacity)
				{
					/* this should never happen if reserve() was called with correct size,
					 * but handle it defensively */
					allocator.deallocate(reinterpret_cast<std::byte *>(metadata), new_capacity * sizeof(std::uint8_t));
					allocator.deallocate(reinterpret_cast<std::byte *>(keys), new_capacity * sizeof(key_type));
					allocator.deallocate(reinterpret_cast<std::byte *>(values), new_capacity * sizeof(mapped_type));

					reserve(new_capacity * 2);
					/* recursive call with doubled capacity - will eventually succeed */
					old_metadata = metadata;
					old_keys = keys;
					old_values = values;
					old_capacity = capacity;
					old_occupied = occupied_count;
					occupied_count = 0;
				}

				for (size_type i = 0; i < old_capacity; ++i)
				{
					if (old_metadata[i] & OCCUPIED_BIT)
					{
						insert_slot_move(std::move(old_keys[i]), std::move(old_values[i]));
						++occupied_count;

						if constexpr (!std::is_trivially_destructible_v<key_type>)
							old_keys[i].~key_type();
						if constexpr (!std::is_trivially_destructible_v<mapped_type>)
							old_values[i].~mapped_type();
					}
				}

				allocator.deallocate(reinterpret_cast<std::byte *>(old_metadata), old_capacity * sizeof(std::uint8_t));
				allocator.deallocate(reinterpret_cast<std::byte *>(old_keys), old_capacity * sizeof(key_type));
				allocator.deallocate(reinterpret_cast<std::byte *>(old_values), old_capacity * sizeof(mapped_type));
			}
		}

		/**
		 * @brief Emplace a key-value pair into the map.
		 * @tparam Args Types of arguments to construct the value.
		 * @param key Key to insert.
		 * @param args Arguments forwarded to value constructor.
		 * @return insert_return_type containing pointers to key and value, and insertion status.
		 */
		template<typename... Args>
		insert_return_type emplace(const key_type &key, Args &&...args)
		{
			return emplace_impl(key, std::forward<Args>(args)...);
		}

		/**
		 * @brief Emplace a key-value pair into the map (rvalue key).
		 * @tparam Args Types of arguments to construct the value.
		 * @param key Key to insert (moved).
		 * @param args Arguments forwarded to value constructor.
		 * @return insert_return_type containing pointers to key and value, and insertion status.
		 */
		template<typename... Args>
		insert_return_type emplace(key_type &&key, Args &&...args)
		{
			return emplace_impl(std::move(key), std::forward<Args>(args)...);
		}

		/**
		 * @brief Emplace a key-value pair into the map (heterogeneous key support).
		 * @tparam KeyArg Type convertible to key_type.
		 * @tparam Args Types of arguments to construct the value.
		 * @param key_arg Key-like argument to insert.
		 * @param args Arguments forwarded to value constructor.
		 * @return insert_return_type containing pointers to key and value, and insertion status.
		 */
		template<typename KeyArg, typename... Args>
			requires(!std::same_as<std::remove_cvref_t<KeyArg>, key_type>)
		insert_return_type emplace(KeyArg &&key_arg, Args &&...args)
		{
			return emplace_impl(std::forward<KeyArg>(key_arg), std::forward<Args>(args)...);
		}

		/**
		 * @brief Insert a key-value pair into the map.
		 * @param key Key to insert.
		 * @param value Value to insert.
		 * @return insert_return_type containing pointers to key and value, and insertion status.
		 */
		insert_return_type insert(const key_type &key, const mapped_type &value)
		{
			return emplace(key, value);
		}

		/**
		 * @brief Insert a key-value pair into the map (move semantics).
		 * @param key Key to insert.
		 * @param value Value to insert (moved).
		 * @return insert_return_type containing pointers to key and value, and insertion status.
		 */
		insert_return_type insert(const key_type &key, mapped_type &&value)
		{
			return emplace(key, std::move(value));
		}

		/**
		 * @brief Insert a key-value pair into the map (heterogeneous key).
		 * @param key Key-like object to insert.
		 * @param value Value to insert.
		 * @return insert_return_type containing pointers to key and value, and insertion status.
		 */
		template<typename K>
			requires(!std::same_as<std::remove_cvref_t<K>, key_type>)
		insert_return_type insert(K &&key, const mapped_type &value)
		{
			return emplace(std::forward<K>(key), value);
		}

		/**
		 * @brief Insert a key-value pair into the map.
		 * @param key Key-like object to insert.
		 * @param value Value to insert (moved).
		 * @return insert_return_type containing pointers to key and value, and insertion status.
		 */
		template<typename K>
			requires(!std::same_as<std::remove_cvref_t<K>, key_type>)
		insert_return_type insert(K &&key, mapped_type &&value)
		{
			return emplace(std::forward<K>(key), std::move(value));
		}

		/**
		 * @brief Erase an element by key.
		 * @param key Key to erase.
		 * @return Number of elements erased (0 or 1).
		 */
		size_type erase(const key_type &key)
		{
			size_type idx = find_index(key);
			if (idx == capacity)
				return 0;

			erase_at(idx);
			return 1;
		}

		/**
		 * @brief Erase an element by key (heterogeneous).
		 * @param key Key-like object to erase.
		 * @return Number of elements erased (0 or 1).
		 */
		template<typename K>
		size_type erase(const K &key)
		{
			size_type idx = find_index(key);
			if (idx == capacity)
				return 0;

			erase_at(idx);
			return 1;
		}

		/**
		 * @brief Find an element by key.
		 * @param key Key to search for.
		 * @return Pointer to the mapped value, or nullptr if not found.
		 */
		mapped_type *find(const key_type &key) noexcept
		{
			size_type idx = find_index(key);
			return idx != capacity ? &values[idx] : nullptr;
		}

		/**
		 * @brief Find an element by key (const version).
		 * @param key Key to search for.
		 * @return Const pointer to the mapped value, or nullptr if not found.
		 */
		const mapped_type *find(const key_type &key) const noexcept
		{
			size_type idx = find_index(key);
			return idx != capacity ? &values[idx] : nullptr;
		}

		/**
		 * @brief Find an element by key (heterogeneous).
		 * @param key Key-like object to search for.
		 * @return Pointer to the mapped value, or nullptr if not found.
		 */
		template<typename K>
		mapped_type *find(const K &key) noexcept
		{
			size_type idx = find_index(key);
			return idx != capacity ? &values[idx] : nullptr;
		}

		/**
		 * @brief Find an element by key (heterogeneous, const version).
		 * @param key Key-like object to search for.
		 * @return Const pointer to the mapped value, or nullptr if not found.
		 */
		template<typename K>
		const mapped_type *find(const K &key) const noexcept
		{
			size_type idx = find_index(key);
			return idx != capacity ? &values[idx] : nullptr;
		}

		/**
		 * @brief Get reference to value, inserting default if not present.
		 * @param key Key to find or insert.
		 * @return Reference to the mapped value.
		 */
		mapped_type &operator[](const key_type &key)
		{
			auto result = emplace(key);
			return *result.second;
		}

		/**
		 * @brief Get reference to value, inserting default if not present (rvalue key).
		 * @param key Key to find or insert (moved if new).
		 * @return Reference to the mapped value.
		 */
		mapped_type &operator[](key_type &&key)
		{
			auto result = emplace(std::move(key));
			return *result.second;
		}

		/**
		 * @brief Get reference to value, inserting default if not present (heterogeneous).
		 * @param key Key-like object to find or insert.
		 * @return Reference to the mapped value.
		 */
		template<typename K>
			requires(!std::same_as<std::remove_cvref_t<K>, key_type>)
		mapped_type &operator[](K &&key)
		{
			auto result = emplace(std::forward<K>(key));
			return *result.second;
		}

		/**
		 * @brief Check if a key exists in the map.
		 * @param key Key to search for.
		 * @return True if key exists, false otherwise.
		 */
		bool contains(const key_type &key) const noexcept
		{
			return find_index(key) != capacity;
		}

		/**
		 * @brief Check if a key exists in the map.
		 * @param key Key-like object to search for.
		 * @return True if key exists, false otherwise.
		 */
		template<typename K>
		bool contains(const K &key) const noexcept
		{
			return find_index(key) != capacity;
		}

		/**
		 * @brief Get iterator to the beginning.
		 * @return Iterator to the first element.
		 */
		iterator begin() noexcept
		{
			size_type idx = 0;
			while (idx < capacity && !is_occupied(idx))
				++idx;
			return iterator(this, idx);
		}

		/**
		 * @brief Get const iterator to the beginning.
		 * @return Const iterator to the first element.
		 */
		const_iterator begin() const noexcept
		{
			size_type idx = 0;
			while (idx < capacity && !is_occupied(idx))
				++idx;
			return const_iterator(this, idx);
		}

		/**
		 * @brief Get const iterator to the beginning.
		 * @return Const iterator to the first element.
		 */
		const_iterator cbegin() const noexcept
		{
			return begin();
		}

		/**
		 * @brief Get iterator to the end.
		 * @return Iterator to one past the last element.
		 */
		iterator end() noexcept
		{
			return iterator(this, capacity);
		}

		/**
		 * @brief Get const iterator to the end.
		 * @return Const iterator to one past the last element.
		 */
		const_iterator end() const noexcept
		{
			return const_iterator(this, capacity);
		}

		/**
		 * @brief Get const iterator to the end.
		 * @return Const iterator to one past the last element.
		 */
		const_iterator cend() const noexcept
		{
			return end();
		}

		/**
		 * @brief Swap contents with another unique_map.
		 * @param other Map to swap with.
		 */
		void swap(unique_map &other) noexcept
		{
			using std::swap;
			swap(hash_function, other.hash_function);
			swap(key_eq, other.key_eq);
			swap(allocator, other.allocator);
			swap(metadata, other.metadata);
			swap(keys, other.keys);
			swap(values, other.values);
			swap(capacity, other.capacity);
			swap(occupied_count, other.occupied_count);
		}
	};

	/**
	 * @brief Swap two unique_maps.
	 */
	template<typename Key, typename Value, typename Hash, typename KeyEqual, typename Allocator>
	void swap(unique_map<Key, Value, Hash, KeyEqual, Allocator> &lhs,
						unique_map<Key, Value, Hash, KeyEqual, Allocator> &rhs) noexcept
	{
		lhs.swap(rhs);
	}

} /* namespace ach */
