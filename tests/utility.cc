/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#include <acheron/utility.hpp>
#include <gtest/gtest.h>

#include <thread>
#include <vector>

TEST(CounterTest, DefaultConstruction)
{
	ach::counter<int> c;
	EXPECT_EQ(c.load(), 0);
	EXPECT_EQ(c.value(), 0);
}

TEST(CounterTest, InitialValueConstruction)
{
	ach::counter<int> c(42);
	EXPECT_EQ(c.load(), 42);
	EXPECT_EQ(c.value(), 42);
}

TEST(CounterTest, PreIncrement)
{
	ach::counter<int> c(10);
	auto prev = ++c;
	EXPECT_EQ(prev, 10);
	EXPECT_EQ(c.load(), 11);
}

TEST(CounterTest, PostIncrement)
{
	ach::counter<int> c(10);
	auto prev = c++;
	EXPECT_EQ(prev, 10);
	EXPECT_EQ(c.load(), 11);
}

TEST(CounterTest, AdditionAssignment)
{
	ach::counter<int> c(100);
	auto prev = c += 50;
	EXPECT_EQ(prev, 100);
	EXPECT_EQ(c.load(), 150);
}

TEST(CounterTest, PreDecrement)
{
	ach::counter<int> c(20);
	auto prev = --c;
	EXPECT_EQ(prev, 20);
	EXPECT_EQ(c.load(), 19);
}

TEST(CounterTest, PostDecrement)
{
	ach::counter<int> c(20);
	auto prev = c--;
	EXPECT_EQ(prev, 20);
	EXPECT_EQ(c.load(), 19);
}

TEST(CounterTest, SubtractionAssignment)
{
	ach::counter<int> c(100);
	auto prev = c -= 30;
	EXPECT_EQ(prev, 100);
	EXPECT_EQ(c.load(), 70);
}

TEST(CounterTest, Store)
{
	ach::counter<int> c(42);
	c.store(99);
	EXPECT_EQ(c.load(), 99);
}

TEST(CounterTest, Reset)
{
	ach::counter<int> c(42);
	c.reset();
	EXPECT_EQ(c.load(), 0);

	c.store(100);
	c.reset(50);
	EXPECT_EQ(c.load(), 50);
}

TEST(CounterTest, ExplicitConversion)
{
	ach::counter<int> c(123);
	int value = static_cast<int>(c);
	EXPECT_EQ(value, 123);
}

TEST(CounterTest, IsLockFree)
{
	ach::counter<int> c;
	EXPECT_TRUE(c.is_lock_free());

	ach::counter<std::size_t> c2;
	EXPECT_TRUE(c2.is_lock_free());
}

TEST(CounterTest, UnsignedType)
{
	ach::counter<std::size_t> c(100);
	++c;
	EXPECT_EQ(c.load(), 101u);
	--c;
	EXPECT_EQ(c.load(), 100u);
}

TEST(CounterTest, DeductionGuide)
{
	ach::counter c(42);
	static_assert(std::is_same_v<decltype(c)::value_type, int>);
	EXPECT_EQ(c.load(), 42);

	ach::counter c2(100u);
	static_assert(std::is_same_v<decltype(c2)::value_type, unsigned int>);
	EXPECT_EQ(c2.load(), 100u);
}

TEST(CounterTest, MultipleOperations)
{
	ach::counter<int> c(0);
	++c;
	++c;
	c += 5;
	--c;
	c -= 2;
	EXPECT_EQ(c.load(), 4);
}

TEST(CounterTest, Wrapping)
{
	ach::counter<unsigned char> c(255);
	++c;
	EXPECT_EQ(c.load(), 0);

	c.store(0);
	--c;
	EXPECT_EQ(c.load(), 255);
}

TEST(CounterTest, ThreadSafetyIncrement)
{
	ach::counter<int> c(0);
	constexpr int num_threads = 10;
	constexpr int increments_per_thread = 1000;

	std::vector<std::thread> threads;
	for (int i = 0; i < num_threads; ++i)
	{
		threads.emplace_back([&c]()
		{
			for (int j = 0; j < increments_per_thread; ++j)
				++c;
		});
	}

	for (auto &t : threads)
		t.join();

	EXPECT_EQ(c.load(), num_threads * increments_per_thread);
}

