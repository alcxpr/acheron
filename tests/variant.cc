/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#include <acheron/variant.hpp>
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

TEST(VariantTest, DefaultConstruction)
{
	ach::variant<int, double, std::string> v;
	EXPECT_EQ(v.index(), 0);
	EXPECT_TRUE(ach::holds_alternative<int>(v));
	EXPECT_EQ(ach::get<int>(v), 0);
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, ConvertingConstructionInt)
{
	ach::variant<int, double, std::string> v{ 42 };
	EXPECT_EQ(v.index(), 0);
	EXPECT_EQ(ach::get<int>(v), 42);
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, ConvertingConstructionDouble)
{
	ach::variant<int, double, std::string> v{ 3.14 };
	EXPECT_EQ(v.index(), 1);
	EXPECT_DOUBLE_EQ(ach::get<double>(v), 3.14);
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, ConvertingConstructionString)
{
	ach::variant<int, double, std::string> v{ std::string("hello") };
	EXPECT_EQ(v.index(), 2);
	EXPECT_EQ(ach::get<std::string>(v), "hello");
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, ConvertingConstructionStringLiteral)
{
	ach::variant<int, double, std::string> v{ "hello" };
	EXPECT_EQ(v.index(), 2);
	EXPECT_EQ(ach::get<std::string>(v), "hello");
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, InPlaceConstructionByType)
{
	ach::variant<int, std::string, std::vector<int>> v{ std::in_place_type<std::string>, "constructed" };
	EXPECT_EQ(v.index(), 1);
	EXPECT_EQ(ach::get<std::string>(v), "constructed");
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, InPlaceConstructionByIndex)
{
	ach::variant<int, std::string, std::vector<int>> v{ std::in_place_index<2>, 3, 42 };
	EXPECT_EQ(v.index(), 2);
	EXPECT_EQ(ach::get<std::vector<int>>(v).size(), 3);
	EXPECT_EQ(ach::get<std::vector<int>>(v)[0], 42);
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, CopyConstruction)
{
	ach::variant<int, double, std::string> v1{ std::string("original") };
	ach::variant<int, double, std::string> v2{ v1 };

	EXPECT_EQ(v2.index(), v1.index());
	EXPECT_EQ(ach::get<std::string>(v2), "original");
	EXPECT_EQ(ach::get<std::string>(v1), "original");
	EXPECT_FALSE(v1.valueless_by_exception());
	EXPECT_FALSE(v2.valueless_by_exception());
}

TEST(VariantTest, MoveConstruction)
{
	ach::variant<int, double, std::string> v1{ std::string("original") };
	ach::variant<int, double, std::string> v2{ std::move(v1) };

	EXPECT_EQ(v2.index(), 2);
	EXPECT_EQ(ach::get<std::string>(v2), "original");
	EXPECT_TRUE(v1.valueless_by_exception());
	EXPECT_FALSE(v2.valueless_by_exception());
}

TEST(VariantTest, CopyAssignmentSameType)
{
	ach::variant<int, double, std::string> v1{ 42 };
	ach::variant<int, double, std::string> v2{ 99 };

	v2 = v1;

	EXPECT_EQ(v2.index(), v1.index());
	EXPECT_EQ(ach::get<int>(v2), 42);
	EXPECT_EQ(ach::get<int>(v1), 42);
	EXPECT_FALSE(v1.valueless_by_exception());
	EXPECT_FALSE(v2.valueless_by_exception());
}

TEST(VariantTest, CopyAssignmentDifferentType)
{
	ach::variant<int, double, std::string> v1{ std::string("hello") };
	ach::variant<int, double, std::string> v2{ 42 };

	v2 = v1;

	EXPECT_EQ(v2.index(), v1.index());
	EXPECT_EQ(ach::get<std::string>(v2), "hello");
	EXPECT_EQ(ach::get<std::string>(v1), "hello");
	EXPECT_FALSE(v1.valueless_by_exception());
	EXPECT_FALSE(v2.valueless_by_exception());
}

TEST(VariantTest, MoveAssignmentSameType)
{
	ach::variant<int, double, std::string> v1{ std::string("hello") };
	ach::variant<int, double, std::string> v2{ std::string("world") };

	v2 = std::move(v1);

	EXPECT_EQ(v2.index(), 2);
	EXPECT_EQ(ach::get<std::string>(v2), "hello");
	EXPECT_FALSE(v2.valueless_by_exception());
}

