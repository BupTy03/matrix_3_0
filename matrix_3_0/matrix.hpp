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

namespace impl {
	namespace internal_ns {

		template<typename Iter>
		constexpr bool is_input_iter_v =
			std::is_same_v<typename std::iterator_traits<Iter>::iterator_category, std::input_iterator_tag>;

		// matrix algorithms for POD types
		template<typename T, typename AllocRows, typename AllocCols>
		T** make_matrix(const std::size_t rows, const std::size_t cols, const T& val, std::true_type) {
			AllocRows alloc_rows;
			AllocCols alloc_cols;

			T** mtx = alloc_rows.allocate(rows);
			for (std::size_t row = 0; row < rows; ++row) {
				T* pRow = alloc_cols.allocate(cols);
				std::fill_n(pRow, cols, val);
				mtx[row] = pRow;
			}

			return mtx;
		}

		template<typename T, typename Iter, typename AllocRows, typename AllocCols,
			typename = std::enable_if_t<is_input_iter_v<Iter>>>
		T** make_matrix(const std::size_t rows, const std::size_t cols, Iter first, Iter last, std::true_type) {
			assert(rows * cols == std::distance(first, last));

			AllocRows alloc_rows;
			AllocCols alloc_cols;

			T** mtx = alloc_rows.allocate(rows);
			for (std::size_t row = 0; row < rows; ++row) {
				mtx[row] = alloc_cols.allocate(cols);
				for (std::size_t col = 0; col < cols; ++col) {
					mtx[row][col] = *first;
					++first;
				}
			}

			return mtx;
		}

		template<typename T, typename AllocRows, typename AllocCols>
		T** make_matrix(T** other, const std::size_t rows, const std::size_t cols, std::true_type) {
			assert(other != nullptr);

			AllocRows alloc_rows;
			AllocCols alloc_cols;

			T** mtx = alloc_rows.allocate(rows);
			for (std::size_t row = 0; row < rows; ++row) {
				T* pRow = alloc_cols.allocate(cols);
				std::copy_n(other[row], cols, pRow);
				mtx[row] = pRow;
			}

			return mtx;
		}

		template<typename T>
		void copy_matrix(T** lhs, const T** rhs, const std::size_t rows, const std::size_t cols, std::true_type) {
			assert(lhs != nullptr);
			assert(rhs != nullptr);

			for (std::size_t row = 0; row < rows; ++row) {
				std::copy_n(rhs[row], cols, lhs[row]);
			}
		}

		template<typename T, typename AllocRows, typename AllocCols>
		void deallocate_matrix(T** mtx, const std::size_t rows, const std::size_t cols, const std::size_t, const std::size_t, std::true_type) {
			assert(mtx != nullptr);

			AllocRows alloc_rows;
			AllocCols alloc_cols;

			for (std::size_t row = 0; row < rows; ++row) {
				alloc_cols.deallocate(mtx[row], cols);
			}

			alloc_rows.deallocate(mtx, rows);
		}

		template<typename T, typename AllocRows, typename AllocCols>
		void assign_matrix(T*** lhs, const std::size_t lhs_rows, const std::size_t lhs_cols, const std::size_t, const std::size_t,
			const T** rhs, const std::size_t rhs_rows, const std::size_t rhs_cols, std::true_type) {

			assert(lhs != nullptr);
			AllocRows alloc_rows;
			AllocCols alloc_cols;

			T** pLhs = (*lhs);

			// if rhs matrix is empty
			if (rhs == nullptr) {
				assert(rhs_rows == 0);
				assert(rhs_cols == 0);

				if (pLhs == nullptr) { // if lhs matrix is empty
					assert(lhs_rows == 0);
					assert(lhs_cols == 0);
				}
				else { // if lhs matrix is not empty - deallocate lhs
					assert(lhs_rows > 0);
					assert(lhs_cols > 0);
					deallocate_matrix<T, AllocRows, AllocCols>(pLhs, lhs_rows, lhs_cols, std::true_type);
				}
				return;
			}

			// if rhs matrix is not empty
			assert(rhs_rows > 0);
			assert(rhs_cols > 0);

			// if lhs matrix is empty
			if (pLhs == nullptr) {
				assert(lhs_rows == 0);
				assert(lhs_cols == 0);
				*lhs = make_matrix<T, AllocRows, AllocCols>(rhs, rhs_rows, rhs_cols, std::true_type);
				return;
			}

			// if lhs matrix is not empty
			assert(lhs_rows > 0);
			assert(lhs_cols > 0);

			// if count rows in lhs and rhs are equal
			if (lhs_rows == rhs_rows) {
				if (lhs_cols == rhs_cols) { // if count cols in lhs and rhs are equal
					copy_matrix(pLhs, rhs, lhs_rows, lhs_cols, std::true_type);
				}
				else { // if count cols in lhs and rhs are not equal
					for (std::size_t row = 0; row < lhs_rows; ++row) {
						alloc_cols.deallocate(pLhs[row], lhs_cols);
						pLhs[row] = alloc_cols.allocate(rhs_cols);
						std::copy_n(rhs[row], rhs_cols, pLhs[row]);
					}
				}
				return;
			}

			// if count rows in lhs and rhs are not equal - allocating new matrix
			deallocate_matrix<T, AllocRows, AllocCols>(pLhs, lhs_rows, lhs_cols, std::true_type);
			*lhs = make_matrix<T, AllocRows, AllocCols>(rhs, rhs_rows, rhs_cols, std::true_type);
		}


