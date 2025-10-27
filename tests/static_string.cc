/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#include <acheron/static_string.hpp>
#include <gtest/gtest.h>
#include <sstream>

using namespace ach::literals;

TEST(StaticStringTest, DefaultConstruction)
{
	ach::static_string<32> s;
	EXPECT_EQ(s.size(), 0u);
	EXPECT_TRUE(s.empty());
	EXPECT_EQ(s.data()[0], '\0');
}

TEST(StaticStringTest, ConstructFromCString)
{
	ach::static_string<32> s("hello");
	EXPECT_EQ(s.size(), 5u);
	EXPECT_FALSE(s.empty());
	EXPECT_EQ(s, "hello");
	EXPECT_EQ(s.data()[5], '\0');
}

TEST(StaticStringTest, ConstructFromCStringWithLength)
{
	const char str[] = "world\0extra";
	ach::static_string<32> s(str, 5);
	EXPECT_EQ(s.size(), 5u);
	EXPECT_EQ(s, "world");
	EXPECT_EQ(s.data()[5], '\0');
}

TEST(StaticStringTest, ConstructFromStringView)
{
	std::string_view sv = "test string";
	ach::static_string<32> s(sv);
	EXPECT_EQ(s.size(), sv.size());
	EXPECT_EQ(s, sv);
}

TEST(StaticStringTest, ConstructFromStaticString)
{
	ach::static_string<16> s1("original");
	ach::static_string<32> s2(s1);
	EXPECT_EQ(s1, s2);
	EXPECT_EQ(s1.size(), s2.size());
}

TEST(StaticStringTest, CopyConstruction)
{
	ach::static_string<32> s1("hello");
	ach::static_string<32> s2(s1);
	EXPECT_EQ(s1, s2);
	/* different address but same content to check if this is actually a deep copy */
	EXPECT_NE(s1.data(), s2.data());
	EXPECT_STREQ(s1.data(), s2.data());
}

TEST(StaticStringTest, CopyAssignment)
{
	ach::static_string<32> s1("first");
	ach::static_string<32> s2("second");
	s2 = s1;
	EXPECT_EQ(s1, s2);
}

TEST(StaticStringTest, Iterators)
{
	ach::static_string<32> s("test");
	std::string result;
	for (auto c : s)
		result += c;
	EXPECT_EQ(result, "test");
}

TEST(StaticStringTest, ReverseIterators)
{
	ach::static_string<32> s("abc");
	std::string result;
	for (auto it = s.rbegin(); it != s.rend(); ++it)
		result += *it;
	EXPECT_EQ(result, "cba");
}

TEST(StaticStringTest, ElementAccess)
{
	ach::static_string<32> s("hello");
	EXPECT_EQ(s[0], 'h');
	EXPECT_EQ(s[4], 'o');
	EXPECT_EQ(s.front(), 'h');
	EXPECT_EQ(s.back(), 'o');
}

TEST(StaticStringTest, ElementAccessAt)
{
	ach::static_string<32> s("test");
	EXPECT_EQ(s.at(0), 't');
	EXPECT_EQ(s.at(3), 't');
	EXPECT_THROW(s.at(4), std::out_of_range);
	EXPECT_THROW(s.at(100), std::out_of_range);
}

TEST(StaticStringTest, DataAndCStr)
{
	ach::static_string<32> s("data test");
	EXPECT_STREQ(s.c_str(), "data test");
	EXPECT_STREQ(s.data(), "data test");
	EXPECT_EQ(s.c_str()[9], '\0');
}

TEST(StaticStringTest, ConversionToStringView)
{
	ach::static_string<32> s("convert");
	std::string_view sv = s;
	EXPECT_EQ(sv.size(), s.size());
	EXPECT_EQ(sv.data(), s.data());
	EXPECT_EQ(sv, "convert");
}

TEST(StaticStringTest, Clear)
{
	ach::static_string<32> s("hello");
	EXPECT_FALSE(s.empty());
	s.clear();
	EXPECT_TRUE(s.empty());
	EXPECT_EQ(s.size(), 0u);
	EXPECT_EQ(s.data()[0], '\0');
}

TEST(StaticStringTest, PushBack)
{
	ach::static_string<32> s;
	s.push_back('h');
	s.push_back('i');
	EXPECT_EQ(s.size(), 2u);
	EXPECT_EQ(s, "hi");
	EXPECT_EQ(s.data()[2], '\0');
}

TEST(StaticStringTest, PopBack)
{
	ach::static_string<32> s("hello");
	s.pop_back();
	EXPECT_EQ(s.size(), 4u);
	EXPECT_EQ(s, "hell");
	EXPECT_EQ(s.data()[4], '\0');
}

