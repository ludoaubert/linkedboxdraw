#include <cstdint>
#include <iostream>
#include <chrono>
#include <cmath>
#include <numeric>
#include <utility>
#include <algorithm>
#include <execution>
using namespace std;

double f(double x) noexcept
{
        const int N = 1000;
        for (int i = 0; i < N; ++i) {
            x = log2(x);
            x = cos(x);
            x = x * x + 1;
        }
        return x;
}

double sum(const vector<double>& vec)
{
        double sum = 0;
        for (auto x : vec)
            sum += x;
        return sum;
}

int main()
{
        cout << "Hey! Your machine has " << thread::hardware_concurrency() << " cores!\n";

        // Make an input vector.
        const int N = 1000000;
        vector<double> vecInput(N);
        for (int i = 0; i < N; ++i)
            vecInput[i] = i + 1;

        {   // Case #1: Plain transform, no parallelism.
            auto startTime = chrono::system_clock::now();
            vector<double> vecOutput(N);
            transform(vecInput.cbegin(), vecInput.cend(), vecOutput.begin(), f);
            auto endTime = chrono::system_clock::now();
            chrono::duration<double> diff = endTime - startTime;
            cout << "1. sum = " << sum(vecOutput) << ", time = " << diff.count() << "\n";
        }
        {   // Case #2: Transform with parallel unsequenced.
            vector<double> vecOutput(N);
            auto startTime = chrono::system_clock::now();
            transform(execution::par_unseq,
                        vecInput.cbegin(), vecInput.cend(), vecOutput.begin(), f);
            auto endTime = chrono::system_clock::now();
            chrono::duration<double> diff = endTime - startTime;
            cout << "2. sum = " << sum(vecOutput) << ", time = " << diff.count() << "\n";
        }
}

    /* Output:
        Hey! Your machine has 4 cores!
        1. sum = 1.60346e+06, time = 43.7997
        2. sum = 1.60346e+06, time = 43.8235
    */
