#include <acheron/defer.hpp>
#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>

using namespace ach;

TEST(DeferTest, ExecutesOnDestruction)
{
	std::vector<int> v;
	{
		auto guard = defer([&] { v.push_back(42); });
		EXPECT_TRUE(guard.is_active());
	}
	EXPECT_EQ(v, std::vector<int>({ 42 }));
}

TEST(DeferTest, CanCancelBeforeDestruction)
{
	std::vector<int> v;
	{
		auto guard = defer([&] { v.push_back(99); });
		guard.cancel();
		EXPECT_FALSE(guard.is_active());
	}
	EXPECT_TRUE(v.empty());
}

TEST(DeferTest, CanExecuteManually)
{
	std::vector<int> v;
	{
		auto guard = defer([&] { v.push_back(5); });
		guard.execute();
		EXPECT_FALSE(guard.is_active());
		EXPECT_EQ(v, std::vector<int>({ 5 }));
	}
	EXPECT_EQ(v, std::vector<int>({ 5 }));
}

TEST(DeferTest, MoveConstructionTransfersOwnership)
{
	std::vector<int> v;
	{
		auto guard1 = defer([&] { v.push_back(7); });
		auto guard2 = std::move(guard1);

		EXPECT_FALSE(guard1.is_active());
		EXPECT_TRUE(guard2.is_active());
	}
	EXPECT_EQ(v, std::vector<int>({ 7 }));
}

TEST(DeferTest, MoveAssignmentExecutesCurrentIfActive)
{
	std::vector<int> v;

	struct PushBack
	{
		std::reference_wrapper<std::vector<int>> v;
		int x;
		void operator()() const
		{
			v.get().push_back(x);
		}
	};

	{
		auto guard1 = defer(PushBack{ v, 1 });
		auto guard2 = defer(PushBack{ v, 2 });

		guard2 = std::move(guard1);

		EXPECT_FALSE(guard1.is_active());
		EXPECT_TRUE(guard2.is_active());
		EXPECT_EQ(v, std::vector<int>({ 2 }));
	}
	EXPECT_EQ(v, std::vector<int>({ 2, 1 }));
}

TEST(DeferTest, NoExecutionAfterCancelAndExecute)
{
	std::vector<int> v;
	{
		auto guard = defer([&] { v.push_back(42); });
		guard.execute();
		guard.cancel();
	}
	EXPECT_EQ(v, std::vector<int>({ 42 }));
}

TEST(DeferTest, WorksWithNothrowMoveOnlyFunctor)
{
	struct F
	{
		F() = default;
		F(F &&) noexcept = default;
		void operator()() const noexcept
		{}
	};

	static_assert(ach::Deferrable<F>);
	auto guard = defer(F{});
	EXPECT_TRUE(guard.is_active());
}

TEST(DeferTest, ThrowsIfCallableThrowsOnExecute)
{
	struct Thrower
	{
		void operator()() const
		{
			throw std::runtime_error("boom");
		}
	};

	auto guard = defer(Thrower{});
	EXPECT_THROW(guard.execute(), std::runtime_error);
}

TEST(DeferTest, IsDeferrableConceptWorks)
{
	struct NoInvoke
	{};
	static_assert(!ach::is_deferrable<NoInvoke>);

	struct Invocable
	{
		void operator()() const noexcept
		{}
	};
	static_assert(ach::is_deferrable<Invocable>);
}
