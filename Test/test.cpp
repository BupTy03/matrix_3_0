#include "pch.h"
#include "../matrix_3_0/matrix.hpp"

#include <array>
#include <numeric>

template<typename T>
void ExpectAllEqualTo(const matrix<T>& mtx, const T& val) {
	for (std::size_t row = 0; row < mtx.size().rows; ++row) {
		for (std::size_t col = 0; col < mtx.size().cols; ++col) {
			EXPECT_EQ(mtx[row][col], val);
			EXPECT_EQ(mtx[row][col], mtx(row, col));
		}
	}
}

template<typename T>
void ExpectEqualState(const matrix<T>& lhs, const matrix<T>& rhs) {
	EXPECT_EQ(lhs.size(), rhs.size());
	EXPECT_EQ(lhs.capacity(), rhs.capacity());
	EXPECT_EQ(lhs.empty(), rhs.empty());
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
	ExpectAllEqualTo(mtx, int{});

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
	ExpectAllEqualTo(mtx, default_value);

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

TEST(Construction, ConstructFromInitializerList) {
	constexpr auto cols_count = 4;

	const auto init_list = { 1,2,3,9,10,11,124,5,6,7,8,0 };

	matrix<int> mtx(cols_count, init_list);
	EXPECT_EQ(mtx.size().rows * mtx.size().cols, std::size(init_list));
	EXPECT_EQ(mtx.size().cols, cols_count);
	EXPECT_EQ(mtx.size().rows, std::size(init_list) / cols_count);
	EXPECT_EQ(mtx.size(), mtx.capacity());

	auto init_list_it = std::begin(init_list);
	for (std::size_t row = 0; row < mtx.size().rows; ++row) {
		for (std::size_t col = 0; col < mtx.size().cols; ++col) {
			EXPECT_EQ(mtx[row][col], mtx(row, col));
			EXPECT_EQ(mtx[row][col], (*init_list_it));
			++init_list_it;
		}
	}
}

TEST(Construction, CopyConstruction) {
	// empty matrix copy construction
	matrix<int> empty_mtx1;
	EXPECT_EQ(empty_mtx1.size(), matrix_size_type());
	EXPECT_EQ(empty_mtx1.size(), empty_mtx1.capacity());
	EXPECT_TRUE(empty_mtx1.empty());
	EXPECT_EQ(empty_mtx1.data(), nullptr);

	auto empty_mtx2 = empty_mtx1;
	ExpectEqualState(empty_mtx1, empty_mtx2);

	// non-empty matrix copy construction
	matrix<int> mtx(3, { 1, 2, 3, 4, 5, 6, 7, 8, 9 });
	EXPECT_EQ(mtx.size(), mtx.capacity());

	auto mtx2 = mtx;
	EXPECT_EQ(mtx2.size(), mtx2.capacity());
	ExpectEqualState(mtx, mtx2);
}

TEST(Assignment, CopyAssignment) {
	// empty matrix copy assignment
	matrix<int> empty_mtx1;
	EXPECT_EQ(empty_mtx1.size(), matrix_size_type());
	EXPECT_EQ(empty_mtx1.size(), empty_mtx1.capacity());
	EXPECT_TRUE(empty_mtx1.empty());
	EXPECT_EQ(empty_mtx1.data(), nullptr);

	matrix<int> empty_mtx2;
	empty_mtx2 = empty_mtx1;
	ExpectEqualState(empty_mtx1, empty_mtx2);

	// non empty matrix copy assignment
	matrix<int> mtx(3, { 1, 2, 3, 4, 5, 6, 7, 8, 9 });
	EXPECT_EQ(mtx.size(), mtx.capacity());

	matrix<int> mtx2;
	mtx2 = mtx;
	EXPECT_EQ(mtx2.size(), mtx2.capacity());
	ExpectEqualState(mtx, mtx2);

	// non empty matrix to non empty matrix copy assignment
	std::array<int, 50> arr = { 0 };
	std::iota(std::begin(arr), std::end(arr), 1);
	matrix<int> mtx3(5, std::cbegin(arr), std::cend(arr));
	EXPECT_EQ(mtx3.size(), mtx3.capacity());
	
	std::array<int, 10> arr2 = { 0 };
	std::iota(std::rbegin(arr), std::rend(arr), 20);
	matrix<int> mtx4(5, std::cbegin(arr), std::cend(arr));
	EXPECT_EQ(mtx4.size(), mtx4.capacity());

	mtx4 = mtx3;
	EXPECT_EQ(mtx4.size(), mtx4.capacity());
	ExpectEqualState(mtx3, mtx4);
}

TEST(Construction, MoveConstruction) {
	// empty matrix copy construction
	matrix<int> empty_mtx1;
	EXPECT_EQ(empty_mtx1.size(), matrix_size_type());
	EXPECT_EQ(empty_mtx1.size(), empty_mtx1.capacity());
	EXPECT_TRUE(empty_mtx1.empty());
	EXPECT_EQ(empty_mtx1.data(), nullptr);

	matrix<int> empty_mtx2 = std::move(empty_mtx1);
	ExpectEqualState(empty_mtx1, empty_mtx2);

	// non-empty matrix copy construction
	matrix<int> mtx(3, { 1, 2, 3, 4, 5, 6, 7, 8, 9 });
	EXPECT_EQ(mtx.size(), mtx.capacity());

	const auto mtx_data = mtx.data();
	matrix<int> tmp_mtx = mtx;
	matrix<int> mtx2 = std::move(mtx);

	ExpectEqualState(mtx2, tmp_mtx);
	ExpectEqualState(mtx, matrix<int>());
	EXPECT_EQ(mtx2.data(), mtx_data);
}

TEST(Assignment, MoveAssignment) {
	// empty matrix copy assignment
	matrix<int> empty_mtx1;
	EXPECT_EQ(empty_mtx1.size(), matrix_size_type());
	EXPECT_EQ(empty_mtx1.size(), empty_mtx1.capacity());
	EXPECT_TRUE(empty_mtx1.empty());
	EXPECT_EQ(empty_mtx1.data(), nullptr);

	matrix<int> empty_mtx2;
	EXPECT_NO_THROW(empty_mtx2 = std::move(empty_mtx1));
	EXPECT_EQ(empty_mtx1.data(), empty_mtx2.data());
	EXPECT_EQ(empty_mtx1.data(), nullptr);
	ExpectEqualState(empty_mtx1, matrix<int>());
	ExpectEqualState(empty_mtx1, empty_mtx2);

	// non-empty matrix copy assignment
	matrix<int> mtx(3, { 1, 2, 3, 4, 5, 6, 7, 8, 9 });
	EXPECT_EQ(mtx.size(), mtx.capacity());

	matrix<int> mtx2;
	const auto mtx_data = mtx.data();
	matrix<int> tmp_mtx = mtx;
	EXPECT_NO_THROW(mtx2 = std::move(mtx));
	EXPECT_EQ(mtx2.data(), mtx_data);
	EXPECT_EQ(mtx2.size(), mtx2.capacity());
	ExpectEqualState(tmp_mtx, mtx2);
	ExpectEqualState(mtx, matrix<int>());

	// non-empty matrix to non empty matrix copy assignment
	std::array<int, 50> arr = { 0 };
	std::iota(std::begin(arr), std::end(arr), 1);
	matrix<int> mtx3(5, std::cbegin(arr), std::cend(arr));
	EXPECT_EQ(mtx3.size(), mtx3.capacity());

	std::array<int, 10> arr2 = { 0 };
	std::iota(std::rbegin(arr2), std::rend(arr2), 20);
	matrix<int> mtx4(5, std::cbegin(arr2), std::cend(arr2));
	EXPECT_EQ(mtx4.size(), mtx4.capacity());

	const auto mtx3_data = mtx3.data();
	matrix<int> tmp_mtx3 = mtx3;
	matrix<int> tmp_mtx4 = mtx4;
	EXPECT_NO_THROW(mtx4 = std::move(mtx3));
	EXPECT_EQ(mtx4.data(), mtx3_data);
	EXPECT_EQ(mtx4.size(), mtx4.capacity());
	ExpectEqualState(tmp_mtx3, mtx4);
	ExpectEqualState(mtx3, tmp_mtx4);
}

TEST(Swap, UsualSwap) {

	// swap non-empty matrix with empty matrix
	std::array<int, 50> arr = { 0 };
	std::iota(std::begin(arr), std::end(arr), 1);
	matrix<int> mtx(5, std::cbegin(arr), std::cend(arr));
	EXPECT_EQ(mtx.size(), mtx.capacity());

	const auto mtx_size = mtx.size();
	const auto mtx_capacity = mtx.capacity();
	const auto mtx_data = mtx.data();

	matrix<int> mtx2;
	EXPECT_NO_THROW(mtx.swap(mtx2));
	EXPECT_EQ(mtx2.size(), mtx_size);
	EXPECT_EQ(mtx2.capacity(), mtx_capacity);
	EXPECT_EQ(mtx2.data(), mtx_data);

	ExpectEqualState(mtx, matrix<int>());

	// swap two non-empty matrixes
	std::array<int, 50> arr2 = { 0 };
	std::iota(std::begin(arr2), std::end(arr2), 1);
	matrix<int> mtx3(5, std::cbegin(arr2), std::cend(arr2));
	EXPECT_EQ(mtx3.size(), mtx3.capacity());

	std::array<int, 10> arr3 = { 0 };
	std::iota(std::rbegin(arr3), std::rend(arr3), 20);
	matrix<int> mtx4(5, std::cbegin(arr3), std::cend(arr3));
	EXPECT_EQ(mtx4.size(), mtx4.capacity());

	const auto mtx3_data = mtx3.data();
	matrix<int> tmp_mtx3 = mtx3;
	matrix<int> tmp_mtx4 = mtx4;
	EXPECT_NO_THROW(mtx4 = std::move(mtx3));
	EXPECT_EQ(mtx4.data(), mtx3_data);
	EXPECT_EQ(mtx4.size(), mtx4.capacity());
	ExpectEqualState(tmp_mtx3, mtx4);
	ExpectEqualState(mtx3, tmp_mtx4);
}

TEST(Clear, UsualClear) {
	std::array<int, 50> arr = { 0 };
	std::iota(std::begin(arr), std::end(arr), 1);
	matrix<int> mtx(5, std::cbegin(arr), std::cend(arr));
	EXPECT_EQ(mtx.size(), mtx.capacity());

	mtx.clear();
	ExpectEqualState(mtx, matrix<int>());
}

TEST(AccessByIndexes, Exceptions) {
	matrix<int> mtx(3, 4);

	EXPECT_THROW(mtx(3, 3), std::out_of_range);
	EXPECT_THROW(mtx(2, 4), std::out_of_range);

	EXPECT_THROW(mtx(3, 4), std::out_of_range);
	EXPECT_THROW(mtx(-1, 0), std::out_of_range);
}