		// matrix algorithms for non-POD types
		template<typename T, std::enable_if_t<std::is_class_v<T>>>
		void destroy_by_ptr(T* ptr) noexcept { assert(ptr != nullptr); ptr->~T(); }

		template<typename Iter, std::enable_if_t<std::is_class_v<typename std::iterator_traits<Iter>::value_type>>>
		void destroy_n_elems(Iter first, std::size_t n) {
			for (std::size_t index = 0; index < n; ++index, ++first)
				destroy_by_ptr(std::addressof(*first));
		}

		template<typename T, typename AllocRows, typename AllocCols>
		void destroy_and_deallocate_matrix(
			T** pMtx,
			std::size_t countRows,
			std::size_t countCols,
			std::size_t spaceRows, 
			std::size_t spaceCols) {

			if (pMtx == nullptr) {
				assert(countRows == 0);
				assert(countCols == 0);
				assert(spaceRows == 0);
				assert(spaceCols == 0);
				return;
			}

			AllocCols alloc_cols;
			for (std::size_t row = 0; row < countRows; ++row) {
				destroy_n_elems(pMtx[row], countCols);
				alloc_cols.deallocate(pMtx[row], spaceCols);
			}

			for (std::size_t row = countRows; row < spaceRows; ++row) {
				alloc_cols.deallocate(pMtx[row], spaceCols);
			}

			AllocRows alloc_rows;
			alloc_rows.deallocate(pMtx, spaceRows);
		}

		template<typename T, typename AllocRows, typename AllocCols>
		void destroy_and_deallocate_until_curr(
			T** pMtx,
			std::size_t countRows, 
			std::size_t countCols, 
			std::size_t currRow, 
			std::size_t currCol) {

			assert(pMtx == nullptr);

			T** pFirstRow = pMtx;
			const T** pCurrRow = pMtx + currRow;
			const T** pLastRow = pMtx + countRows;

			AllocCols alloc_cols;

			// destructing all before current row
			for (; pFirstRow != pCurrRow; ++pFirstRow) {
				destroy_n_elems(pFirstRow, countCols);
				alloc_cols.deallocate(pFirstRow, countCols);
			}

			// destructing current row
			destroy_n_elems(pFirstRow, currCol);
			alloc_cols.deallocate(pFirstRow, countCols);

			// advance to the next row
			++pFirstRow;

			// deallocating all after current row
			for (; pFirstRow != pLastRow; ++pFirstRow) {
				alloc_cols.deallocate(pFirstRow, countCols);
			}

			AllocRows alloc_rows;
			alloc_rows.deallocate(pMtx, countRows);
		}

		template<typename T, typename AllocRows, typename AllocCols>
		T** make_matrix(const std::size_t rows, const std::size_t cols, const T& val, std::false_type) {
			AllocRows alloc_rows;
			AllocCols alloc_cols;

			T** mtx = alloc_rows.allocate(rows);
			std::size_t row = 0;
			std::size_t col = 0;

			try {
				for (row = 0; row < rows; ++row) {
					mtx[row] = alloc_cols.allocate(cols);
					for (col = 0; col < cols; ++col) {
						new (&mtx[row][col]) T(val);
					}
				}
			}
			catch (...) {
				destroy_and_deallocate_until_curr<T, AllocRows, AllocCols>(mtx, row, col);
				throw;
			}

			return mtx;
		}

