/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#include <acheron/bitfield.hpp>
#include <bit>
#include <cstdint>
#include <gtest/gtest.h>

TEST(BitfieldTest, DefaultConstructionIsZero)
{
	constexpr ach::bitfield<std::uint32_t> bf;
	static_assert(bf.raw() == 0);
	EXPECT_EQ(bf.raw(), 0);
}

TEST(BitfieldTest, ConstructFromValue)
{
	constexpr ach::bitfield<std::uint8_t> bf(0b10101010);
	static_assert(bf.raw() == 0b10101010);
	EXPECT_EQ(bf.raw(), 0b10101010);
}

TEST(BitfieldTest, SetAndGetSingleBit)
{
	ach::bitfield<std::uint8_t> bf;
	bf.set<3>(1);
	EXPECT_EQ(bf.get<3>(), 1);
	EXPECT_EQ(bf.raw(), 0b00001000);
}

TEST(BitfieldTest, SetAndGetMultiBitField)
{
	ach::bitfield<std::uint16_t> bf;
	bf.set<4, 4>(0b1010);
	EXPECT_EQ(bf.raw(), 0b10100000);
}

TEST(BitfieldTest, ClearBits)
{
	ach::bitfield<std::uint8_t> bf(0xFF);
	bf.clear<4, 2>();
	EXPECT_EQ(bf.raw(), 0b11001111);
}

TEST(BitfieldTest, TestBits)
{
	constexpr ach::bitfield<std::uint8_t> bf(0b00110000);
	static_assert(bf.test<4, 2>());
}

TEST(BitfieldTest, FlipBits)
{
	ach::bitfield<std::uint8_t> bf(0b00001111);
	bf.flip<0, 8>();
	EXPECT_EQ(bf.raw(), static_cast<std::uint8_t>(~0b00001111));
}

TEST(BitfieldTest, BitwiseOperations)
{
	ach::bitfield<std::uint8_t> a(0b10101010);
	ach::bitfield<std::uint8_t> b(0b11001100);

	EXPECT_EQ((a & b).raw(), 0b10001000);
	EXPECT_EQ((a | b).raw(), 0b11101110);
	EXPECT_EQ((a ^ b).raw(), 0b01100110);

	ach::bitfield<std::uint8_t> c = ~a;
	EXPECT_EQ(c.raw(), static_cast<std::uint8_t>(~0b10101010));
}

TEST(BitfieldTest, CompoundAssignment)
{
	ach::bitfield<std::uint8_t> a(0b10101010);
	ach::bitfield<std::uint8_t> b(0b01010101);

	a &= b;
	EXPECT_EQ(a.raw(), 0);
	a |= b;
	EXPECT_EQ(a.raw(), 0b01010101);
	a ^= b;
	EXPECT_EQ(a.raw(), 0);
}

TEST(BitfieldTest, ToEndianSwapsCorrectly)
{
	constexpr ach::bitfield<std::uint32_t> bf(0x11223344);
	constexpr auto swapped = bf.to_endian<std::endian::big>();
	if constexpr (std::endian::native == std::endian::little)
		EXPECT_EQ(swapped.raw(), 0x44332211u);
	else
		EXPECT_EQ(swapped.raw(), 0x11223344u);
}

TEST(BitfieldTest, ConstexprChaining)
{
	constexpr auto bf = ach::bitfield<std::uint16_t>().set<0, 3>(0b101).set<8, 4>(0b1111).flip<0, 3>();

	static_assert(bf.get<8, 4>() == 0b1111);
	static_assert(bf.get<0, 3>() == 0b010);
}

TEST(BitfieldTest, MaskEdgeCaseFullWidth)
{
	constexpr ach::bitfield<std::uint8_t> bf(0xAA);
	constexpr auto mask = bf.get<0, 8>();
	static_assert(mask == 0xAA);
	EXPECT_EQ(mask, 0xAA);
}