TEST(VariantTest, MoveAssignmentDifferentType)
{
	ach::variant<int, double, std::string> v1{ std::string("hello") };
	ach::variant<int, double, std::string> v2{ 42 };

	v2 = std::move(v1);

	EXPECT_EQ(v2.index(), 2);
	EXPECT_EQ(ach::get<std::string>(v2), "hello");
	EXPECT_TRUE(v1.valueless_by_exception());
	EXPECT_FALSE(v2.valueless_by_exception());
}

TEST(VariantTest, ConvertingAssignmentSameType)
{
	ach::variant<int, double, std::string> v{ 42 };

	v = 99;

	EXPECT_EQ(v.index(), 0);
	EXPECT_EQ(ach::get<int>(v), 99);
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, ConvertingAssignmentDifferentType)
{
	ach::variant<int, double, std::string> v{ 42 };

	v = std::string("hello");

	EXPECT_EQ(v.index(), 2);
	EXPECT_EQ(ach::get<std::string>(v), "hello");
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, GetByType)
{
	ach::variant<int, double, std::string> v{ 42 };

	EXPECT_EQ(ach::get<int>(v), 42);
	EXPECT_EQ(v.get<int>(), 42);

	const auto &cv = v;
	EXPECT_EQ(ach::get<int>(cv), 42);
	EXPECT_EQ(cv.get<int>(), 42);
}

TEST(VariantTest, GetByIndex)
{
	ach::variant<int, double, std::string> v{ 42 };

	EXPECT_EQ(ach::get<0>(v), 42);

	const auto &cv = v;
	EXPECT_EQ(ach::get<0>(cv), 42);
}

TEST(VariantTest, GetThrowsOnWrongType)
{
	ach::variant<int, double, std::string> v{ 42 };

	EXPECT_THROW(ach::get<std::string>(v), std::bad_variant_access);
	EXPECT_THROW(v.get<std::string>(), std::bad_variant_access);
	EXPECT_THROW(ach::get<double>(v), std::bad_variant_access);
}

TEST(VariantTest, GetIfByType)
{
	ach::variant<int, double, std::string> v{ 42 };

	auto *int_ptr = ach::get_if<int>(&v);
	ASSERT_NE(int_ptr, nullptr);
	EXPECT_EQ(*int_ptr, 42);

	auto *int_ptr2 = v.get_if<int>();
	ASSERT_NE(int_ptr2, nullptr);
	EXPECT_EQ(*int_ptr2, 42);

	auto *str_ptr = ach::get_if<std::string>(&v);
	EXPECT_EQ(str_ptr, nullptr);
}

TEST(VariantTest, GetIfByIndex)
{
	ach::variant<int, double, std::string> v{ 42 };

	auto *int_ptr = ach::get_if<0>(&v);
	ASSERT_NE(int_ptr, nullptr);
	EXPECT_EQ(*int_ptr, 42);

	auto *double_ptr = ach::get_if<1>(&v);
	EXPECT_EQ(double_ptr, nullptr);
}

TEST(VariantTest, HoldsAlternative)
{
	ach::variant<int, double, std::string> v{ 42 };

	EXPECT_TRUE(ach::holds_alternative<int>(v));
	EXPECT_TRUE(v.holds_alternative<int>());
	EXPECT_FALSE(ach::holds_alternative<double>(v));
	EXPECT_FALSE(v.holds_alternative<double>());
}

TEST(VariantTest, EmplaceByType)
{
	ach::variant<int, std::string, std::vector<int>> v;

	auto &str_ref = v.emplace<std::string>("constructed");

	EXPECT_EQ(v.index(), 1);
	EXPECT_EQ(ach::get<std::string>(v), "constructed");
	EXPECT_EQ(&str_ref, &ach::get<std::string>(v));
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, EmplaceByIndex)
{
	ach::variant<int, std::string, std::vector<int>> v;

	auto &vec_ref = v.emplace<2>(3, 42);

	EXPECT_EQ(v.index(), 2);
	EXPECT_EQ(ach::get<std::vector<int>>(v).size(), 3);
	EXPECT_EQ(ach::get<std::vector<int>>(v)[0], 42);
	EXPECT_EQ(&vec_ref, &ach::get<std::vector<int>>(v));
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, SwapSameType)
{
	ach::variant<int, double, std::string> v1{ 42 };
	ach::variant<int, double, std::string> v2{ 99 };

	v1.swap(v2);

	EXPECT_EQ(ach::get<int>(v1), 99);
	EXPECT_EQ(ach::get<int>(v2), 42);
	EXPECT_FALSE(v1.valueless_by_exception());
	EXPECT_FALSE(v2.valueless_by_exception());
}

