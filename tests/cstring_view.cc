/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#include <acheron/cstring_view.hpp>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>
#include <format>

using namespace ach::literals;

TEST(CStringViewTest, DefaultConstruction)
{
	ach::cstring_view csv;
	EXPECT_EQ(csv.size(), 0u);
	EXPECT_TRUE(csv.empty());
	EXPECT_NE(csv.data(), nullptr);
	EXPECT_EQ(csv.data()[0], '\0');
}

TEST(CStringViewTest, ConstructFromCString)
{
	const char* str = "hello";
	ach::cstring_view csv(str);
	EXPECT_EQ(csv.size(), 5u);
	EXPECT_FALSE(csv.empty());
	EXPECT_EQ(csv, "hello");
	EXPECT_EQ(csv.data()[5], '\0');
}

TEST(CStringViewTest, ConstructFromCStringWithLength)
{
	const char str[] = "world\0extra";
	ach::cstring_view csv(str, 5);
	EXPECT_EQ(csv.size(), 5u);
	EXPECT_EQ(csv, "world");
	EXPECT_EQ(csv.data()[5], '\0');
}

TEST(CStringViewTest, ConstructFromStdString)
{
	std::string str = "test string";
	ach::cstring_view csv(str);
	EXPECT_EQ(csv.size(), str.size());
	EXPECT_EQ(csv, str);
	EXPECT_EQ(csv.c_str(), str.c_str());
}

TEST(CStringViewTest, CopyConstruction)
{
	ach::cstring_view csv1("original");
	ach::cstring_view csv2(csv1);
	EXPECT_EQ(csv1, csv2);
	EXPECT_EQ(csv1.data(), csv2.data());
}

TEST(CStringViewTest, CopyAssignment)
{
	ach::cstring_view csv1("first");
	ach::cstring_view csv2("second");
	csv2 = csv1;
	EXPECT_EQ(csv1, csv2);
	EXPECT_EQ(csv1.data(), csv2.data());
}

TEST(CStringViewTest, Iterators)
{
	ach::cstring_view csv("test");
	std::string result;
	for (auto c : csv)
		result += c;
	EXPECT_EQ(result, "test");
}

TEST(CStringViewTest, ReverseIterators)
{
	ach::cstring_view csv("abc");
	std::string result;
	for (auto it = csv.rbegin(); it != csv.rend(); ++it)
		result += *it;
	EXPECT_EQ(result, "cba");
}

TEST(CStringViewTest, ElementAccess)
{
	ach::cstring_view csv("hello");
	EXPECT_EQ(csv[0], 'h');
	EXPECT_EQ(csv[4], 'o');
	EXPECT_EQ(csv[5], '\0');
	EXPECT_EQ(csv.front(), 'h');
	EXPECT_EQ(csv.back(), 'o');
}

TEST(CStringViewTest, ElementAccessAt)
{
	ach::cstring_view csv("test");
	EXPECT_EQ(csv.at(0), 't');
	EXPECT_EQ(csv.at(3), 't');
	EXPECT_EQ(csv.at(4), '\0');
	EXPECT_THROW(csv.at(5), std::out_of_range);
	EXPECT_THROW(csv.at(100), std::out_of_range);
}

TEST(CStringViewTest, DataAndCStr)
{
	const char* str = "data test";
	ach::cstring_view csv(str);
	EXPECT_EQ(csv.data(), str);
	EXPECT_EQ(csv.c_str(), str);
	EXPECT_STREQ(csv.c_str(), "data test");
}

TEST(CStringViewTest, ConversionToStringView)
{
	ach::cstring_view csv("convert");
	std::string_view sv = csv;
	EXPECT_EQ(sv.size(), csv.size());
	EXPECT_EQ(sv.data(), csv.data());
	EXPECT_EQ(sv, "convert");
}

TEST(CStringViewTest, RemovePrefix)
{
	ach::cstring_view csv("hello world");
	csv.remove_prefix(6);
	EXPECT_EQ(csv, "world");
	EXPECT_EQ(csv.size(), 5u);
}

TEST(CStringViewTest, Swap)
{
	ach::cstring_view csv1("first");
	ach::cstring_view csv2("second");
	csv1.swap(csv2);
	EXPECT_EQ(csv1, "second");
	EXPECT_EQ(csv2, "first");
}

TEST(CStringViewTest, SwapFreeFunction)
{
	ach::cstring_view csv1("alpha");
	ach::cstring_view csv2("beta");
	swap(csv1, csv2);
	EXPECT_EQ(csv1, "beta");
	EXPECT_EQ(csv2, "alpha");
}

TEST(CStringViewTest, Copy)
{
	ach::cstring_view csv("hello world");
	char buffer[20] = {};
	auto count = csv.copy(buffer, 5, 6);
	EXPECT_EQ(count, 5u);
	EXPECT_STREQ(buffer, "world");
}

