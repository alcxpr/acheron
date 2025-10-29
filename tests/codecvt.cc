/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#include <acheron/codecvt.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace ach;

TEST(CodecvtTest, Utf8ToUtf16BasicAscii)
{
	std::string input = "hello";
	auto output = utf8_to_utf16(input);
	EXPECT_EQ(output.size(), 5);
	EXPECT_EQ(output, u"hello");
}

TEST(CodecvtTest, Utf8ToUtf32BasicAscii)
{
	std::string input = "world";
	auto output = utf8_to_utf32(input);
	EXPECT_EQ(output.size(), 5);
	EXPECT_EQ(output, U"world");
}

TEST(CodecvtTest, Utf16ToUtf8BasicAscii)
{
	std::u16string input = u"test";
	auto output = utf16_to_utf8(input);
	EXPECT_EQ(output.size(), 4);
	EXPECT_EQ(output, "test");
}

TEST(CodecvtTest, Utf16ToUtf32BasicAscii)
{
	std::u16string input = u"data";
	auto output = utf16_to_utf32(input);
	EXPECT_EQ(output.size(), 4);
	EXPECT_EQ(output, U"data");
}

TEST(CodecvtTest, Utf32ToUtf8BasicAscii)
{
	std::u32string input = U"code";
	auto output = utf32_to_utf8(input);
	EXPECT_EQ(output.size(), 4);
	EXPECT_EQ(output, "code");
}

TEST(CodecvtTest, Utf32ToUtf16BasicAscii)
{
	std::u32string input = U"func";
	auto output = utf32_to_utf16(input);
	EXPECT_EQ(output.size(), 4);
	EXPECT_EQ(output, u"func");
}

TEST(CodecvtTest, Utf8ToUtf16ChineseCharacters)
{
	std::string input = "你好";
	auto output = utf8_to_utf16(input);
	EXPECT_EQ(output.size(), 2);
	EXPECT_EQ(output, u"你好");
}

TEST(CodecvtTest, Utf8ToUtf32ChineseCharacters)
{
	std::string input = "世界";
	auto output = utf8_to_utf32(input);
	EXPECT_EQ(output.size(), 2);
	EXPECT_EQ(output, U"世界");
}

TEST(CodecvtTest, Utf8ToUtf16Emoji)
{
	std::string input = "🎉";
	auto output = utf8_to_utf16(input);
	EXPECT_EQ(output.size(), 2);
	EXPECT_EQ(output[0], 0xD83C);
	EXPECT_EQ(output[1], 0xDF89);
}

TEST(CodecvtTest, Utf8ToUtf32Emoji)
{
	std::string input = "🎉";
	auto output = utf8_to_utf32(input);
	EXPECT_EQ(output.size(), 1);
	EXPECT_EQ(output[0], 0x1F389);
}

TEST(CodecvtTest, Utf16ToUtf8Emoji)
{
	std::u16string input = u"🌟";
	auto output = utf16_to_utf8(input);
	EXPECT_EQ(output, "🌟");
}

TEST(CodecvtTest, Utf32ToUtf16Emoji)
{
	std::u32string input = U"🔥";
	auto output = utf32_to_utf16(input);
	EXPECT_EQ(output.size(), 2);
	EXPECT_EQ(output, u"🔥");
}

TEST(CodecvtTest, Utf32ToUtf8Emoji)
{
	std::u32string input = U"🌟";
	auto output = utf32_to_utf8(input);
	EXPECT_EQ(output, "🌟");
}

TEST(CodecvtTest, Utf8ToUtf16MixedContent)
{
	std::string input = "hello世界🎉";
	auto output = utf8_to_utf16(input);
	EXPECT_EQ(output, u"hello世界🎉");
}

TEST(CodecvtTest, Utf8ToUtf32MixedContent)
{
	std::string input = "test测试🔥";
	auto output = utf8_to_utf32(input);
	EXPECT_EQ(output, U"test测试🔥");
}

TEST(CodecvtTest, Utf16ToUtf8MixedContent)
{
	std::u16string input = u"café🌟";
	auto output = utf16_to_utf8(input);
	EXPECT_EQ(output, "café🌟");
}

TEST(CodecvtTest, Utf32ToUtf8MixedContent)
{
	std::u32string input = U"Ñoño🎉";
	auto output = utf32_to_utf8(input);
	EXPECT_EQ(output, "Ñoño🎉");
}