TEST(StaticStringTest, AppendStringView)
{
	ach::static_string<32> s("hello");
	s += " world";
	EXPECT_EQ(s, "hello world");
	EXPECT_EQ(s.size(), 11u);
}

TEST(StaticStringTest, AppendChar)
{
	ach::static_string<32> s("hello");
	s += '!';
	EXPECT_EQ(s, "hello!");
	EXPECT_EQ(s.size(), 6u);
}

TEST(StaticStringTest, AppendStaticString)
{
	ach::static_string<16> s1("hello");
	ach::static_string<16> s2(" world");
	s1 += s2;
	EXPECT_EQ(s1, "hello world");
}

TEST(StaticStringTest, Compare)
{
	ach::static_string<32> s1("abc");
	ach::static_string<32> s2("abc");
	ach::static_string<32> s3("abd");
	EXPECT_EQ(s1.compare(s2), 0);
	EXPECT_LT(s1.compare(s3), 0);
	EXPECT_GT(s3.compare(s1), 0);
}

TEST(StaticStringTest, StartsWith)
{
	ach::static_string<32> s("hello world");
	EXPECT_TRUE(s.starts_with("hello"));
	EXPECT_TRUE(s.starts_with('h'));
	EXPECT_FALSE(s.starts_with("world"));
	EXPECT_FALSE(s.starts_with('w'));
}

TEST(StaticStringTest, EndsWith)
{
	ach::static_string<32> s("hello world");
	EXPECT_TRUE(s.ends_with("world"));
	EXPECT_TRUE(s.ends_with('d'));
	EXPECT_FALSE(s.ends_with("hello"));
	EXPECT_FALSE(s.ends_with('h'));
}

TEST(StaticStringTest, Contains)
{
	ach::static_string<32> s("hello world");
	EXPECT_TRUE(s.contains("world"));
	EXPECT_TRUE(s.contains('o'));
	EXPECT_FALSE(s.contains("xyz"));
	EXPECT_FALSE(s.contains('z'));
}

TEST(StaticStringTest, Find)
{
	ach::static_string<32> s("hello world");
	EXPECT_EQ(s.find("world"), 6u);
	EXPECT_EQ(s.find('o'), 4u);
	EXPECT_EQ(s.find("xyz"), ach::static_string<32>::npos);
	EXPECT_EQ(s.find('z'), ach::static_string<32>::npos);
}

TEST(StaticStringTest, Rfind)
{
	ach::static_string<32> s("hello world");
	EXPECT_EQ(s.rfind('o'), 7u);
	EXPECT_EQ(s.rfind('l'), 9u);
	EXPECT_EQ(s.rfind('z'), ach::static_string<32>::npos);
}

TEST(StaticStringTest, Substr)
{
	ach::static_string<32> s("hello world");
	auto sub = s.substr(0, 5);
	EXPECT_EQ(sub, "hello");
	sub = s.substr(6);
	EXPECT_EQ(sub, "world");
}

TEST(StaticStringTest, EqualityOperator)
{
	ach::static_string<32> s1("test");
	ach::static_string<32> s2("test");
	ach::static_string<32> s3("different");
	EXPECT_TRUE(s1 == s2);
	EXPECT_FALSE(s1 == s3);
}

TEST(StaticStringTest, EqualityWithStringView)
{
	ach::static_string<32> s("test");
	EXPECT_TRUE(s == "test");
	EXPECT_FALSE(s == "other");
}

TEST(StaticStringTest, ThreeWayComparison)
{
	ach::static_string<32> s1("abc");
	ach::static_string<32> s2("abc");
	ach::static_string<32> s3("abd");
	EXPECT_EQ(s1 <=> s2, std::strong_ordering::equal);
	EXPECT_EQ(s1 <=> s3, std::strong_ordering::less);
	EXPECT_EQ(s3 <=> s1, std::strong_ordering::greater);
}

TEST(StaticStringTest, ConcatenationTwoStaticStrings)
{
	ach::static_string<16> s1("hello");
	ach::static_string<16> s2(" world");
	auto result = s1 + s2;
	EXPECT_EQ(result, "hello world");
}

TEST(StaticStringTest, ConcatenationWithCString)
{
	ach::static_string<16> s("hello");
	auto result = s + " world";
	EXPECT_EQ(result, "hello world");
}

TEST(StaticStringTest, ConcatenationWithChar)
{
	ach::static_string<16> s("hello");
	auto result = s + '!';
	EXPECT_EQ(result, "hello!");
}

