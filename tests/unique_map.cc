/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#include <acheron/unique_map.hpp>
#include <gtest/gtest.h>
#include <string>

TEST(UniqueMapTest, DefaultConstruction)
{
	ach::unique_map<int, int> map;
	EXPECT_TRUE(map.empty());
	EXPECT_EQ(map.size(), 0);
}

TEST(UniqueMapTest, EmplaceAndFind)
{
	ach::unique_map<int, int> map;
	auto [key, value, inserted] = map.emplace(42, 100);

	EXPECT_TRUE(inserted);
	EXPECT_EQ(*key, 42);
	EXPECT_EQ(*value, 100);
	EXPECT_EQ(map.size(), 1);

	auto *found = map.find(42);
	EXPECT_NE(found, nullptr);
	EXPECT_EQ(*found, 100);
}

TEST(UniqueMapTest, DuplicateKeyReturnsFalse)
{
	ach::unique_map<int, int> map;
	map.emplace(10, 20);

	auto [key, value, inserted] = map.emplace(10, 30);
	EXPECT_FALSE(inserted);
	EXPECT_EQ(*value, 20);
	EXPECT_EQ(map.size(), 1);
}

TEST(UniqueMapTest, FindNonExistent)
{
	ach::unique_map<int, int> map;
	map.emplace(1, 10);

	EXPECT_EQ(map.find(2), nullptr);
}

TEST(UniqueMapTest, EraseExisting)
{
	ach::unique_map<int, int> map;
	map.emplace(5, 50);

	EXPECT_EQ(map.erase(5), 1);
	EXPECT_EQ(map.size(), 0);
	EXPECT_EQ(map.find(5), nullptr);
}

TEST(UniqueMapTest, EraseNonExistent)
{
	ach::unique_map<int, int> map;
	EXPECT_EQ(map.erase(999), 0);
}

TEST(UniqueMapTest, StableKeyPointers)
{
	ach::unique_map<int, int> map;
	auto [key1, val1, _] = map.emplace(1, 10);
	auto [key2, val2, __] = map.emplace(2, 20);

	int *key1_ptr = key1;
	int *key2_ptr = key2;

	map.emplace(3, 30);
	map.emplace(4, 40);

	EXPECT_EQ(key1_ptr, key1);
	EXPECT_EQ(key2_ptr, key2);

	EXPECT_EQ(*map.find(1), 10);
	EXPECT_EQ(*map.find(2), 20);
}

TEST(UniqueMapTest, SlotReuse)
{
	ach::unique_map<int, std::string> map;
	auto [_, val1, __] = map.emplace(1, "first");
	std::string *ptr1 = val1;

	map.erase(1);
	auto [_2, val2, __2] = map.emplace(2, "second");

	EXPECT_EQ(ptr1, val2);
	EXPECT_EQ(*val2, "second");
}

TEST(UniqueMapTest, ClearAll)
{
	ach::unique_map<int, int> map;
	for (int i = 0; i < 100; ++i)
		map.emplace(i, i * 2);

	EXPECT_EQ(map.size(), 100);
	map.clear();
	EXPECT_EQ(map.size(), 0);
	EXPECT_TRUE(map.empty());
}

TEST(UniqueMapTest, StringKeys)
{
	ach::unique_map<std::string, int> map;
	map.emplace("hello", 1);
	map.emplace("world", 2);

	EXPECT_EQ(*map.find("hello"), 1);
	EXPECT_EQ(*map.find("world"), 2);
	EXPECT_EQ(map.find("missing"), nullptr);
}

TEST(UniqueMapTest, NonTrivialValues)
{
	ach::unique_map<int, std::string> map;
	map.emplace(1, "one");
	map.emplace(2, "two");

	EXPECT_EQ(*map.find(1), "one");
	EXPECT_EQ(*map.find(2), "two");

	map.erase(1);
	EXPECT_EQ(map.find(1), nullptr);
}

TEST(UniqueMapTest, Rehashing)
{
	ach::unique_map<int, int> map(4);

	for (int i = 0; i < 100; ++i)
		map.emplace(i, i * 10);

	EXPECT_EQ(map.size(), 100);

	for (int i = 0; i < 100; ++i)
	{
		auto *val = map.find(i);
		EXPECT_NE(val, nullptr);
		EXPECT_EQ(*val, i * 10);
	}
}

TEST(UniqueMapTest, RvalueKeyEmplace)
{
	ach::unique_map<std::string, int> map;
	std::string key = "test";
	map.emplace(std::move(key), 42);

	EXPECT_EQ(*map.find("test"), 42);
}

TEST(UniqueMapTest, HeterogeneousLookup)
{
	ach::unique_map<int, std::string> map;
	map.emplace(10, "ten");

	short key = 10;
	auto *val = map.find(key);
	EXPECT_NE(val, nullptr);
	EXPECT_EQ(*val, "ten");
}

TEST(UniqueMapTest, Swap)
{
	ach::unique_map<int, int> map1;
	ach::unique_map<int, int> map2;

	map1.emplace(1, 10);
	map2.emplace(2, 20);

	map1.swap(map2);

	EXPECT_EQ(*map1.find(2), 20);
	EXPECT_EQ(*map2.find(1), 10);
	EXPECT_EQ(map1.find(1), nullptr);
	EXPECT_EQ(map2.find(2), nullptr);
}

TEST(UniqueMapTest, LargeScale)
{
	ach::unique_map<int, int> map;
	constexpr int N = 10000;

	for (int i = 0; i < N; ++i)
		map.emplace(i, i * 2);

	EXPECT_EQ(map.size(), N);

	for (int i = 0; i < N; i += 2)
		map.erase(i);

	EXPECT_EQ(map.size(), N / 2);

	for (int i = 0; i < N; ++i)
	{
		if (i % 2 == 0)
			EXPECT_EQ(map.find(i), nullptr);
		else
			EXPECT_EQ(*map.find(i), i * 2);
	}
}

TEST(UniqueMapTest, ConstFind)
{
	ach::unique_map<int, int> map;
	map.emplace(5, 50);

	const auto &cmap = map;
	const int *val = cmap.find(5);
	EXPECT_NE(val, nullptr);
	EXPECT_EQ(*val, 50);
}

TEST(UniqueMapTest, MaxSize)
{
	ach::unique_map<int, int> map;
	EXPECT_GT(map.max_size(), 0);
}