TEST(CodecvtTest, Utf8ToUtf16AccentedCharacters)
{
	std::string input = "café";
	auto output = utf8_to_utf16(input);
	EXPECT_EQ(output, u"café");
}

TEST(CodecvtTest, Utf8ToUtf32AccentedCharacters)
{
	std::string input = "Ñoño";
	auto output = utf8_to_utf32(input);
	EXPECT_EQ(output, U"Ñoño");
}

TEST(CodecvtTest, Utf16ToUtf32AccentedCharacters)
{
	std::u16string input = u"naïve";
	auto output = utf16_to_utf32(input);
	EXPECT_EQ(output, U"naïve");
}

TEST(CodecvtTest, Utf32ToUtf16AccentedCharacters)
{
	std::u32string input = U"résumé";
	auto output = utf32_to_utf16(input);
	EXPECT_EQ(output, u"résumé");
}

TEST(CodecvtTest, Utf8ToUtf16RussianCyrillic)
{
	std::string input = "тест";
	auto output = utf8_to_utf16(input);
	EXPECT_EQ(output, u"тест");
}

TEST(CodecvtTest, Utf8ToUtf32ArabicScript)
{
	std::string input = "العربية";
	auto output = utf8_to_utf32(input);
	EXPECT_EQ(output, U"العربية");
}

TEST(CodecvtTest, Utf8ToUtf16JapaneseKanji)
{
	std::string input = "日本語";
	auto output = utf8_to_utf16(input);
	EXPECT_EQ(output, u"日本語");
}

TEST(CodecvtTest, RoundtripUtf8Utf16Utf8)
{
	std::string original = "hello世界🎉café";
	auto utf16 = utf8_to_utf16(original);
	auto back = utf16_to_utf8(utf16);
	EXPECT_EQ(back, original);
}

TEST(CodecvtTest, RoundtripUtf8Utf32Utf8)
{
	std::string original = "test测试🔥Ñoño";
	auto utf32 = utf8_to_utf32(original);
	auto back = utf32_to_utf8(utf32);
	EXPECT_EQ(back, original);
}

TEST(CodecvtTest, RoundtripUtf16Utf32Utf16)
{
	std::u16string original = u"data🌟тест";
	auto utf32 = utf16_to_utf32(original);
	auto back = utf32_to_utf16(utf32);
	EXPECT_EQ(back, original);
}

TEST(CodecvtTest, EmptyStringUtf8ToUtf16)
{
	std::string input = "";
	auto output = utf8_to_utf16(input);
	EXPECT_TRUE(output.empty());
}

TEST(CodecvtTest, EmptyStringUtf8ToUtf32)
{
	std::string input = "";
	auto output = utf8_to_utf32(input);
	EXPECT_TRUE(output.empty());
}

TEST(CodecvtTest, EmptyStringUtf16ToUtf8)
{
	std::u16string input = u"";
	auto output = utf16_to_utf8(input);
	EXPECT_TRUE(output.empty());
}

TEST(CodecvtTest, SingleCharacterUtf8ToUtf16)
{
	std::string input = "A";
	auto output = utf8_to_utf16(input);
	EXPECT_EQ(output.size(), 1);
	EXPECT_EQ(output[0], u'A');
}

TEST(CodecvtTest, SingleEmojiUtf32ToUtf8)
{
	std::u32string input = U"😀";
	auto output = utf32_to_utf8(input);
	EXPECT_EQ(output, "😀");
}

TEST(CodecvtTest, MultipleEmojisUtf8ToUtf32)
{
	std::string input = "🎉🔥🌟";
	auto output = utf8_to_utf32(input);
	EXPECT_EQ(output.size(), 3);
	EXPECT_EQ(output, U"🎉🔥🌟");
}

TEST(CodecvtTest, LongMixedStringUtf8ToUtf16)
{
	std::string input = "The quick brown fox jumps over the lazy dog. 你好世界! 🎉🔥 café Ñoño тест";
	auto output = utf8_to_utf16(input);
	auto back = utf16_to_utf8(output);
	EXPECT_EQ(back, input);
}

TEST(CodecvtTest, ComplexMultiscriptString)
{
	std::string input = "mixed🔥Latin中文العربية日本語";
	auto utf16 = utf8_to_utf16(input);
	auto utf32 = utf8_to_utf32(input);
	auto back16 = utf16_to_utf8(utf16);
	auto back32 = utf32_to_utf8(utf32);
	EXPECT_EQ(back16, input);
	EXPECT_EQ(back32, input);
}