TEST(VariantTest, SwapDifferentTypes)
{
	ach::variant<int, double, std::string> v1{ 42 };
	ach::variant<int, double, std::string> v2{ std::string("hello") };

	v1.swap(v2);

	EXPECT_EQ(v1.index(), 2);
	EXPECT_EQ(v2.index(), 0);
	EXPECT_EQ(ach::get<std::string>(v1), "hello");
	EXPECT_EQ(ach::get<int>(v2), 42);
	EXPECT_FALSE(v1.valueless_by_exception());
	EXPECT_FALSE(v2.valueless_by_exception());
}

TEST(VariantTest, EqualityComparison)
{
	ach::variant<int, double, std::string> v1{ 42 };
	ach::variant<int, double, std::string> v2{ 42 };
	ach::variant<int, double, std::string> v3{ 43 };

	EXPECT_TRUE(v1 == v2);
	EXPECT_FALSE(v1 == v3);
	EXPECT_FALSE(v2 == v3);
}

TEST(VariantTest, InequalityComparison)
{
	ach::variant<int, double, std::string> v1{ 42 };
	ach::variant<int, double, std::string> v2{ 42 };
	ach::variant<int, double, std::string> v3{ 43 };

	EXPECT_FALSE(v1 != v2);
	EXPECT_TRUE(v1 != v3);
	EXPECT_TRUE(v2 != v3);
}

TEST(VariantTest, ThreeWayComparisonSameType)
{
	ach::variant<int, double, std::string> v1{ 42 };
	ach::variant<int, double, std::string> v2{ 42 };
	ach::variant<int, double, std::string> v3{ 43 };

	auto cmp1 = v1 <=> v2;
	auto cmp2 = v1 <=> v3;
	auto cmp3 = v3 <=> v1;

	EXPECT_TRUE(cmp1 == 0);
	EXPECT_TRUE(cmp2 < 0);
	EXPECT_TRUE(cmp3 > 0);
}

TEST(VariantTest, BasicVisit)
{
	ach::variant<int, double, std::string> v{ 42 };

	auto result = ach::visit(
					[](auto &&arg) -> std::string
					{
						using T = std::decay_t<decltype(arg)>;
						if constexpr (std::is_same_v<T, int>)
							return "int: " + std::to_string(arg);
						else if constexpr (std::is_same_v<T, double>)
							return "double: " + std::to_string(arg);
						else if constexpr (std::is_same_v<T, std::string>)
							return "string: " + arg;
						return "";
					},
					v);

	EXPECT_EQ(result, "int: 42");
}

TEST(VariantTest, VisitThrowsOnValueless)
{
	ach::variant<std::string> v{ "hello" };
	auto moved = std::move(v);

	EXPECT_TRUE(v.valueless_by_exception());
	EXPECT_THROW(ach::visit([](auto &&) { return 0; }, v), std::bad_variant_access);
}

TEST(VariantTest, StaticTypeIndices)
{
	using test_variant = ach::variant<int, std::string, double>;

	static_assert(test_variant::of<int> == 0);
	static_assert(test_variant::of<std::string> == 1);
	static_assert(test_variant::of<double> == 2);

	EXPECT_EQ(test_variant::of<int>, 0);
	EXPECT_EQ(test_variant::of<std::string>, 1);
	EXPECT_EQ(test_variant::of<double>, 2);
}