TEST(CStringViewTest, Substr)
{
	ach::cstring_view csv("hello world");
	auto sub = csv.substr(0, 5);
	EXPECT_EQ(sub, "hello");
	sub = csv.substr(6);
	EXPECT_EQ(sub, "world");
}

TEST(CStringViewTest, Compare)
{
	ach::cstring_view csv1("abc");
	ach::cstring_view csv2("abc");
	ach::cstring_view csv3("abd");
	EXPECT_EQ(csv1.compare(csv2), 0);
	EXPECT_LT(csv1.compare(csv3), 0);
	EXPECT_GT(csv3.compare(csv1), 0);
}

TEST(CStringViewTest, StartsWith)
{
	ach::cstring_view csv("hello world");
	EXPECT_TRUE(csv.starts_with("hello"));
	EXPECT_TRUE(csv.starts_with('h'));
	EXPECT_FALSE(csv.starts_with("world"));
	EXPECT_FALSE(csv.starts_with('w'));
}

TEST(CStringViewTest, EndsWith)
{
	ach::cstring_view csv("hello world");
	EXPECT_TRUE(csv.ends_with("world"));
	EXPECT_TRUE(csv.ends_with('d'));
	EXPECT_FALSE(csv.ends_with("hello"));
	EXPECT_FALSE(csv.ends_with('h'));
}

TEST(CStringViewTest, Contains)
{
	ach::cstring_view csv("hello world");
	EXPECT_TRUE(csv.contains("world"));
	EXPECT_TRUE(csv.contains('o'));
	EXPECT_FALSE(csv.contains("xyz"));
	EXPECT_FALSE(csv.contains('z'));
}

TEST(CStringViewTest, Find)
{
	ach::cstring_view csv("hello world");
	EXPECT_EQ(csv.find("world"), 6u);
	EXPECT_EQ(csv.find('o'), 4u);
	EXPECT_EQ(csv.find("xyz"), ach::cstring_view::npos);
	EXPECT_EQ(csv.find('z'), ach::cstring_view::npos);
}

TEST(CStringViewTest, Rfind)
{
	ach::cstring_view csv("hello world");
	EXPECT_EQ(csv.rfind('o'), 7u);
	EXPECT_EQ(csv.rfind('l'), 9u);
	EXPECT_EQ(csv.rfind('z'), ach::cstring_view::npos);
}

TEST(CStringViewTest, FindFirstOf)
{
	ach::cstring_view csv("hello world");
	EXPECT_EQ(csv.find_first_of("aeiou"), 1u);
	EXPECT_EQ(csv.find_first_of('w'), 6u);
	EXPECT_EQ(csv.find_first_of("xyz"), ach::cstring_view::npos);
}

TEST(CStringViewTest, FindLastOf)
{
	ach::cstring_view csv("hello world");
	EXPECT_EQ(csv.find_last_of("aeiou"), 7u);
	EXPECT_EQ(csv.find_last_of('l'), 9u);
	EXPECT_EQ(csv.find_last_of("xyz"), ach::cstring_view::npos);
}

TEST(CStringViewTest, FindFirstNotOf)
{
	ach::cstring_view csv("aaabbbccc");
	EXPECT_EQ(csv.find_first_not_of('a'), 3u);
	EXPECT_EQ(csv.find_first_not_of("ab"), 6u);
}

TEST(CStringViewTest, FindLastNotOf)
{
	ach::cstring_view csv("aaabbbccc");
	EXPECT_EQ(csv.find_last_not_of('c'), 5u);
	EXPECT_EQ(csv.find_last_not_of("bc"), 2u);
}

TEST(CStringViewTest, EqualityOperator)
{
	ach::cstring_view csv1("test");
	ach::cstring_view csv2("test");
	ach::cstring_view csv3("different");
	EXPECT_TRUE(csv1 == csv2);
	EXPECT_FALSE(csv1 == csv3);
}

TEST(CStringViewTest, ThreeWayComparison)
{
	ach::cstring_view csv1("abc");
	ach::cstring_view csv2("abc");
	ach::cstring_view csv3("abd");
	EXPECT_EQ(csv1 <=> csv2, std::strong_ordering::equal);
	EXPECT_EQ(csv1 <=> csv3, std::strong_ordering::less);
	EXPECT_EQ(csv3 <=> csv1, std::strong_ordering::greater);
}

TEST(CStringViewTest, StreamInsertion)
{
	ach::cstring_view csv("stream test");
	std::ostringstream oss;
	oss << csv;
	EXPECT_EQ(oss.str(), "stream test");
}

TEST(CStringViewTest, LiteralOperator)
{
	auto csv = "literal"_csv;
	EXPECT_EQ(csv.size(), 7u);
	EXPECT_EQ(csv, "literal");
	EXPECT_EQ(csv.data()[7], '\0');
}

