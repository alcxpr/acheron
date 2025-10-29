/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include "freelist.hpp"
#include "allocator.hpp"

namespace ach
{
    /**
     * @brief Slot state for unique_map entries.
     */
    enum class slot_state : std::uint8_t
    {
        empty,
        occupied,
        deleted
    };

    /**
     * @brief A specialized lookup table with stable key pointer semantics.
     *
     * unique_map is an open-addressed hash table that maintains stable pointers
     * to keys. Keys are never destroyed or moved after insertion (only reassigned
     * on slot reuse). Values may be moved during vector reallocation and are
     * destroyed on erase.
     *
     * @tparam Key Key type.
     * @tparam Value Mapped value type.
     * @tparam Hash Hash function object type.
     * @tparam KeyEqual Key equality comparison function object type.
     * @tparam Allocator Allocator type.
     */
    template<
        typename Key,
        typename Value,
        typename Hash = std::hash<Key>,
        typename KeyEqual = std::equal_to<Key>,
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
            key_type* first;
            mapped_type* second;
            bool inserted;
        };

    private:
        static constexpr size_type npos = static_cast<std::size_t>(-1);
        static constexpr size_type tombstone = static_cast<std::size_t>(-2);

        [[no_unique_address]] hasher hash_function;
        [[no_unique_address]] key_equal key_eq;
        [[no_unique_address]] allocator_type allocator;

        std::vector<std::size_t> buckets;
        std::vector<key_type*> key_ptrs;
        std::vector<mapped_type> values;
        std::vector<slot_state> states;
        freelist<key_type> key_storage;
        freelist<std::size_t> slot_freelist;

        std::size_t occupied_count = 0;

    public:
        /**
         * @brief Construct an empty unique_map with the specified bucket count.
         * @param bucket_count Initial number of buckets.
         * @param hash Hash function object.
         * @param equal Key equality function object.
         * @param alloc Allocator object.
         */
        explicit unique_map(
            std::size_t bucket_count = 16,
            const hasher& hash = hasher(),
            const key_equal& equal = key_equal(),
            const allocator_type& alloc = allocator_type())
            noexcept(std::is_nothrow_copy_constructible_v<hasher>
                    && std::is_nothrow_copy_constructible_v<key_equal>
                    && std::is_nothrow_copy_constructible_v<allocator_type>
                    && std::is_nothrow_constructible_v<std::vector<std::size_t>>
                    && std::is_nothrow_constructible_v<freelist<key_type>, std::size_t>
                    && std::is_nothrow_constructible_v<freelist<std::size_t>, std::size_t>)
            : hash_function(hash)
            , key_eq(equal)
            , allocator(alloc)
            , buckets(bucket_count, npos)
            , key_storage(0)
            , slot_freelist(0)
        {}

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
        [[nodiscard]] std::size_t size() const noexcept
        {
            return occupied_count;
        }

        /**
         * @brief Get the maximum possible number of elements.
         * @return Maximum size.
         */
        [[nodiscard]] std::size_t max_size() const noexcept
        {
            return std::size_t(-3);
        }

        /**
         * @brief Remove all elements from the map.
         */
        void clear() noexcept
        {
            if constexpr (!std::is_trivially_destructible_v<mapped_type>)
            {
                for (std::size_t i = 0; i < states.size(); ++i)
                {
                    if (states[i] == slot_state::occupied)
                        values[i].~mapped_type();
                }
            }

            buckets.clear();
            key_ptrs.clear();
            values.clear();
            states.clear();
            key_storage = freelist<key_type>(0);
            slot_freelist = freelist<std::size_t>(0);
            occupied_count = 0;
        }

        /**
         * @brief Emplace a key-value pair into the map.
         * @tparam Args Types of arguments to construct the value.
         * @param key Key to insert.
         * @param args Arguments forwarded to value constructor.
         * @return insert_return_type containing pointers to key and value, and insertion status.
         */
        template<typename... Args>
        insert_return_type emplace(const key_type& key, Args&&... args)
            noexcept(std::is_nothrow_copy_constructible_v<key_type>
                    && std::is_nothrow_constructible_v<mapped_type, Args...>
                    && std::is_nothrow_copy_assignable_v<key_type>)
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
        insert_return_type emplace(key_type&& key, Args&&... args)
            noexcept(std::is_nothrow_move_constructible_v<key_type>
                    && std::is_nothrow_constructible_v<mapped_type, Args...>
                    && std::is_nothrow_move_assignable_v<key_type>)
        {
            return emplace_impl(std::move(key), std::forward<Args>(args)...);
        }

        /**
         * @brief Erase an element by key.
         * @param key Key to erase.
         * @return Number of elements erased (0 or 1).
         */
        std::size_t erase(const key_type& key)
        {
            std::size_t h = find_bucket(key);
            if (h == npos)
                return 0;

            std::size_t idx = buckets[h];
            
            if constexpr (!std::is_trivially_destructible_v<mapped_type>)
                values[idx].~mapped_type();

            states[idx] = slot_state::deleted;
            buckets[h] = tombstone;
            slot_freelist.emplace(idx);
            --occupied_count;

            return 1;
        }

        /**
         * @brief Erase an element by key (heterogeneous lookup).
         * @tparam K Key type convertible to key_type.
         * @param x Key to erase.
         * @return Number of elements erased (0 or 1).
         */
        template<typename K>
        std::size_t erase(K&& x)
        {
            return erase(static_cast<const key_type&>(x));
        }

        /**
         * @brief Find an element by key.
         * @param key Key to search for.
         * @return Pointer to the mapped value, or nullptr if not found.
         */
        mapped_type* find(const key_type& key) noexcept
        {
            std::size_t h = find_bucket(key);
            if (h == npos)
                return nullptr;
            return &values[buckets[h]];
        }

        /**
         * @brief Find an element by key (const version).
         * @param key Key to search for.
         * @return Const pointer to the mapped value, or nullptr if not found.
         */
        const mapped_type* find(const key_type& key) const noexcept
        {
            std::size_t h = find_bucket(key);
            if (h == npos)
                return nullptr;
            return &values[buckets[h]];
        }

        /**
         * @brief Find an element by key (heterogeneous lookup).
         * @tparam K Key type convertible to key_type.
         * @param key Key to search for.
         * @return Pointer to the mapped value, or nullptr if not found.
         */
        template<typename K>
        mapped_type* find(const K& key) noexcept
            requires std::convertible_to<K, key_type>
        {
            return find(static_cast<const key_type&>(key));
        }

        /**
         * @brief Find an element by key (const heterogeneous lookup).
         * @tparam K Key type convertible to key_type.
         * @param key Key to search for.
         * @return Const pointer to the mapped value, or nullptr if not found.
         */
        template<typename K>
        const mapped_type* find(const K& key) const noexcept
            requires std::convertible_to<K, key_type>
        {
            return find(static_cast<const key_type&>(key));
        }

        /**
         * @brief Swap contents with another unique_map.
         * @param other Map to swap with.
         */
        void swap(unique_map& other) noexcept(
            std::allocator_traits<allocator_type>::is_always_equal::value
            && std::is_nothrow_swappable_v<hasher>
            && std::is_nothrow_swappable_v<key_equal>)
        {
            using std::swap;
            swap(hash_function, other.hash_function);
            swap(key_eq, other.key_eq);
            swap(allocator, other.allocator);
            swap(buckets, other.buckets);
            swap(key_ptrs, other.key_ptrs);
            swap(values, other.values);
            swap(states, other.states);
            swap(key_storage, other.key_storage);
            swap(slot_freelist, other.slot_freelist);
            swap(occupied_count, other.occupied_count);
        }

    private:
        std::size_t find_bucket(const key_type& key) const noexcept
        {
            if (buckets.empty())
                return npos;

            std::size_t h = hash_function(key) % buckets.size();
            while (buckets[h] != npos)
            {
                if (buckets[h] != tombstone)
                {
                    std::size_t idx = buckets[h];
                    if (states[idx] == slot_state::occupied && key_eq(*key_ptrs[idx], key))
                        return h;
                }
                h = (h + 1) % buckets.size();
            }
            return npos;
        }

        template<typename KeyArg, typename... Args>
        insert_return_type emplace_impl(KeyArg&& key, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<key_type, KeyArg>
                    && std::is_nothrow_constructible_v<mapped_type, Args...>
                    && std::is_nothrow_assignable_v<key_type&, KeyArg>)
        {
            if (should_rehash())
                rehash(buckets.size() * 2);

            std::size_t h = hash_function(key) % buckets.size();
            while (buckets[h] != npos && buckets[h] != tombstone)
            {
                std::size_t idx = buckets[h];
                if (states[idx] == slot_state::occupied && key_eq(*key_ptrs[idx], key))
                    return {key_ptrs[idx], &values[idx], false};
                h = (h + 1) % buckets.size();
            }

            std::size_t idx;
            key_type* key_ptr;
            
            if (!slot_freelist.empty())
            {
                std::size_t* idx_storage = slot_freelist.pop()->value();
                idx = *idx_storage;
                slot_freelist.push(reinterpret_cast<freelist<std::size_t>::node_type*>(idx_storage));
                
                key_ptr = key_ptrs[idx];
                *key_ptr = std::forward<KeyArg>(key);
                ::new (&values[idx]) mapped_type(std::forward<Args>(args)...);
            }
            else
            {
                idx = values.size();
                key_ptr = key_storage.emplace(std::forward<KeyArg>(key));
                key_ptrs.push_back(key_ptr);
                values.emplace_back(std::forward<Args>(args)...);
                states.push_back(slot_state::occupied);
            }

            states[idx] = slot_state::occupied;
            buckets[h] = idx;
            ++occupied_count;

            return {key_ptr, &values[idx], true};
        }

        [[nodiscard]] bool should_rehash() const noexcept
        {
            if (buckets.empty())
                return true;
            return static_cast<double>(occupied_count) > 0.75 * buckets.size();
        }

        void rehash(std::size_t new_bucket_count)
        {
            std::vector<std::size_t> new_buckets(new_bucket_count, npos);

            for (std::size_t i = 0; i < states.size(); ++i)
            {
                if (states[i] == slot_state::occupied)
                {
                    std::size_t h = hash_function(*key_ptrs[i]) % new_bucket_count;
                    while (new_buckets[h] != npos)
                        h = (h + 1) % new_bucket_count;
                    new_buckets[h] = i;
                }
            }

            buckets.swap(new_buckets);
        }
    };
}
