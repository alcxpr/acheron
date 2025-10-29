/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>
#include <algorithm>
#include <utility>
#include "allocator.hpp"

namespace ach
{
    /**
     * @brief Node structure used by freelist for both pointer and object types.
     *
     * For non-pointer types, embeds aligned storage for `T`.
     * For pointer types, acts as an intrusive node referring to its own address.
     */
    template<typename T, bool = std::is_pointer_v<T>>
    class freelist_node;

    template<typename T>
    class freelist_node<T, false>
    {
    public:
        alignas(T) std::byte storage[sizeof(T)];
        freelist_node* next = nullptr;

        T* value() noexcept
        {
            return std::launder(reinterpret_cast<T*>(storage));
        }

        const T* value() const noexcept
        {
            return std::launder(reinterpret_cast<const T*>(storage));
        }
    };

    template<typename T>
    class freelist_node<T, true>
    {
    public:
        freelist_node* next = nullptr;

        T value() noexcept
        {
            return reinterpret_cast<T>(this);
        }
    };

    /**
     * @brief A simple allocator-aware freelist for fast node reuse.
     *
     * This freelist allocates contiguous memory blocks for nodes,
     * chains them into a singly-linked list, and recycles released nodes.
     * Block sizes grow geometrically by 2x (64, 128, 256, ...).
     *
     * @tparam T Value or pointer type stored.
     * @tparam Allocator Standard allocator type.
     */
    template<typename T, typename Allocator = ach::allocator<freelist_node<T>>>
    class freelist
    {
    public:
        using value_type = T;
        using node_type = freelist_node<T>;
        using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<node_type>;
        using allocator_traits = std::allocator_traits<allocator_type>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;

    private:
        [[no_unique_address]] allocator_type allocator;
        std::vector<node_type*> blocks;
        node_type* head = nullptr;
        size_type in_use = 0;
        size_type total_capacity = 0;

    public:
        /**
         * @brief Construct a freelist with an initial capacity.
         * @param initial_capacity Number of nodes to pre-allocate (must be power of 2, default 64).
         */
        explicit freelist(size_type initial_capacity = 64)
        {
            if (initial_capacity > 0)
                allocate_block(initial_capacity);
        }

        /**
         * @brief Destructor. Destroys all in-use objects and deallocates all blocks.
         */
        ~freelist() noexcept(std::is_nothrow_destructible_v<T> || std::is_pointer_v<T>)
        {
            /* destroy in-use objects if non-trivial */
            if constexpr (!std::is_pointer_v<T> && !std::is_trivially_destructible_v<T>)
                destroy_lives();
            
            for (size_type i = 0; i < blocks.size(); ++i)
                allocator_traits::deallocate(allocator, blocks[i], block_capacity(i));
        }

        freelist(const freelist&) = delete;
        freelist& operator=(const freelist&) = delete;

        /**
         * @brief Move constructor.
         * @param other Freelist to move from.
         */
        freelist(freelist&& other) noexcept(
            std::is_nothrow_move_constructible_v<allocator_type>
            && std::is_nothrow_move_constructible_v<std::vector<node_type*>>)
            : allocator(std::move(other.allocator))
            , blocks(std::move(other.blocks))
            , head(std::exchange(other.head, nullptr))
            , in_use(std::exchange(other.in_use, 0))
            , total_capacity(std::exchange(other.total_capacity, 0))
        {
        }

        /**
         * @brief Move assignment operator.
         * @param other Freelist to move from.
         * @return Reference to this freelist.
         */
        freelist& operator=(freelist&& other) noexcept(
            std::is_nothrow_move_assignable_v<allocator_type>
            && std::is_nothrow_move_constructible_v<freelist>
            && std::is_nothrow_destructible_v<freelist>)
        {
            if (this != &other)
            {
                this->~freelist();
                new(this) freelist(std::move(other));
            }
            return *this;
        }

