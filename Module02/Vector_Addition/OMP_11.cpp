#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>

// include Open MP library
#include <omp.h>

using namespace std::chrono;
using namespace std;

void randomVector(int vector[], int size, unsigned int seed)
{
    // Parallelise for loop to set elements to random numbers
    #pragma omp for
    for (int i = 0; i < size; i++)
    {
        // Set element i in vector with a random number from 0-99
        vector[i] = rand_r(&seed) % 100;
    }
}

int main(){

    unsigned long size = 150000000; // 150,000,000
    const int THREADS = 8;

    srand(time(0));

    int *v1, *v2, *v3;

    // Setup number of threads to use
    omp_set_num_threads(THREADS);

    // Start time of vector randomisation and addition
    auto start = high_resolution_clock::now();

    // Allocate vector size
    v1 = (int *) malloc(size * sizeof(int *));
    v2 = (int *) malloc(size * sizeof(int *));
    v3 = (int *) malloc(size * sizeof(int *));

   // Parallelise filling random vectors and performing addition
   cout << "Running Threads";
   #pragma omp parallel
   {

        // Seed for the random number generator
        unsigned int seed = omp_get_thread_num();
        // Printing of thread numbers as created.
        printf(", %d", seed);

        // Fill random vectors
        randomVector(v1, size, seed);
        randomVector(v2, size, seed);

        // Wait for random vector to fill
        #pragma omp barrier

        // Parallelise for loop to add v1 and v2 components
        #pragma omp for
        for (int i = 0; i < size; i++)
        {
            v3[i] = v1[i] + v2[i];
        }
   }

    auto stop = high_resolution_clock::now();

    // End time of vector randomisation and addition
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

    // Print Summary
    cout << ".\nOMP: Parallel & For Directives: 1.1 (" << THREADS << " Threads, "
        <<  size << " Elements). \nTotal Value: "
    << total << ", expecting: " << totalCheck <<endl;
    cout << " Time taken by function: " << duration.count() << " microseconds" << endl;
    cout << " Time taken by function: " << duration.count()/1000000.0 << " seconds" << endl;

    // Free Memory
    free(v1);
    free(v2);
    free(v3);

   return 0;
}
