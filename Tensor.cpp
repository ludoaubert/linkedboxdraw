#include <vector>
#include <stdio.h>
#include <cstdint>
using namespace std;


struct TensorDimension
{
	int i;
	int n;
};

int number_of_tensor_cells(const vector<TensorDimension>& dimensions)
{
	int nb=1;
	
	for (const auto& [i,n] : dimensions)
		nb *= n;
	
	return nb;
}

uint64_t digits2number(const vector<TensorDimension>& dimensions)
{
	uint64_t number=0;
	int base=1;
	for (const auto& [i,n] : dimensions)
	{
		number += i*base;
		base *= n;
	}
	return number;
}

void number2digits(uint64_t number, vector<TensorDimension>& dimensions)
{
	for (auto& [i,n] : dimensions)
	{
		i = number % n;
		number -= i ;
		number /= n;
	}
}


struct Tensor
{
	Tensor& operator++(int)
	{
		uint64_t number = digits2number(dimensions);
		number++;
		number2digits(number, dimensions);
		return *this;
	}
	vector<TensorDimension> dimensions;
};



int main()
{
	Tensor tensor;
	tensor.dimensions = {{0,2},{0,3}};
	
	int nb = number_of_tensor_cells(tensor.dimensions);
	printf("%d\n", nb);
	
	for (int i=0; i<nb; i++)
	{
		printf("%d %d\n", tensor.dimensions[0].i, tensor.dimensions[1].i);
		tensor++;
	}
	
	tensor.dimensions = {{9,10},{9,10},{2,10}};
	for (int i=0; i<3; i++)
	{
		printf("%d %d %d\n", tensor.dimensions[0].i, tensor.dimensions[1].i, tensor.dimensions[2].i);
		tensor++;
	}
	
	tensor.dimensions = {{9,10},{2,10},{2,10}};
	for (int i=0; i<3; i++)
	{
		printf("%d %d %d\n", tensor.dimensions[0].i, tensor.dimensions[1].i, tensor.dimensions[2].i);
		tensor++;
	}
}