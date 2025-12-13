/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#include <acheron/algorithm.hpp>
#include <gtest/gtest.h>

#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <string>
#include <vector>

TEST(AlgorithmTest, EnumerateVector)
{
	std::vector<int> vec = { 10, 20, 30, 40, 50 };
	std::vector<std::pair<std::size_t, int>> results;

	ach::enumerate(vec.begin(), vec.end(), [&](std::size_t i, int value) { results.emplace_back(i, value); });

	ASSERT_EQ(results.size(), 5u);
	for (std::size_t i = 0; i < 5; ++i)
	{
		EXPECT_EQ(results[i].first, i);
		EXPECT_EQ(results[i].second, vec[i]);
	}
}

TEST(AlgorithmTest, EnumerateRanges)
{
	std::vector<std::string> vec = { "a", "b", "c" };
	std::size_t count = 0;

	ach::ranges::enumerate(vec,
												 [&](std::size_t i, const std::string &s)
												 {
													 EXPECT_EQ(i, count);
													 EXPECT_EQ(s, vec[i]);
													 ++count;
												 });

	EXPECT_EQ(count, 3u);
}

TEST(AlgorithmTest, EnumerateEmpty)
{
	std::vector<int> vec;
	bool called = false;

	ach::ranges::enumerate(vec, [&](std::size_t, int) { called = true; });

	EXPECT_FALSE(called);
}

TEST(AlgorithmTest, EnumerateMutation)
{
	std::vector<int> vec = { 1, 2, 3, 4, 5 };

	ach::ranges::enumerate(vec, [](std::size_t i, int &value) { value = static_cast<int>(i * 10); });

	for (std::size_t i = 0; i < vec.size(); ++i)
		EXPECT_EQ(vec[i], static_cast<int>(i * 10));
}

TEST(AlgorithmTest, EnumerateArray)
{
	std::array<int, 4> arr = { 100, 200, 300, 400 };
	std::size_t sum_indices = 0;
	int sum_values = 0;

	ach::ranges::enumerate(arr,
												 [&](std::size_t i, int value)
												 {
													 sum_indices += i;
													 sum_values += value;
												 });

	EXPECT_EQ(sum_indices, 0u + 1u + 2u + 3u);
	EXPECT_EQ(sum_values, 1000);
}

TEST(AlgorithmTest, HashCombineBasic)
{
	std::vector<int> vec1 = { 1, 2, 3, 4, 5 };
	std::vector<int> vec2 = { 1, 2, 3, 4, 5 };
	std::vector<int> vec3 = { 5, 4, 3, 2, 1 };

	auto hash1 = ach::hash_combine(vec1.begin(), vec1.end(), [](int x) { return x; });
	auto hash2 = ach::hash_combine(vec2.begin(), vec2.end(), [](int x) { return x; });
	auto hash3 = ach::hash_combine(vec3.begin(), vec3.end(), [](int x) { return x; });

	EXPECT_EQ(hash1, hash2);
	EXPECT_NE(hash1, hash3);
}

TEST(AlgorithmTest, HashCombineRanges)
{
	std::vector<std::string> vec = { "hello", "world", "test" };

	auto hash = ach::ranges::hash_combine(vec, [](const std::string &s) { return s; });

	EXPECT_NE(hash, 0u);
}

TEST(AlgorithmTest, HashCombineEmpty)
{
	std::vector<int> vec;

	auto hash = ach::ranges::hash_combine(vec, [](int x) { return x; });
	EXPECT_EQ(hash, 0xcbf29ce484222325ULL);
}

TEST(AlgorithmTest, HashCombineTransform)
{
	std::vector<int> vec = { 1, 2, 3 };

	auto hash1 = ach::ranges::hash_combine(vec, [](int x) { return x; });
	auto hash2 = ach::ranges::hash_combine(vec, [](int x) { return x * 2; });

	EXPECT_NE(hash1, hash2);
}

TEST(AlgorithmTest, HashCombineConsistency)
{
	std::vector<int> vec = { 42, 43, 44 };

	auto hash1 = ach::ranges::hash_combine(vec, [](int x) { return x; });
	auto hash2 = ach::ranges::hash_combine(vec, [](int x) { return x; });

	EXPECT_EQ(hash1, hash2);
}