		template<typename T, typename Iter, typename AllocRows, typename AllocCols,
			typename = std::enable_if_t<is_input_iter_v<Iter>>>
		T** make_matrix(const std::size_t rows, const std::size_t cols, Iter first, Iter last, std::false_type) {
			assert(rows * cols == std::distance(first, last));

			AllocRows alloc_rows;
			AllocCols alloc_cols;

			T** mtx = alloc_rows.allocate(rows);
			std::size_t row = 0;
			std::size_t col = 0;

			try {
				for (row = 0; row < rows; ++row) {
					mtx[row] = alloc_cols.allocate(cols);
					for (col = 0; col < cols; ++col) {
						new (&mtx[row][col]) T(*first);
						++first;
					}
				}
			}
			catch (...) {
				destroy_and_deallocate_until_curr<T, AllocRows, AllocCols>(mtx, row, col);
				throw;
			}

			return mtx;
		}

		template<typename T, typename AllocRows, typename AllocCols>
		T** make_matrix(T** other, const std::size_t rows, const std::size_t cols, std::false_type) {
			assert(other != nullptr);

			AllocRows alloc_rows;
			AllocCols alloc_cols;

			T** mtx = alloc_rows.allocate(rows);
			std::size_t row = 0;
			std::size_t col = 0;

			try {
				for (row = 0; row < rows; ++row) {
					mtx[row] = alloc_cols.allocate(cols);
					for (col = 0; col < cols; ++col) {
						new (&mtx[row][col]) T(other[row][col]);
					}
				}
			}
			catch (...) {
				destroy_and_deallocate_until_curr<T, AllocRows, AllocCols>(mtx, row, col);
				throw;
			}

			return mtx;
		}
		
		template<typename T>
		void copy_matrix(T** lhs, const T** rhs, const std::size_t rows, const std::size_t cols, std::false_type) {
			assert(lhs != nullptr);
			assert(rhs != nullptr);

			for (std::size_t row = 0; row < rows; ++row)
				std::copy_n(rhs[row], cols, lhs[row]);
		}
		
		template<typename T, typename AllocRows, typename AllocCols>
		void deallocate_matrix(T** mtx, const std::size_t rows, const std::size_t cols, 
			const std::size_t space_rows, const std::size_t space_cols, std::false_type) {
			
			assert(mtx != nullptr);

			AllocRows alloc_rows;
			AllocCols alloc_cols;

			for (std::size_t row = 0; row < rows; ++row) {
				destroy_n_elems(mtx[row], cols);
				alloc_cols.deallocate(mtx[row], space_cols);
			}

			alloc_rows.deallocate(mtx, space_rows);
		}