TEST(StaticStringTest, StreamInsertion)
{
	ach::static_string<32> s("stream test");
	std::ostringstream oss;
	oss << s;
	EXPECT_EQ(oss.str(), "stream test");
}

TEST(StaticStringTest, LiteralOperator)
{
	auto s = "literal"_ss;
	EXPECT_EQ(s.size(), 7u);
	EXPECT_EQ(s, "literal");
}

TEST(StaticStringTest, Hash)
{
	ach::static_string<32> s1("hash test");
	ach::static_string<32> s2("hash test");
	ach::static_string<32> s3("different");
	std::hash<ach::static_string<32>> hasher;
	EXPECT_EQ(hasher(s1), hasher(s2));
	EXPECT_NE(hasher(s1), hasher(s3));
}

TEST(StaticStringTest, EmptyStringBehavior)
{
	ach::static_string<32> s("");
	EXPECT_TRUE(s.empty());
	EXPECT_EQ(s.size(), 0u);
	EXPECT_EQ(s.data()[0], '\0');
	EXPECT_EQ(s.c_str()[0], '\0');
}

TEST(StaticStringTest, MaxSize)
{
	ach::static_string<32> s;
	EXPECT_EQ(s.max_size(), 32u);
	EXPECT_EQ(s.capacity(), 32u);
}

// TEST(StaticStringTest, Constexpr)
// {
// 	constexpr ach::static_string<16> s("constexpr");
// 	static_assert(s.size() == 9);
// 	static_assert(s[0] == 'c');
// 	static_assert(!s.empty());
// }
//
// TEST(StaticStringTest, ConstexprOperations)
// {
// 	constexpr auto s1 = ach::static_string<16>("hello");
// 	constexpr auto s2 = ach::static_string<16>("world");
// 	static_assert(s1.size() == 5);
// 	static_assert(s2.size() == 5);
// 	static_assert(s1[0] == 'h');
// 	static_assert(s2[0] == 'w');
// }

TEST(StaticStringTest, CompareWithCString)
{
	ach::static_string<32> s("hello");
	EXPECT_EQ(s.compare("hello"), 0);
	EXPECT_LT(s.compare("world"), 0);
	EXPECT_GT(s.compare("abc"), 0);
}

TEST(StaticStringTest, FindWithPos)
{
	ach::static_string<32> s("hello hello");
	EXPECT_EQ(s.find("hello", 0), 0u);
	EXPECT_EQ(s.find("hello", 1), 6u);
	EXPECT_EQ(s.find("hello", 7), ach::static_string<32>::npos);
}

TEST(StaticStringTest, LongString)
{
	ach::static_string<128> s;
	for (int i = 0; i < 100; ++i)
		s.push_back('x');
	EXPECT_EQ(s.size(), 100u);
	EXPECT_TRUE(s.contains('x'));
	EXPECT_FALSE(s.contains('y'));
}

TEST(StaticStringTest, SpecialCharacters)
{
	ach::static_string<32> s;
	s += "tab\there";
	s += '\n';
	s += "newline";
	EXPECT_TRUE(s.contains('\t'));
	EXPECT_TRUE(s.contains('\n'));
}

TEST(StaticStringTest, RangeBasedFor)
{
	ach::static_string<32> s("loop");
	std::vector<char> chars;
	for (char c : s)
		chars.push_back(c);
	EXPECT_EQ(chars.size(), 4u);
	EXPECT_EQ(chars[0], 'l');
	EXPECT_EQ(chars[3], 'p');
}

TEST(StaticStringTest, DifferentCapacityComparison)
{
	ach::static_string<16> small("test");
	ach::static_string<32> large("test");
	EXPECT_EQ(small, large);
}

TEST(StaticStringTest, MultipleAppends)
{
	ach::static_string<64> s;
	s += "hello";
	s += ' ';
	s += "world";
	s += '!';
	EXPECT_EQ(s, "hello world!");
	EXPECT_EQ(s.size(), 12u);
}

TEST(StaticStringTest, ClearAndReuse)
{
	ach::static_string<32> s("first");
	EXPECT_EQ(s, "first");
	s.clear();
	EXPECT_TRUE(s.empty());
	s += "second";
	EXPECT_EQ(s, "second");
}

TEST(StaticStringTest, NullTerminatorPreserved)
{
	ach::static_string<32> s("test");
	EXPECT_EQ(s[s.size()], '\0');
	s.push_back('!');
	EXPECT_EQ(s[s.size()], '\0');
	s.pop_back();
	EXPECT_EQ(s[s.size()], '\0');
}
