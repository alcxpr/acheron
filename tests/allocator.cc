/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#include <acheron/allocator.hpp>
#include <gtest/gtest.h>

TEST(AllocatorTest, Basic)
{
	ach::allocator<int> alloc;
	int *ptr = alloc.allocate(10);
	/* note: in rare cases, such as OOM, the allocator will crash */
	ASSERT_NE(ptr, nullptr);

	/* writes */
	for (int i = 0; i < 10; ++i)
		ptr[i] = i;
	for (int i = 0; i < 10; ++i)
		EXPECT_EQ(ptr[i], i);

	alloc.deallocate(ptr, 10);
}

TEST(AllocatorTest, ZeroAllocation)
{
	ach::allocator<int> alloc;
	int *ptr = alloc.allocate(0);
	EXPECT_EQ(ptr, nullptr);
	alloc.deallocate(ptr, 0);
}

TEST(AllocatorTest, SingleElementAllocation)
{
	ach::allocator<int> alloc;
	int *ptr = alloc.allocate(1);
	ASSERT_NE(ptr, nullptr);
	*ptr = 42;
	EXPECT_EQ(*ptr, 42);
	alloc.deallocate(ptr, 1);
}

TEST(AllocatorTest, LargeAllocation)
{
	ach::allocator<int> alloc;
	/* allocate more than max_size which is 4MB to test fallback to `mmap`/`VirtualAlloc` */
	constexpr std::size_t large_count = (5 * 1024 * 1024) / sizeof(int);
	int *ptr = alloc.allocate(large_count);
	ASSERT_NE(ptr, nullptr);

	ptr[0] = 1;
	ptr[large_count - 1] = 2;
	EXPECT_EQ(ptr[0], 1);
	EXPECT_EQ(ptr[large_count - 1], 2);

	alloc.deallocate(ptr, large_count);
}

TEST(AllocatorTest, PowerOfTwoSizes)
{
	ach::allocator<char> alloc;
	std::vector<std::pair<char *, std::size_t>> allocations;

	/* test power-of-2 sizes from 8 to 4096 bytes */
	for (std::size_t size = 8; size <= 4096; size *= 2)
	{
		char *ptr = alloc.allocate(size);
		ASSERT_NE(ptr, nullptr);
		std::memset(ptr, 0xAB, size);
		allocations.emplace_back(ptr, size);
	}

	for (const auto &[ptr, size]: allocations)
	{
		for (std::size_t i = 0; i < size; ++i)
			EXPECT_EQ(static_cast<unsigned char>(ptr[i]), 0xAB);
	}

	for (const auto &[ptr, size]: allocations)
		alloc.deallocate(ptr, size);
}

TEST(AllocatorTest, NonPowerOfTwoSizes)
{
	ach::allocator<char> alloc;

	/* test that non-power-of-2 sizes are rounded up correctly */
	std::vector<std::size_t> sizes = { 7, 15, 33, 65, 129, 257, 513, 1025 };
	std::vector<char *> ptrs;

	for (std::size_t size: sizes)
	{
		char *ptr = alloc.allocate(size);
		ASSERT_NE(ptr, nullptr);
		std::memset(ptr, 0xCD, size);
		ptrs.push_back(ptr);
	}

	for (std::size_t i = 0; i < sizes.size(); ++i)
		alloc.deallocate(ptrs[i], sizes[i]);
}

TEST(AllocatorTest, OveralignedType)
{
	struct alignas(64) overaligned
	{
		char data[64];
	};

	ach::allocator<overaligned> alloc;

	for (int i = 0; i < 10; ++i)
	{
		overaligned *ptr = alloc.allocate(1);
		ASSERT_NE(ptr, nullptr);
		EXPECT_EQ(reinterpret_cast<std::uintptr_t>(ptr) % 64, 0) << "overaligned allocation not properly aligned";
		alloc.deallocate(ptr, 1);
	}
}

TEST(AllocatorTest, ComplexTypes)
{
	struct complex_type
	{
		int a;
		double b;
		std::vector<int> c;

		complex_type() : a(0), b(0.0), c{ 1, 2, 3 }
		{}
		complex_type(int x, double y) : a(x), b(y), c{ 1, 2, 3 }
		{}
	};

	ach::allocator<complex_type> alloc;
	complex_type *ptr = alloc.allocate(5);
	ASSERT_NE(ptr, nullptr);

	for (int i = 0; i < 5; ++i)
		std::construct_at(&ptr[i], i, i * 1.5);

	for (int i = 0; i < 5; ++i)
	{
		EXPECT_EQ(ptr[i].a, i);
		EXPECT_DOUBLE_EQ(ptr[i].b, i * 1.5);
		EXPECT_EQ(ptr[i].c.size(), 3u);
	}

	for (int i = 0; i < 5; ++i)
		std::destroy_at(&ptr[i]);

	alloc.deallocate(ptr, 5);
}

TEST(AllocatorTest, VectorIntegration)
{
	std::vector<int, ach::allocator<int>> vec;

	for (int i = 0; i < 1000; ++i)
		vec.push_back(i);

	EXPECT_EQ(vec.size(), 1000u);
	for (int i = 0; i < 1000; ++i)
		EXPECT_EQ(vec[i], i);
}

TEST(AllocatorTest, StringIntegration)
{
	using string_type = std::basic_string<char, std::char_traits<char>, ach::allocator<char>>;

	string_type str;
	str.reserve(100);

	for (int i = 0; i < 100; ++i)
		str += 'a';

	EXPECT_EQ(str.size(), 100u);
	EXPECT_TRUE(std::all_of(str.begin(), str.end(), [](char c) { return c == 'a'; }));
}

TEST(AllocatorTest, MapIntegration)
{
	using map_type = std::map<int, std::string, std::less<>, ach::allocator<std::pair<const int, std::string>>>;

	map_type map;

	for (int i = 0; i < 100; ++i)
		map[i] = std::to_string(i);

	EXPECT_EQ(map.size(), 100u);
	for (int i = 0; i < 100; ++i)
		EXPECT_EQ(map[i], std::to_string(i));
}

TEST(AllocatorTest, SharedPolicyConcurrentAllocation)
{
	ach::allocator<int, ach::allocation_policy::shared> alloc;
	constexpr int num_threads = 8;
	constexpr int allocs_per_thread = 1000;

	std::vector<std::thread> threads;
	std::vector<std::vector<int *>> thread_ptrs(num_threads);

	for (int t = 0; t < num_threads; ++t)
	{
		threads.emplace_back(
				[&, t]()
				{
					for (int i = 0; i < allocs_per_thread; ++i)
					{
						int *ptr = alloc.allocate(10);
						ASSERT_NE(ptr, nullptr);
						for (int j = 0; j < 10; ++j)
							ptr[j] = t * 10000 + i * 10 + j;
						thread_ptrs[t].push_back(ptr);
					}
				});
	}

	for (auto &thread: threads)
		thread.join();

	for (int t = 0; t < num_threads; ++t)
	{
		for (int i = 0; i < allocs_per_thread; ++i)
		{
			int *ptr = thread_ptrs[t][i];
			for (int j = 0; j < 10; ++j)
				EXPECT_EQ(ptr[j], t * 10000 + i * 10 + j);
		}
	}

	for (int t = 0; t < num_threads; ++t)
	{
		for (int *ptr: thread_ptrs[t])
			alloc.deallocate(ptr, 10);
	}
}
