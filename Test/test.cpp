#include "pch.h"
#include "../matrix_3_0/matrix.hpp"

#include <array>

template<typename T>
bool all_of_equal_to(const matrix<T>& mtx, const T& val)
{
	for (std::size_t row = 0; row < mtx.size().rows; ++row) {
		for (std::size_t col = 0; col < mtx.size().cols; ++col) {
			if (mtx[row][col] != val) {
				return false;
			}
		}
	}
	return true;
}

TEST(MatrixSizeType, Test) {
	EXPECT_EQ(matrix_size_type(), matrix_size_type(0, 0));
	EXPECT_TRUE(matrix_size_type(0, 0) < matrix_size_type(1, 5));
	EXPECT_FALSE(matrix_size_type(3, 0) > matrix_size_type(5, 5));
	EXPECT_TRUE(matrix_size_type(3, 0) <= matrix_size_type(3, 0));
	EXPECT_TRUE(matrix_size_type(3, 0) >= matrix_size_type(1, 0));
}

TEST(Construction, DefaultConstruct) {
	matrix<int> mtx;
	EXPECT_EQ(mtx.size(), matrix_size_type());
	EXPECT_EQ(mtx.size(), mtx.capacity());
	EXPECT_TRUE(mtx.empty());
	EXPECT_EQ(mtx.data(), nullptr);
}

TEST(Construction, ConstructWithSizeDefault) {
	constexpr auto rows_count = 3;
	constexpr auto cols_count = 5;

	matrix<int> mtx(rows_count, cols_count);

	EXPECT_EQ(mtx.size(), matrix_size_type(rows_count, cols_count));
	EXPECT_EQ(mtx.size(), mtx.capacity());
	EXPECT_NE(mtx.data(), nullptr);
	EXPECT_FALSE(mtx.empty());
	EXPECT_TRUE(all_of_equal_to(mtx, 0));

	mtx.clear();
	EXPECT_EQ(mtx.data(), nullptr);
	EXPECT_TRUE(mtx.empty());
}

TEST(Construction, ConstructWithSizeAndDefaultValue) {
	constexpr auto rows_count = 3;
	constexpr auto cols_count = 5;
	constexpr auto default_value = 3;

	matrix<int> mtx(rows_count, cols_count, default_value);

	EXPECT_EQ(mtx.size(), matrix_size_type(rows_count, cols_count));
	EXPECT_EQ(mtx.size(), mtx.capacity());
	EXPECT_NE(mtx.data(), nullptr);
	EXPECT_FALSE(mtx.empty());
	EXPECT_TRUE(all_of_equal_to(mtx, default_value));

	mtx.clear();
	EXPECT_EQ(mtx.data(), nullptr);
	EXPECT_TRUE(mtx.empty());
}

TEST(Construction, ConstructFromRange) {
	constexpr auto cols_count = 4;
	constexpr std::array<int, 12> arr = { 1,2,3,4,5,6,7,8,9,10,11,12 };

	matrix<int> mtx(cols_count, std::cbegin(arr), std::cend(arr));
	EXPECT_EQ(mtx.size().rows * mtx.size().cols, std::size(arr));
	EXPECT_EQ(mtx.size().cols, cols_count);
	EXPECT_EQ(mtx.size().rows, std::size(arr) / cols_count);
	EXPECT_EQ(mtx.size(), mtx.capacity());

	for (std::size_t row = 0; row < mtx.size().rows; ++row) {
		for (std::size_t col = 0; col < mtx.size().cols; ++col) {
			EXPECT_EQ(mtx[row][col], mtx(row, col));
			EXPECT_EQ(mtx[row][col], arr.at(row * cols_count + col));
		}
	}
}