TEST(AlgorithmTest, HashCombineComplex)
{
	struct data
	{
		int id;
		std::string name;
	};

	std::vector<data> vec = { { 1, "alice" }, { 2, "bob" }, { 3, "charlie" } };

	auto hash = ach::ranges::hash_combine(vec, [](const data &d) { return d.id; });

	EXPECT_NE(hash, 0u);
}

TEST(AlgorithmTest, ContainsFound)
{
	std::vector<int> vec = { 1, 2, 3, 4, 5 };

	EXPECT_TRUE(ach::contains(vec.begin(), vec.end(), 3));
	EXPECT_TRUE(ach::ranges::contains(vec, 1));
	EXPECT_TRUE(ach::ranges::contains(vec, 5));
}

TEST(AlgorithmTest, ContainsNotFound)
{
	std::vector<int> vec = { 1, 2, 3, 4, 5 };

	EXPECT_FALSE(ach::contains(vec.begin(), vec.end(), 10));
	EXPECT_FALSE(ach::ranges::contains(vec, 0));
	EXPECT_FALSE(ach::ranges::contains(vec, -1));
}

TEST(AlgorithmTest, ContainsEmpty)
{
	std::vector<int> vec;

	EXPECT_FALSE(ach::ranges::contains(vec, 42));
}

TEST(AlgorithmTest, ContainsString)
{
	std::vector<std::string> vec = { "hello", "world", "test" };

	EXPECT_TRUE(ach::ranges::contains(vec, "world"));
	EXPECT_FALSE(ach::ranges::contains(vec, "missing"));
}

TEST(AlgorithmTest, ContainsIfPredicate)
{
	std::vector<int> vec = { 1, 2, 3, 4, 5 };

	EXPECT_TRUE(ach::contains_if(vec.begin(), vec.end(), [](int x) { return x > 3; }));
	EXPECT_TRUE(ach::ranges::contains_if(vec, [](int x) { return x % 2 == 0; }));
	EXPECT_FALSE(ach::ranges::contains_if(vec, [](int x) { return x > 10; }));
}

TEST(AlgorithmTest, ContainsIfEmpty)
{
	std::vector<int> vec;

	EXPECT_FALSE(ach::ranges::contains_if(vec, [](int) { return true; }));
}

TEST(AlgorithmTest, ContainsIfComplex)
{
	struct person
	{
		std::string name;
		int age;
	};

	std::vector<person> people = { { "alice", 25 }, { "bob", 30 }, { "charlie", 35 } };

	EXPECT_TRUE(ach::ranges::contains_if(people, [](const person &p) { return p.age >= 30; }));
	EXPECT_FALSE(ach::ranges::contains_if(people, [](const person &p) { return p.name == "dave"; }));
}

TEST(AlgorithmTest, FindIndexIfFound)
{
	std::vector<int> vec = { 10, 20, 30, 40, 50 };

	auto idx = ach::find_index_if(vec.begin(), vec.end(), [](int x) { return x == 30; });
	EXPECT_EQ(idx, 2u);

	idx = ach::ranges::find_index_if(vec, [](int x) { return x > 35; });
	EXPECT_EQ(idx, 3u);
}

TEST(AlgorithmTest, FindIndexIfNotFound)
{
	std::vector<int> vec = { 10, 20, 30, 40, 50 };

	auto idx = ach::ranges::find_index_if(vec, [](int x) { return x > 100; });
	EXPECT_EQ(idx, static_cast<std::size_t>(-1));
}

TEST(AlgorithmTest, FindIndexIfEmpty)
{
	std::vector<int> vec;

	auto idx = ach::ranges::find_index_if(vec, [](int) { return true; });
	EXPECT_EQ(idx, static_cast<std::size_t>(-1));
}

TEST(AlgorithmTest, FindIndexIfFirst)
{
	std::vector<int> vec = { 1, 2, 3, 4, 5 };

	auto idx = ach::ranges::find_index_if(vec, [](int x) { return x > 0; });
	EXPECT_EQ(idx, 0u);
}

