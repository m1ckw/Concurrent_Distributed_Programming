#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>


using namespace std::chrono;
using namespace std;

// Populates the vector passed with random values
void randomVector(int vector[], int size)
{
    for (int i = 0; i < size; i++)
    {
        // Values of 0-99.
        vector[i] = rand() % 100;
    }
}


int main(){

    unsigned long size = 150000000; // 150,000,000

    srand(time(0));

    int *v1, *v2, *v3;

    // Start timer
    auto start = high_resolution_clock::now();

    // Allocation of memory on the heap for each vector.
    v1 = (int *) malloc(size * sizeof(int *));
    v2 = (int *) malloc(size * sizeof(int *));
    v3 = (int *) malloc(size * sizeof(int *));

    // Populates V1 and V2 with interger values 0-99.
    randomVector(v1, size);
    randomVector(v2, size);

    // Sums corrosponding elements in V1 and V2 adding the result to V3.
    for (int i = 0; i < size; i++)
    {
        v3[i] = v1[i] + v2[i];
    }

    auto stop = high_resolution_clock::now();

    // Calculates the computation time - start to stop.
    auto duration = duration_cast<microseconds>(stop - start);

    int total = 0;
    for (int i = 0; i < size; i++)
    {
        total += v3[i];
    }
    // Ensure that total is calculating correctly.
    int totalCheck = 0;
    for (int i = 0; i < size; i++)
    {
        totalCheck += v1[i];
        totalCheck += v2[i];
    }

    // Print summary of results
    cout << "Sequential for comparison: 2.1 (1 Threads, " << size << " Elements). \nTotal Value: "
    << total << ", expecting: " << totalCheck <<endl;
    cout << " Time taken by function: "
         << duration.count() << " microseconds" << endl;
    cout << " Time taken by function: "
         << duration.count()/1000000 << " seconds" << endl;

    // Free Memory
    free(v1);
    free(v2);
    free(v3);

    return 0;
}
