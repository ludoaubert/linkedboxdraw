#include <vector>
#include <stdio.h>
using namespace std;


struct TensorDimension
{
	int i;
	int n;
};


struct Tensor
{
	Tensor& operator++(int)
	{
		TensorDimension& dim = dimensions[k];
		if (dim.i + 1 < dim.n)
		{
			dim.i++;
			return *this;
		}
		
		k++;
		
		for (int l=0; l<k; l++)
		{
			dimensions[l].i=0;
		}
		dimensions[k].i++;
		k=0;
		return *this;
	}
	vector<TensorDimension> dimensions;
	int k=0;
};


int number_of_tensor_cells(const vector<TensorDimension>& dimensions)
{
	int nb=1;
	
	for (const auto& [i,n] : dimensions)
		nb *= n;
	
	return nb;
}

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
}