TEST(AlgorithmTest, FindIndexIfLast)
{
	std::vector<int> vec = { 1, 2, 3, 4, 5 };

	auto idx = ach::ranges::find_index_if(vec, [](int x) { return x == 5; });
	EXPECT_EQ(idx, 4u);
}

TEST(AlgorithmTest, FindIndexIfString)
{
	std::vector<std::string> vec = { "apple", "banana", "cherry", "date" };

	auto idx = ach::ranges::find_index_if(vec, [](const std::string &s) { return s.length() > 5; });
	EXPECT_EQ(idx, 1u); /* "banana" */
}

TEST(AlgorithmTest, IsSortedUntilIndexFullySorted)
{
	std::vector<int> vec = { 1, 2, 3, 4, 5 };

	auto idx = ach::is_sorted_until_index(vec.begin(), vec.end());
	EXPECT_EQ(idx, vec.size());

	idx = ach::ranges::is_sorted_until_index(vec);
	EXPECT_EQ(idx, vec.size());
}

TEST(AlgorithmTest, IsSortedUntilIndexPartiallySorted)
{
	std::vector<int> vec = { 1, 2, 3, 2, 4 }; /* unsorted at index 3 */

	auto idx = ach::ranges::is_sorted_until_index(vec);
	EXPECT_EQ(idx, 3u);
}

TEST(AlgorithmTest, IsSortedUntilIndexUnsortedFromStart)
{
	std::vector<int> vec = { 5, 4, 3, 2, 1 };

	auto idx = ach::ranges::is_sorted_until_index(vec);
	EXPECT_EQ(idx, 1u);
}

TEST(AlgorithmTest, IsSortedUntilIndexEmpty)
{
	std::vector<int> vec;

	auto idx = ach::ranges::is_sorted_until_index(vec);
	EXPECT_EQ(idx, 0u);
}

TEST(AlgorithmTest, IsSortedUntilIndexSingle)
{
	std::vector<int> vec = { 42 };

	auto idx = ach::ranges::is_sorted_until_index(vec);
	EXPECT_EQ(idx, 1u);
}

TEST(AlgorithmTest, IsSortedUntilIndexCustomComparator)
{
	std::vector<int> vec = { 5, 4, 3, 2, 1 };

	/* descending order */
	auto idx = ach::ranges::is_sorted_until_index(vec, std::greater<>());
	EXPECT_EQ(idx, vec.size());
}

TEST(AlgorithmTest, IsSortedUntilIndexDuplicates)
{
	std::vector<int> vec = { 1, 2, 2, 3, 4 };

	auto idx = ach::ranges::is_sorted_until_index(vec);
	EXPECT_EQ(idx, vec.size()); /* duplicates are fine with <= */
}

TEST(AlgorithmTest, IsSortedUntilIndexString)
{
	std::vector<std::string> vec = { "apple", "banana", "cherry", "apricot" };

	auto idx = ach::ranges::is_sorted_until_index(vec);
	EXPECT_EQ(idx, 3u); /* "apricot" comes before "cherry" */
}

TEST(AlgorithmTest, SafeCastBitCast)
{
	float f = 3.14f;
	auto u = ach::safe_cast<std::uint32_t>(f);
	auto f2 = ach::safe_cast<float>(u);
	EXPECT_FLOAT_EQ(f, f2);
}

TEST(AlgorithmTest, SafeCastPointerToUintptr)
{
	int value = 42;
	int *ptr = &value;
	auto u = ach::safe_cast<std::uintptr_t>(ptr);
	int *ptr2 = ach::safe_cast<int *>(u);

	EXPECT_EQ(ptr, ptr2);
	EXPECT_EQ(*ptr2, 42);
}

TEST(AlgorithmTest, SafeCastSignedUnsigned)
{
	std::int32_t s = -1;
	auto u = ach::safe_cast<std::uint32_t>(s);
	EXPECT_EQ(u, 0xFFFFFFFFu);
}

TEST(AlgorithmTest, SafeCastCharTypes)
{
	char c = 'A';
	auto uc = ach::safe_cast<unsigned char>(c);
	EXPECT_EQ(uc, static_cast<unsigned char>('A'));
}

