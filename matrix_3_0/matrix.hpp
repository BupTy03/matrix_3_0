#pragma once
#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <iostream>
#include <memory>
#include <scoped_allocator>
#include <type_traits>
#include <cassert>


struct matrix_size_type {
	constexpr matrix_size_type() = default;
	explicit constexpr matrix_size_type(std::size_t rows, std::size_t cols) : rows{ rows }, cols{ cols } {}

	constexpr bool operator<(matrix_size_type other) const noexcept { return rows * cols < other.rows * other.cols; }
	constexpr bool operator>=(matrix_size_type other) const noexcept { return !(*this < other); }

	constexpr bool operator>(matrix_size_type other) const noexcept { return (other < *this); }
	constexpr bool operator<=(matrix_size_type other) const noexcept { return !(*this > other); }

	constexpr bool operator==(matrix_size_type other) const noexcept { return !(*this < other || *this > other); }
	constexpr bool operator!=(matrix_size_type other) const noexcept { return !(*this == other); }

	std::size_t rows = 0;
	std::size_t cols = 0;
};

template<class T, class Allocator = std::allocator<T>>
class matrix {
public:
	using value_type = T;
	using allocator_type = Allocator;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using const_reference = const T&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	static_assert(std::is_same_v<T, typename Allocator::value_type>, "allocator must allocate type T");

	explicit matrix() = default;
	explicit matrix(size_type rows, size_type cols, const T& value) { construct_with_value(rows, cols, value); }
	explicit matrix(size_type rows, size_type cols) { construct_with_value(rows, cols, T()); }
	explicit matrix(size_type cols, std::initializer_list<T> initList) 
		: matrix(cols, initList.begin(), initList.end()) {}

	template<class It, typename = std::enable_if_t<std::is_same_v<
		typename std::iterator_traits<It>::iterator_category, 
		typename std::iterator_traits<It>::iterator_category>
	>>
	explicit matrix(size_type cols, It first, It last)
	{
		if (cols == 0) {
			if (first == last) {
				return;
			}
			throw std::invalid_argument{ "cols count must be greater than zero" };
		}

		const auto count_elems = std::distance(first, last);
		assert(count_elems % cols == 0);

		const size_type rows = count_elems / cols;
		assert(count_elems == (rows * cols));

		construct_from_iterators(rows, cols, first, last);
	}

	~matrix() { clear(); }

	explicit matrix(const matrix& other) { copy_of(other.elems_, other.sz_.rows, other.sz_.cols); }
	matrix& operator=(const matrix& other){ matrix tmp(other); this->swap(tmp); }

	explicit matrix(matrix&& other) noexcept { this->swap(other); }
	matrix& operator=(matrix&& other) noexcept 
	{
		if (this == &other)
			return;

		this->swap(other);
	}

	T** data() { return elems_; }

	T* operator[](size_type index) noexcept { return elems_[index]; }
	const T* operator[](size_type index) const noexcept { return elems_[index]; }

	T& operator()(size_type row, size_type col) { check_index(row, col); return elems_[row][col]; }
	const T& operator()(size_type row, size_type col) const { check_index(row, col); return elems_[row][col]; }

	bool empty() const noexcept { return sz_ == matrix_size_type{ 0,0 }; }
	matrix_size_type size() const noexcept { return sz_; }
	matrix_size_type capacity() const noexcept { return space_; }

	void swap(matrix& other) noexcept
	{
		std::swap(sz_, other.sz_);
		std::swap(space_, other.space_);
		std::swap(elems_, other.elems_);
		std::swap(alloc_, other.alloc_);
	}