TEST(VariantTest, SwitchOnVariantIndex)
{
	ach::variant<int, std::string, double> v;

	v = 42;
	switch (v.index())
	{
		case decltype(v)::of<int>:
			EXPECT_EQ(ach::get<int>(v), 42);
			break;
		case decltype(v)::of<std::string>:
			FAIL() << "should not reach string case";
			break;
		case decltype(v)::of<double>:
			FAIL() << "should not reach double case";
			break;
		default:
			FAIL() << "unknown variant index";
	}

	v = std::string("hello");
	switch (v.index())
	{
		case decltype(v)::of<int>:
			FAIL() << "should not reach int case";
			break;
		case decltype(v)::of<std::string>:
			EXPECT_EQ(ach::get<std::string>(v), "hello");
			break;
		case decltype(v)::of<double>:
			FAIL() << "should not reach double case";
			break;
		default:
			FAIL() << "unknown variant index";
	}
}

TEST(VariantTest, MonostateDefaultConstruction)
{
	struct NonDefault
	{
		int value;
		NonDefault() = delete;
		explicit NonDefault(int v) : value(v)
		{}
	};

	ach::variant<ach::monostate, NonDefault> v;
	EXPECT_EQ(v.index(), 0);
	EXPECT_TRUE(ach::holds_alternative<ach::monostate>(v));
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, MonostateComparison)
{
	ach::monostate m1, m2;
	EXPECT_TRUE(m1 == m2);
	EXPECT_FALSE(m1 != m2);
	EXPECT_TRUE((m1 <=> m2) == 0);
}

TEST(VariantTest, VariantSize)
{
	using test_variant = ach::variant<int, double, std::string>;
	EXPECT_EQ(ach::variant_size_v<test_variant>, 3);
	EXPECT_EQ(std::variant_size_v<test_variant>, 3);
}

TEST(VariantTest, VariantAlternative)
{
	using test_variant = ach::variant<int, double, std::string>;
	static_assert(std::is_same_v<ach::variant_alternative_t<0, test_variant>, int>);
	static_assert(std::is_same_v<ach::variant_alternative_t<1, test_variant>, double>);
	static_assert(std::is_same_v<ach::variant_alternative_t<2, test_variant>, std::string>);
}

TEST(VariantTest, Hash)
{
	using test_variant = ach::variant<int, double, std::string>;
	test_variant v1{ 42 };
	test_variant v2{ 42 };
	test_variant v3{ 99 };

	std::hash<test_variant> hasher;
	EXPECT_EQ(hasher(v1), hasher(v2));
	EXPECT_NE(hasher(v1), hasher(v3));
}

TEST(VariantTest, MoveOnlyType)
{
	ach::variant<std::unique_ptr<int>, std::string> v{ std::make_unique<int>(42) };
	EXPECT_EQ(v.index(), 0);
	EXPECT_EQ(*ach::get<std::unique_ptr<int>>(v), 42);
	EXPECT_FALSE(v.valueless_by_exception());
}

TEST(VariantTest, ValuelessAfterMove)
{
	ach::variant<std::string> v{ "hello" };
	EXPECT_FALSE(v.valueless_by_exception());

	auto moved = std::move(v);
	EXPECT_TRUE(v.valueless_by_exception());
	EXPECT_FALSE(moved.valueless_by_exception());
	EXPECT_EQ(v.index(), ach::variant_npos);
}

TEST(VariantTest, AssignToValueless)
{
	ach::variant<int, double, std::string> v1{ std::string("hello") };
	ach::variant<int, double, std::string> v2{ 42 };

	auto v2_moved = std::move(v2);
	EXPECT_TRUE(v2.valueless_by_exception());

	v2 = v1;

	EXPECT_FALSE(v2.valueless_by_exception());
	EXPECT_EQ(v2.index(), 2);
	EXPECT_EQ(ach::get<std::string>(v2), "hello");
}

TEST(VariantTest, BasicMatch)
{
	ach::variant<int, double, std::string> v{ 42 };

	auto result = ach::match(v) | [](auto &&var)
	{
		return ach::visit(
						[](auto &&value) -> std::string
						{
							using T = std::decay_t<decltype(value)>;
							if constexpr (std::is_same_v<T, int>)
								return "int: " + std::to_string(value);
							else if constexpr (std::is_same_v<T, double>)
								return "double: " + std::to_string(value);
							else if constexpr (std::is_same_v<T, std::string>)
								return "string: " + value;
							return "";
						},
						var);
	};

	EXPECT_EQ(result, "int: 42");
}