TEST(AlgorithmTest, SafeCastConstexpr)
{
	constexpr float f = 3.14f;
	constexpr auto u = ach::safe_cast<std::uint32_t>(f);
	constexpr auto f2 = ach::safe_cast<float>(u);

	static_assert(f == f2);
}

TEST(AlgorithmTest, WorksWithList)
{
	std::list<int> lst = { 1, 2, 3, 4, 5 };

	EXPECT_TRUE(ach::ranges::contains(lst, 3));
	EXPECT_EQ(ach::ranges::find_index_if(lst, [](int x) { return x == 4; }), 3u);
}

TEST(AlgorithmTest, WorksWithDeque)
{
	std::deque<int> deq = { 10, 20, 30, 40, 50 };

	std::size_t count = 0;
	ach::ranges::enumerate(deq, [&](std::size_t, int) { ++count; });

	EXPECT_EQ(count, 5u);
}

TEST(AlgorithmTest, WorksWithArray)
{
	std::array<int, 5> arr = { 5, 4, 3, 2, 1 };

	auto hash = ach::ranges::hash_combine(arr, [](int x) { return x; });
	EXPECT_NE(hash, 0u);

	auto idx = ach::ranges::is_sorted_until_index(arr, std::greater<>());
	EXPECT_EQ(idx, arr.size());
}

TEST(AlgorithmTest, WorksWithForwardList)
{
	std::forward_list<int> flist = { 1, 2, 3, 4, 5 };

	EXPECT_TRUE(ach::ranges::contains(flist, 2));
	EXPECT_FALSE(ach::ranges::contains_if(flist, [](int x) { return x > 10; }));
}

TEST(AlgorithmTest, LargeContainer)
{
	std::vector<int> vec(10000);
	for (int i = 0; i < 10000; ++i)
		vec[i] = i;

	EXPECT_TRUE(ach::ranges::contains(vec, 5000));
	EXPECT_EQ(ach::ranges::find_index_if(vec, [](int x) { return x == 9999; }), 9999u);

	auto hash = ach::ranges::hash_combine(vec, [](int x) { return x; });
	EXPECT_NE(hash, 0u);
}

TEST(AlgorithmTest, EnumerateLargeRange)
{
	std::vector<int> vec(1000, 42);
	std::size_t count = 0;

	ach::ranges::enumerate(vec,
												 [&](std::size_t i, int value)
												 {
													 EXPECT_EQ(i, count);
													 EXPECT_EQ(value, 42);
													 ++count;
												 });

	EXPECT_EQ(count, 1000u);
}

TEST(AlgorithmTest, HashCombineCollisionResistance)
{
	std::vector<int> vec1 = { 1, 2, 3 };
	std::vector<int> vec2 = { 1, 2, 4 };
	std::vector<int> vec3 = { 1, 3, 2 };

	auto hash1 = ach::ranges::hash_combine(vec1, [](int x) { return x; });
	auto hash2 = ach::ranges::hash_combine(vec2, [](int x) { return x; });
	auto hash3 = ach::ranges::hash_combine(vec3, [](int x) { return x; });

	EXPECT_NE(hash1, hash2);
	EXPECT_NE(hash1, hash3);
	EXPECT_NE(hash2, hash3);
}

TEST(AlgorithmTest, ContainsWithCustomEquality)
{
	struct point
	{
		int x, y;

		bool operator==(const point &other) const
		{
			return x == other.x && y == other.y;
		}
	};

	std::vector<point> points = { { 1, 2 }, { 3, 4 }, { 5, 6 } };

	EXPECT_TRUE(ach::ranges::contains(points, point{ 3, 4 }));
	EXPECT_FALSE(ach::ranges::contains(points, point{ 7, 8 }));
}

TEST(AlgorithmTest, IsSortedUntilIndexStability)
{
	struct value
	{
		int key;
		int data;

		bool operator<(const value &other) const
		{
			return key < other.key;
		}
	};

	std::vector<value> vec = { { 1, 100 }, { 2, 200 }, { 2, 300 }, { 3, 400 } };

	auto idx = ach::ranges::is_sorted_until_index(vec, [](auto const &a, auto const &b) { return a.key < b.key; });
	EXPECT_EQ(idx, vec.size());
}