	void clear() { destroy_and_deallocate_elems(space_.rows, space_.cols, sz_.rows - 1, sz_.cols); }

private:
	void check_index(size_type row, size_type col) const
	{ 
		if (row >= sz_.rows)
			throw std::out_of_range{ "row is out of this matrix" };

		if (col >= sz_.cols) 
			throw std::out_of_range{ "col is out of this matrix" };
	}

private:
	void construct_with_value(size_type rows, size_type cols, const T& value)
	{
		if (rows != cols) {
			if (rows == 0) {
				throw std::invalid_argument{ "rows count must be greater than zero" };
			}

			if (cols == 0) {
				throw std::invalid_argument{ "cols count must be greater than zero" };
			}
		}

		elems_ = alloc_.allocate(rows);
		sz_ = matrix_size_type{ rows, cols };
		space_ = sz_;

		size_type currRow = 0;
		size_type currCol = 0;
		try {
			for (currRow = 0; currRow < rows; ++currRow) {
				elems_[currRow] = (alloc_.inner_allocator()).allocate(cols);
				for (currCol = 0; currCol < cols; ++currCol) {
					::new (&(elems_[currRow][currCol])) T(value);
				}
			}
		}
		catch (...) {
			destroy_and_deallocate_elems(rows, cols, currRow, currCol);
			throw;
		}
	}
	void assign_elems(size_type rows, size_type cols, T** other_elems)
	{
		assert(other_elems != nullptr);

		sz_ = matrix_size_type{ rows, cols };
		space_ = sz_;
		elems_ = alloc_.allocate(rows);

		size_type currRow = 0;
		size_type currCol = 0;
		try {
			for (currRow = 0; currRow < rows; ++currRow) {
				elems_[currRow] = (alloc_.inner_allocator()).allocate(sz_.cols);
				for (currCol = 0; currCol < cols; ++currCol) {
					::new (&(elems_[currRow][currCol])) T(other_elems[currRow][currCol]);
				}
			}
		}
		catch (...) {
			destroy_and_deallocate_elems(rows, cols, currRow, currCol);
			throw;
		}
	}
	template<class It>
	void construct_from_iterators(size_type rows, size_type cols, It first, It last)
	{
		assert(rows * cols == std::distance(first, last));
		sz_ = matrix_size_type{ rows, cols };
		space_ = sz_;
		elems_ = alloc_.allocate(rows);

		size_type currRow = 0;
		size_type currCol = 0;
		try {
			for (currRow = 0; currRow < rows; ++currRow) {
				elems_[currRow] = (alloc_.inner_allocator()).allocate(cols);
				for (currCol = 0; currCol < cols; ++currCol, ++first) {
					::new (&(elems_[currRow][currCol])) T(*first);
				}
			}
		}
		catch (...) {
			destroy_and_deallocate_elems(rows, cols, currRow, currCol);
			throw;
		}
	}
	void destroy_and_deallocate_elems(size_type countRows, size_type countCols, size_type currRow, size_type currCol)
	{
		if (elems_ == nullptr) {
			assert(sz_ == matrix_size_type{});
			assert(space_ == matrix_size_type{});
			return;
		}

		// destructing all before current row
		for (size_type row = 0; row < currRow; ++row) {
			for (size_type col = 0; col < countCols; ++col) {
				elems_[row][col].~T();
			}
			(alloc_.inner_allocator()).deallocate(elems_[row], countCols);
		}

		// destructing current row [0...currCol)
		for (size_type col = 0; col < currCol; ++col) {
			elems_[currRow][col].~T();
		}
		(alloc_.inner_allocator()).deallocate(elems_[currRow], countCols);

		// deallocating all after current row
		for (size_type row = currRow + 1; row < countRows; ++row) {
			(alloc_.inner_allocator()).deallocate(elems_[row], countCols);
		}
		alloc_.deallocate(elems_, countRows);

		elems_ = nullptr;
		sz_ = matrix_size_type{};
		space_ = sz_;
	}

private:
	using RowAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<T*>;

	matrix_size_type sz_{ 0, 0 };
	matrix_size_type space_{ 0, 0 };
	T** elems_ = nullptr;
	std::scoped_allocator_adaptor<RowAllocator, Allocator> alloc_;
};


template<class T, class A>
std::ostream& operator<<(std::ostream& os, const matrix<T, A>& mtx)
{
	os << '{';
	const auto mtx_sz = mtx.size();
	for (std::size_t row = 0; row < mtx_sz.rows; ++row) {
		os << '{';
		for (std::size_t col = 0; col < mtx_sz.cols; ++col) {
			os << mtx[row][col] << ((col != mtx_sz.cols - 1) ? ", " : "");
		}
		os << '}' << ((row != mtx_sz.rows - 1) ? ", " : "");
	}
	os << "}";
	return os;
}


#endif // !MATRIX_HPP
