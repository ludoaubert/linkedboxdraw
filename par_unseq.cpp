#include <cstdint>
#include <iostream>
#include <chrono>
#include <cmath>
#include <numeric>
#include <utility>
#include <algorithm>
#include <execution>

static double f(double x) noexcept
{
        const int N = 1000;
        for (int i = 0; i < N; ++i) {
            x = std::log2(x);
            x = std::cos(x);
            x = x * x + 1;
        }
        return x;
}

static double sum(const std::vector<double>& vec)
{
        double sum = 0;
        for (auto x : vec)
            sum += x;
        return sum;
}

int main()
{
        std::cout << "Hey! Your machine has " << std::thread::hardware_concurrency() << " cores!\n";

        // Make an input vector.
        const int N = 1000000;
        std::vector<double> vecInput(N);
        for (int i = 0; i < N; ++i)
            vecInput[i] = i + 1;

        {   // Case #1: Plain transform, no parallelism.
            auto startTime = std::chrono::system_clock::now();
            std::vector<double> vecOutput(N);
            std::transform(vecInput.cbegin(), vecInput.cend(), vecOutput.begin(), f);
            auto endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> diff = endTime - startTime;
            std::cout << "1. sum = " << sum(vecOutput) << ", time = " << diff.count() << "\n";
        }
        {   // Case #2: Transform with parallel unsequenced.
            std::vector<double> vecOutput(N);
            auto startTime = std::chrono::system_clock::now();
            std::transform(std::execution::par_unseq,
                        vecInput.cbegin(), vecInput.cend(), vecOutput.begin(), f);
            auto endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> diff = endTime - startTime;
            std::cout << "2. sum = " << sum(vecOutput) << ", time = " << diff.count() << "\n";
        }
}

    /* Output:
        Hey! Your machine has 4 cores!
        1. sum = 1.60346e+06, time = 43.7997
        2. sum = 1.60346e+06, time = 43.8235
    */
