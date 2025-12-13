/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#include <acheron/freelist.hpp>
#include <gtest/gtest.h>
#include <string>

TEST(FreelistTest, DefaultConstruction)
{
	ach::freelist<int> fl(0);
	EXPECT_EQ(fl.size(), 0);
	EXPECT_EQ(fl.capacity(), 0);
	EXPECT_TRUE(fl.empty());
}

TEST(FreelistTest, InitialCapacity)
{
	ach::freelist<int> fl(64);
	EXPECT_EQ(fl.capacity(), 64);
	EXPECT_EQ(fl.size(), 0);
	EXPECT_EQ(fl.available(), 64);
}

TEST(FreelistTest, AcquireAndRelease)
{
	ach::freelist<int> fl(64);
	auto *node = fl.pop();
	EXPECT_NE(node, nullptr);
	EXPECT_EQ(fl.size(), 1);

	fl.push(node);
	EXPECT_EQ(fl.size(), 0);
}

TEST(FreelistTest, EmplaceAndDestroy)
{
	ach::freelist<std::string> fl(64);
	auto *str = fl.emplace("hello");
	EXPECT_EQ(*str, "hello");
	EXPECT_EQ(fl.size(), 1);

	fl.destroy(str);
	EXPECT_EQ(fl.size(), 0);
}

TEST(FreelistTest, MultipleAcquisitions)
{
	ach::freelist<int> fl(64);
	std::vector<ach::freelist<int>::node_type *> nodes;

	nodes.reserve(10);
	for (int i = 0; i < 10; ++i)
		nodes.push_back(fl.pop());

	EXPECT_EQ(fl.size(), 10);
	EXPECT_EQ(fl.available(), 54);

	for (auto *node: nodes)
		fl.push(node);

	EXPECT_EQ(fl.size(), 0);
	EXPECT_EQ(fl.available(), 64);
}

TEST(FreelistTest, GeometricGrowth)
{
	ach::freelist<int> fl(64);

	for (int i = 0; i < 65; ++i)
		fl.pop();

	EXPECT_EQ(fl.capacity(), 64 + 128);
	EXPECT_EQ(fl.size(), 65);
}

TEST(FreelistTest, ReuseAfterRelease)
{
	ach::freelist<int> fl(64);

	auto *node1 = fl.pop();
	fl.push(node1);
	auto *node2 = fl.pop();

	EXPECT_EQ(node1, node2);
}

TEST(FreelistTest, NonTrivialTypeConstruction)
{
	ach::freelist<std::string> fl(64);

	auto *s1 = fl.emplace("test1");
	auto *s2 = fl.emplace("test2");
	auto *s3 = fl.emplace("test3");

	EXPECT_EQ(*s1, "test1");
	EXPECT_EQ(*s2, "test2");
	EXPECT_EQ(*s3, "test3");

	fl.destroy(s2);
	auto *s4 = fl.emplace("test4");
	EXPECT_EQ(*s4, "test4");
}

TEST(FreelistTest, MoveConstruction)
{
	ach::freelist<int> fl1(64);
	fl1.pop();
	fl1.pop();

	ach::freelist<int> fl2(std::move(fl1));
	EXPECT_EQ(fl2.size(), 2);
	EXPECT_EQ(fl2.capacity(), 64);
	EXPECT_EQ(fl1.size(), 0);
	EXPECT_EQ(fl1.capacity(), 0);
}

TEST(FreelistTest, MoveAssignment)
{
	ach::freelist<int> fl1(64);
	fl1.pop();

	ach::freelist<int> fl2(32);
	fl2 = std::move(fl1);

	EXPECT_EQ(fl2.size(), 1);
	EXPECT_EQ(fl2.capacity(), 64);
}

TEST(FreelistTest, PointerTypeSpecialization)
{
	ach::freelist<int *> fl(64);
	auto *node = fl.pop();
	EXPECT_NE(node, nullptr);

	int value = 42;
	*node->value() = value;
	EXPECT_EQ(*node->value(), 42);

	fl.push(node);
}

TEST(FreelistTest, LargeAllocation)
{
	ach::freelist<int> fl(1024);
	std::vector<ach::freelist<int>::node_type *> nodes;

	nodes.reserve(2048);
	for (int i = 0; i < 2048; ++i)
		nodes.push_back(fl.pop());

	EXPECT_GE(fl.capacity(), 2048);
	EXPECT_EQ(fl.size(), 2048);
}

TEST(FreelistTest, AlternatingAcquireRelease)
{
	ach::freelist<std::string> fl(64);

	for (int i = 0; i < 100; ++i)
	{
		auto *str = fl.emplace("temp");
		EXPECT_EQ(*str, "temp");
		fl.destroy(str);
	}

	EXPECT_EQ(fl.size(), 0);
}