        /**
         * @brief Acquire a node from the freelist.
         * 
         * Allocates a new block if necessary. The returned node is uninitialized.
         * 
         * @return Pointer to an acquired node.
         * @throws May throw if allocation fails.
         */
        node_type* pop()
        {
            if (!head)
            {
                size_type new_capacity = blocks.empty() ? 64 : block_capacity(blocks.size() - 1) * 2;
                allocate_block(new_capacity);
            }
            
            node_type* node = head;
            head = head->next;
            ++in_use;
            return node;
        }

        /**
         * @brief Release a node back to the freelist.
         * 
         * The caller must have already destroyed the object if necessary.
         * 
         * @param node Node to release.
         */
        void push(node_type* node) noexcept
        {
            node->next = head;
            head = node;
            --in_use;
        }

        /**
         * @brief Construct an object in-place and return pointer to it.
         * 
         * Only available for non-pointer types.
         * 
         * @tparam Args Types of constructor arguments.
         * @param args Arguments forwarded to T's constructor.
         * @return Pointer to the constructed object.
         * @throws May throw if allocation or construction fails.
         */
        template<typename... Args>
        pointer emplace(Args&&... args) 
            noexcept(std::is_nothrow_constructible_v<T, Args...>)
            requires (!std::is_pointer_v<T>)
        {
            node_type* node = pop();
            return ::new (node->storage) T(std::forward<Args>(args)...);
        }

        /**
         * @brief Destroy an object and return its node to the freelist.
         * 
         * Only available for non-pointer types.
         * 
         * @param obj Pointer to object to destroy.
         */
        void destroy(pointer obj) noexcept(std::is_nothrow_destructible_v<T>)
            requires (!std::is_pointer_v<T>)
        {
            obj->~T();
            release(reinterpret_cast<node_type*>(obj));
        }

        /**
         * @brief Get the total capacity of the freelist.
         * @return Total number of nodes allocated.
         */
        [[nodiscard]] size_type capacity() const noexcept
        {
            return total_capacity;
        }

        /**
         * @brief Get the number of nodes currently in use.
         * @return Number of in-use nodes.
         */
        [[nodiscard]] size_type size() const noexcept
        {
            return in_use;
        }

        /**
         * @brief Get the number of available nodes.
         * @return Number of nodes available for acquisition.
         */
        [[nodiscard]] size_type available() const noexcept
        {
            return total_capacity - in_use;
        }

        /**
         * @brief Check if the freelist has no in-use objects.
         * @return True if no objects are in use, false otherwise.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return in_use == 0;
        }

        /**
         * @brief Get a copy of the allocator.
         * @return Copy of the allocator used by this freelist.
         */
        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return allocator;
        }

    private:
        [[nodiscard]] size_type block_capacity(size_type block_idx) const noexcept
        {
            return 64 << block_idx;  /* 64 * 2^block_idx */
        }

        void allocate_block(size_type count)
        {
            node_type* block = allocator_traits::allocate(allocator, count);
            
            /* chain all nodes: block[0] -> block[1] -> ... -> block[count-1] -> head */
            for (size_type i = 0; i < count - 1; ++i)
                block[i].next = &block[i + 1];
            
            block[count - 1].next = head;
            head = block;

            blocks.push_back(block);
            total_capacity += count;
        }

        void destroy_lives() noexcept(std::is_nothrow_destructible_v<T>)
        {
            /* build set of free nodes for quick lookup */
            std::vector<node_type*> free_nodes;
            for (node_type* cur = head; cur; cur = cur->next)
                free_nodes.push_back(cur);
            std::sort(free_nodes.begin(), free_nodes.end());

            /* destroy objects in blocks that aren't in free list */
            for (size_type block_idx = 0; block_idx < blocks.size(); ++block_idx)
            {
                node_type* block = blocks[block_idx];
                size_type capacity = block_capacity(block_idx);
                for (size_type i = 0; i < capacity; ++i)
                {
                    node_type* node = &block[i];
                    if (!std::binary_search(free_nodes.begin(), free_nodes.end(), node))
                        node->value()->~T();
                }
            }
        }
    };
}