TEST(CStringViewTest, U8Literal)
{
	auto csv = u8"utf8"_csv;
	EXPECT_EQ(csv.size(), 4u);
	EXPECT_EQ(csv.data()[4], u8'\0');
}

TEST(CStringViewTest, U16Literal)
{
	auto csv = u"utf16"_csv;
	EXPECT_EQ(csv.size(), 5u);
	EXPECT_EQ(csv.data()[5], u'\0');
}

TEST(CStringViewTest, U32Literal)
{
	auto csv = U"utf32"_csv;
	EXPECT_EQ(csv.size(), 5u);
	EXPECT_EQ(csv.data()[5], U'\0');
}

TEST(CStringViewTest, WideLiteral)
{
	auto csv = L"wide"_csv;
	EXPECT_EQ(csv.size(), 4u);
	EXPECT_EQ(csv.data()[4], L'\0');
}

TEST(CStringViewTest, Hash)
{
	ach::cstring_view csv1("hash test");
	ach::cstring_view csv2("hash test");
	ach::cstring_view csv3("different");
	std::hash<ach::cstring_view> hasher;
	EXPECT_EQ(hasher(csv1), hasher(csv2));
	EXPECT_NE(hasher(csv1), hasher(csv3));
}

TEST(CStringViewTest, EmptyStringBehavior)
{
	ach::cstring_view csv("");
	EXPECT_TRUE(csv.empty());
	EXPECT_EQ(csv.size(), 0u);
	EXPECT_EQ(csv.data()[0], '\0');
	EXPECT_EQ(csv.c_str()[0], '\0');
}

TEST(CStringViewTest, MaxSize)
{
	ach::cstring_view csv;
	EXPECT_GT(csv.max_size(), 0u);
}

TEST(CStringViewTest, NullTerminatorAccess)
{
	ach::cstring_view csv("test");
	EXPECT_EQ(csv[csv.size()], '\0');
	EXPECT_EQ(csv.data()[csv.size()], '\0');
}

TEST(CStringViewTest, CompareWithCString)
{
	ach::cstring_view csv("hello");
	EXPECT_EQ(csv.compare("hello"), 0);
	EXPECT_LT(csv.compare("world"), 0);
	EXPECT_GT(csv.compare("abc"), 0);
}

TEST(CStringViewTest, SubstrOutOfRange)
{
	ach::cstring_view csv("test");
	EXPECT_THROW(csv.substr(10), std::out_of_range);
}

TEST(CStringViewTest, CopyPartial)
{
	ach::cstring_view csv("0123456789");
	char buffer[5] = {};
	auto count = csv.copy(buffer, 3, 2);
	EXPECT_EQ(count, 3u);
	EXPECT_STREQ(buffer, "234");
}

TEST(CStringViewTest, RangeBasedFor)
{
	ach::cstring_view csv("loop");
	std::vector<char> chars;
	for (char c : csv)
		chars.push_back(c);
	EXPECT_EQ(chars.size(), 4u);
	EXPECT_EQ(chars[0], 'l');
	EXPECT_EQ(chars[3], 'p');
}

TEST(CStringViewTest, ConstructFromDifferentCapacity)
{
	const char str[] = "immutable";
	ach::cstring_view csv1(str);
	ach::cstring_view csv2(csv1);
	EXPECT_EQ(csv1, csv2);
}

TEST(CStringViewTest, CompareDifferentLengths)
{
	ach::cstring_view short_csv("abc");
	ach::cstring_view long_csv("abcdef");
	EXPECT_LT(short_csv.compare(long_csv), 0);
	EXPECT_GT(long_csv.compare(short_csv), 0);
}

TEST(CStringViewTest, FindWithPos)
{
	ach::cstring_view csv("hello hello");
	EXPECT_EQ(csv.find("hello", 0), 0u);
	EXPECT_EQ(csv.find("hello", 1), 6u);
	EXPECT_EQ(csv.find("hello", 7), ach::cstring_view::npos);
}

TEST(CStringViewTest, LongString)
{
	std::string long_str(1000, 'x');
	ach::cstring_view csv(long_str);
	EXPECT_EQ(csv.size(), 1000u);
	EXPECT_TRUE(csv.contains('x'));
	EXPECT_FALSE(csv.contains('y'));
}

TEST(CStringViewTest, SpecialCharacters)
{
	ach::cstring_view csv("tab\there\nnewline\0embedded");
	EXPECT_TRUE(csv.contains('\t'));
	EXPECT_TRUE(csv.contains('\n'));
	EXPECT_EQ(csv.size(), 16u);
}

TEST(CStringViewTest, CompareSubstring)
{
	ach::cstring_view csv("hello world");
	EXPECT_EQ(csv.compare(0, 5, "hello"), 0);
	EXPECT_EQ(csv.compare(6, 5, "world"), 0);
}
