// RUN WITH: clang++ program.cpp -o run -std=c++11

#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <pthread.h>

using namespace std::chrono;
using namespace std;

void randomVector(int vector[], int start, int end, unsigned int seed)
{
    for (int i = start; i < end; i++)
    {
        // Populates the vector passed with random numbers from 0 to 99.
        vector[i] = rand_r(&seed) % 100;
    }
}

struct Data
{
    int *v1, *v2, *v3;
    unsigned int seed;
};

struct thread_data {
   int start;
   int stop;
   Data *vecData;
};


void *addVector(void *args)
{
    // Parse args
    struct thread_data *tData;
    tData = (struct thread_data*) args;

    // Fill vectors 1 and 2.
    randomVector(tData->vecData->v1, tData->start, tData->stop, tData->vecData->seed);
    randomVector(tData->vecData->v2, tData->start, tData->stop, tData->vecData->seed);

    // Calculate sum
    for (int i = tData->start; i < tData->stop; i++)
    {
        tData->vecData->v3[i] = tData->vecData->v1[i] + tData->vecData->v2[i];
    }

    pthread_exit(NULL);
}


int main(){

    unsigned long size = 150000000; // 150,000,000
    const int THREADS = 8;
    int partition = size / THREADS;
    int total = 0;
    int totalCheck = 0;

    srand(time(0));

    int *v1, *v2, *v3;

    // Starts the high res clock which counts in micro seconds (millionths of a second).
    auto start = high_resolution_clock::now();

    // Declares the values held by Vn as an unsigned long and the number to be held as "size" = 100,000,000.
	// Then allocates memory on the heap for each of the soon to be created vectors.
    v1 = (int *) malloc(size * sizeof(int *));
    v2 = (int *) malloc(size * sizeof(int *));
    v3 = (int *) malloc(size * sizeof(int *));

    // Adding vectors to the Data struct for ease of passing.
    Data data;
    data.v1 = v1;
    data.v2 = v2;
    data.v3 = v3;

    // Create a thread for number of THREADS.
    pthread_t threads[THREADS];
    struct thread_data threadData[THREADS];

    for (int i = 0; i < THREADS; i++)
    {
        data.seed = i+1;

        // Partition indexes
        threadData[i].start = partition * i;
        threadData[i].stop = threadData[i].start + partition;
        threadData[i].vecData = &data;

        if (i == THREADS - 1)
        {
            threadData[i].stop = size;
        }
        //cout << "Partition: " << data.start << " to " << data.stop << endl;
        // Create and run thread
        pthread_create(&threads[i], NULL, addVector, &threadData[i]);
    }

    // Joining threads
    for (int i = 0; i < THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    auto stop = high_resolution_clock::now();

    // Calculating the duration in micro seconds from start to stop.
    auto duration = duration_cast<milliseconds>(stop - start);

    for (int i = 0; i < size; i++)
    {
        total += data.v3[i];
    }
    // Ensure that total is calculating correctly.
    for (int i = 0; i < size; i++)
    {
        totalCheck += data.v1[i];
        totalCheck += data.v2[i];
    }

    // Print Summary
    cout << "pThread: for comparision (" << THREADS << " Threads, "
        <<  size << " Elements). \nTotal Value: "
    << total << ", expecting: " << totalCheck <<endl;
    cout << " Time taken by function: " << duration.count() << " milliseconds" << endl;
    cout << " Time taken by function: " << duration.count()/1000.0 << " seconds" << endl;

    free(v1);
    free(v2);
    free(v3);

    return 0;
}