TEST(VariantTest, MatchWithDifferentTypes)
{
	ach::variant<int, double, std::string> v1{ 42 };
	ach::variant<int, double, std::string> v2{ std::string("hello") };
	ach::variant<int, double, std::string> v3{ 3.14 };

	auto matcher = [](auto &&var)
	{
		return ach::visit(
						[]<typename T0>([[maybe_unused]] T0 &&value) -> std::string
						{
							using T = std::decay_t<T0>;
							if constexpr (std::is_same_v<T, int>)
								return "matched_int";
							else if constexpr (std::is_same_v<T, double>)
								return "matched_double";
							else if constexpr (std::is_same_v<T, std::string>)
								return "matched_string";
							return "";
						},
						var);
	};

	EXPECT_EQ(ach::match(v1) | matcher, "matched_int");
	EXPECT_EQ(ach::match(v2) | matcher, "matched_string");
	EXPECT_EQ(ach::match(v3) | matcher, "matched_double");
}

TEST(VariantTest, MatchBuilderForwarding)
{
	ach::variant<int, std::string> v{ 42 };

	auto result = ach::match(std::move(v)) | [](auto &&var)
	{
		return ach::visit(
						[]<typename T0>(T0 &&value) -> int
						{
							using T = std::decay_t<T0>;
							if constexpr (std::is_same_v<T, int>)
								return value * 2;
							else
								return 0;
						},
						var);
	};

	EXPECT_EQ(result, 84);
}

TEST(VariantTest, MatchWithSideEffects)
{
	ach::variant<int, std::string> v{ std::string("test") };

	int counter = 0;
	auto result = ach::match(v) | [&counter](auto &&var)
	{
		return ach::visit(
						[&counter]<typename T0>(T0 &&value) -> bool
						{
							counter++;
							using T = std::decay_t<T0>;
							return std::is_same_v<T, std::string>;
						},
						var);
	};

	EXPECT_TRUE(result);
	EXPECT_EQ(counter, 1);
}

TEST(VariantTest, MatchWithComplexTypes)
{
	struct Point
	{
		int x, y;
		Point(int x, int y) : x(x), y(y)
		{}
	};

	struct Circle
	{
		int radius;
		Circle(int r) : radius(r)
		{}
	};

	ach::variant<Point, Circle, std::string> v{ Point{ 3, 4 } };

	auto result = ach::match(v) | [](auto &&var)
	{
		return ach::visit(
						[]<typename T0>(T0 &&value) -> std::string
						{
							using T = std::decay_t<T0>;
							if constexpr (std::is_same_v<T, Point>)
								return "point(" + std::to_string(value.x) + "," + std::to_string(value.y) + ")";
							else if constexpr (std::is_same_v<T, Circle>)
								return "circle(r=" + std::to_string(value.radius) + ")";
							else if constexpr (std::is_same_v<T, std::string>)
								return "string:" + value;
							return "";
						},
						var);
	};

	EXPECT_EQ(result, "point(3,4)");
}

TEST(VariantTest, MatchInLoop)
{
	using test_variant = ach::variant<int, std::string, bool>;

	std::vector<test_variant> variants = { 42, std::string("hello"), true, 99, std::string("world"), false };

	std::vector<std::string> results;

	for (const auto &v: variants)
	{
		auto result = ach::match(v) | [](auto &&var)
		{
			return ach::visit(
							[](auto &&value) -> std::string
							{
								using T = std::decay_t<decltype(value)>;
								if constexpr (std::is_same_v<T, int>)
									return "int:" + std::to_string(value);
								else if constexpr (std::is_same_v<T, std::string>)
									return "string:" + value;
								else if constexpr (std::is_same_v<T, bool>)
									return value ? "bool:true" : "bool:false";
								return "";
							},
							var);
		};
		results.push_back(result);
	}

	EXPECT_EQ(results.size(), 6);
	EXPECT_EQ(results[0], "int:42");
	EXPECT_EQ(results[1], "string:hello");
	EXPECT_EQ(results[2], "bool:true");
	EXPECT_EQ(results[3], "int:99");
	EXPECT_EQ(results[4], "string:world");
	EXPECT_EQ(results[5], "bool:false");
}
