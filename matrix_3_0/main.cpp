#include "matrix.hpp"

#include <iostream>

int main()
{
	matrix<int> mtx(3, { 1, 2, 3, 4, 5, 6, 7, 8, 9 });
	std::cout << mtx << std::endl;

	return 0;
}