		template<typename T, typename AllocRows, typename AllocCols>
		void assign_matrix(T*** lhs, const std::size_t lhs_rows, const std::size_t lhs_cols, 
			const std::size_t lhs_space_rows, const std::size_t lhs_space_cols,
			const T** rhs, const std::size_t rhs_rows, const std::size_t rhs_cols, std::false_type) {

			assert(lhs != nullptr);
			AllocRows alloc_rows;
			AllocCols alloc_cols;

			T** pLhs = *lhs;

			// if rhs matrix is empty
			if (rhs == nullptr) {
				assert(rhs_rows == 0);
				assert(rhs_cols == 0);

				if (pLhs == nullptr) { // if lhs matrix is empty
					assert(lhs_rows == 0);
					assert(lhs_cols == 0);
				}
				else { // if lhs matrix is not empty - deallocate lhs
					assert(lhs_rows > 0);
					assert(lhs_cols > 0);
					deallocate_matrix<T, AllocRows, AllocCols>(pLhs, lhs_rows, lhs_cols, lhs_space_rows, lhs_space_cols, std::false_type);
				}
				return;
			}

			// if rhs matrix is not empty
			assert(rhs_rows > 0);
			assert(rhs_cols > 0);

			// if lhs matrix is empty
			if (pLhs == nullptr) {
				assert(lhs_rows == 0);
				assert(lhs_cols == 0);
				*lhs = make_matrix<T, AllocRows, AllocCols>(rhs, rhs_rows, rhs_cols, std::false_type);
				return;
			}

			// if lhs matrix is not empty
			assert(lhs_rows > 0);
			assert(lhs_cols > 0);

			// if count rows in lhs and rhs are equal
			if (lhs_rows == rhs_rows) {
				if (lhs_cols == rhs_cols) { // if count cols in lhs and rhs are equal
					copy_matrix(pLhs, rhs, lhs_rows, lhs_cols, std::false_type);
				}
				else { 
					// if count cols in lhs and rhs are not equal - 
					// destructing and deallocating old rows and allocating and constructing new rows
					std::size_t row = 0;
					std::size_t col = 0;
					try {
						for (row = 0; row < lhs_rows; ++row) {
							// destructing and deallocating old row
							for (col = 0; col < lhs_cols; ++col) {
								destroy_by_ptr(&pLhs[row][col]);
							}
							alloc_cols.deallocate(pLhs[row], lhs_space_cols);

							// allocating and constructing new row
							pLhs[row] = alloc_cols.allocate(rhs_cols);
							for (col = 0; col < lhs_cols; ++col) {
								new (&pLhs[row][col]) T(rhs[row][col]);
							}
						}
					}
					catch (...) {
						destroy_and_deallocate_until_curr<T, AllocRows, AllocCols>(pLhs, row, col);
						throw;
					}
				}
				return;
			}

			// if count rows in lhs and rhs are not equal - allocating new matrix
			deallocate_matrix<T, AllocRows, AllocCols>(pLhs, lhs_rows, lhs_cols, lhs_space_rows, lhs_space_cols, std::false_type);
			*lhs = make_matrix<T, AllocRows, AllocCols>(rhs, rhs_rows, rhs_cols, std::false_type);
		}
	}

	template<typename T, typename AllocRows, typename AllocCols>
	T** make_matrix(const std::size_t rows, const std::size_t cols, const T& val) {
		return internal_ns::make_matrix<T, AllocRows, AllocCols>(rows, cols, val, std::is_pod_v<T>);
	}

	template<typename T, typename Iter>
	T** make_matrix(const std::size_t rows, const std::size_t cols, Iter first, Iter last) {
		return internal_ns::make_matrix(rows, cols, first, last, std::is_pod_v<T>);
	}

	template<typename T, typename AllocRows, typename AllocCols>
	T** make_matrix(T** other, const std::size_t rows, const std::size_t cols) {
		return internal_ns::make_matrix<T, AllocRows, AllocCols>(other, rows, cols, std::is_pod_v<T>);
	}

	template<typename T>
	void copy_matrix(T** lhs, const T** rhs, const std::size_t rows, const std::size_t cols) {
		internal_ns::copy_matrix(lhs, rhs, rows, cols, std::is_pod_v<T>);
	}

	template<typename T, typename AllocRows, typename AllocCols>
	void deallocate_matrix(T** mtx, const std::size_t rows, const std::size_t cols,
		const std::size_t space_rows, const std::size_t space_cols) {
		internal_ns::deallocate_matrix<T, AllocRows, AllocCols>(mtx, rows, cols, space_rows, space_cols, std::is_pod_v<T>);
	}

	template<typename T, typename AllocRows, typename AllocCols>
	void assign_matrix(T*** lhs, const std::size_t lhs_rows, const std::size_t lhs_cols,
		const std::size_t lhs_space_rows, const std::size_t lhs_space_cols,
		const T** rhs, const std::size_t rhs_rows, const std::size_t rhs_cols) {
		internal_ns::assign_matrix<T, AllocRows, AllocCols>(lhs, lhs_rows, lhs_cols, lhs_space_rows, lhs_space_cols,
			rhs, rhs_rows, rhs_cols, std::is_pod_v<T>);
	}

}

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
	explicit matrix(size_type cols, std::initializer_list<T> initList) : matrix(cols, initList.begin(), initList.end()) {}

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

	matrix(const matrix& other) 
	{
		if (other.empty()) 
			return;

		assign_elems(other.sz_.rows, other.sz_.cols, other.elems_);
	}
	matrix& operator=(const matrix& other)
	{ 
		if (this == &other)
			return *this;

		matrix tmp(other);
		this->swap(tmp);
		return *this;
	}

	matrix(matrix&& other) noexcept { this->swap(other); }
	matrix& operator=(matrix&& other) noexcept 
	{
		if (this == &other)
			return *this;

		this->swap(other);
		return *this;
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