TEST(CounterTest, ThreadSafetyDecrement)
{
	ach::counter<int> c(10000);
	constexpr int num_threads = 10;
	constexpr int decrements_per_thread = 1000;

	std::vector<std::thread> threads;
	for (int i = 0; i < num_threads; ++i)
	{
		threads.emplace_back([&c]()
		{
			for (int j = 0; j < decrements_per_thread; ++j)
				--c;
		});
	}

	for (auto &t : threads)
		t.join();

	EXPECT_EQ(c.load(), 0);
}

TEST(CounterTest, ThreadSafetyMixed)
{
	ach::counter<int> c(0);
	constexpr int num_threads = 8;
	constexpr int ops_per_thread = 500;

	std::vector<std::thread> threads;
	for (int i = 0; i < num_threads; ++i)
	{
		threads.emplace_back([&c, i]()
		{
			if (i % 2 == 0)
			{
				for (int j = 0; j < ops_per_thread; ++j)
					c += 2;
			}
			else
			{
				for (int j = 0; j < ops_per_thread; ++j)
					c -= 1;
			}
		});
	}

	for (auto &t : threads)
		t.join();

	int expected = (num_threads / 2) * ops_per_thread * 2 - (num_threads / 2) * ops_per_thread;
	EXPECT_EQ(c.load(), expected);
}

TEST(ResourceTest, Construction)
{
	auto r = ach::make_resource(42);
	auto& ref = ach::borrow(r);
	EXPECT_EQ(ref, 42);
}

TEST(ResourceTest, DeductionGuide)
{
	[[maybe_unused]] ach::resource r1 { 42 };
	[[maybe_unused]] ach::resource r2 { 3.142 };
	[[maybe_unused]] ach::resource r3 { std::string("skibidi toilet") };

	static_assert(std::is_same_v<decltype(r1)::value_type, int>);
	static_assert(std::is_same_v<decltype(r2)::value_type, double>);
	static_assert(std::is_same_v<decltype(r3)::value_type, std::string>);
}

TEST(ResourceTest, MutableBorrow)
{
	auto r = ach::make_resource(10);
	auto& ref = ach::borrow(r);

	static_assert(std::is_same_v<decltype(ref), int&>);
	EXPECT_EQ(ref, 10);

	ref = 20;
	EXPECT_EQ(borrow(r), 20);
}

TEST(ResourceTest, ConstBorrow)
{
	auto r = ach::make_resource(42);
	const auto& cr = r;
	const auto& cref = ach::borrow(cr);

	static_assert(std::is_same_v<decltype(cref), const int&>);
	EXPECT_EQ(cref, 42);
}

TEST(ResourceTest, RvalueBorrow)
{
	auto r = ach::make_resource(99);
	int&& moved = ach::borrow(std::move(r));

	static_assert(std::is_same_v<decltype(moved), int&&>);
	EXPECT_EQ(moved, 99);
}

TEST(ResourceTest, MoveSemantics)
{
	auto r1 = ach::make_resource(100);
	auto r2 = std::move(r1);

	EXPECT_EQ(borrow(r2), 100);
}

TEST(ResourceTest, MoveAssignment)
{
	auto r1 = ach::make_resource(50);
	auto r2 = ach::make_resource(75);

	r2 = std::move(r1);
	EXPECT_EQ(ach::borrow(r2), 50);
}

TEST(ResourceTest, NonCopyable)
{
	static_assert(!std::is_copy_constructible_v<ach::resource<int>>);
	static_assert(!std::is_copy_assignable_v<ach::resource<int>>);
}

TEST(ResourceTest, MoveOnly)
{
	static_assert(std::is_move_constructible_v<ach::resource<int>>);
	static_assert(std::is_move_assignable_v<ach::resource<int>>);
}

TEST(ResourceTest, ComplexType)
{
	auto r = ach::make_resource(std::string("ownership"));
	auto& str = ach::borrow(r);

	EXPECT_EQ(str, "ownership");
	str += " model";
	EXPECT_EQ(ach::borrow(r), "ownership model");
}

TEST(ResourceTest, OwnershipTransfer)
{
	auto r1 = ach::make_resource(std::string("data"));
	std::string&& moved = ach::borrow(std::move(r1));

	auto r2 = ach::make_resource(std::move(moved));
	EXPECT_EQ(ach::borrow(r2), "data");
}
