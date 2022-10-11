// On Mac RUN WITH: -
// On Windows RUN WITH: g++ -fopenmp \OMP_Qsort.cpp -o ompQS.exe

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <chrono>
#include <omp.h>

using namespace std::chrono;
using namespace std;

#define SIZE 2000000 // Defines the size of the vector to be sorted.

// Populates the vector with random values
void rand_fill(int* vec, int size, unsigned int seed)
{
    #pragma omp for schedule(auto) // Auto Schedule to fill vector.
    for (int i=0; i<size; i++)
    {
        vec[i] = rand_r(&seed) %1000; // 0 to 999.
    }
}

// Prints vector for testing (small vectors).
void print_vec(int* vector)
{
    cout << "Vector Values:\n [";
    for (int i=0; i<SIZE; i++)
    {
        if (i == SIZE-1)
        {
            cout << vector[i] << "]" << endl;
        }
        else if (i % 40 != 0 || i == 0)
        {
            cout << vector[i] << ",";
        }
        else
        {
            cout << vector[i] << "]\n [";
        }
    }
}

// Writes result to text file. 
void write_vec(string f_name, int duration, int* vector)
{
    ofstream fstream(f_name); // Creates the txt file. 

    // Writes the calculation time. 
    fstream << "\nRESULTS FOR OMP PARALLEL PROCESSING - Time & Output Vector." << endl;
    fstream << "Vector Input Size: " << SIZE << endl;
    fstream << "\nCalculation Time: " << duration << " microseconds." << endl;
    fstream << "                  " << duration/1000000.0 << " seconds." << endl;

    // Writes the matrix passed to the file. 
    fstream << "\n";
    fstream << "Vector Values:\n [";
    for (int i=0; i<SIZE; i++)
    {
        if (i == SIZE-1)
        {
            fstream << vector[i] << "]" << endl;
        }
        else if (i % 50 != 0 || i == 0)
        {
            fstream << vector[i] << ",";
        }
        else
        {
            fstream << vector[i] << "]\n [";
        }
    }
    fstream.close(); // Closes the file.
}

// Prints the final results to the console. 
void print_results(int size, int duration)
{
    cout << "\nVector of Length " << SIZE << ":" << endl;
    cout << " Total Processing time: " << duration << " milliseconds." << endl;
    cout << "                        " << duration / 1000000.0 << " seconds.\n" << endl;
}

// Swaps elements in the array (Quicksort Helper Procedure).
void swap(int* a, int* b)
{
    int* temp = a;
    a = b;
    b = temp;
}

// Re-arrange the array based on the pivot point (Quicksort Pivot Function).
int partition(int* vec, int low, int high)
{
    int pivot = vec[high];  // Uses the last element as the pivot
    int i = low-1;          // Pointer for the greater element.

    // printf("Thread %d is PIVOTING.\n", omp_get_thread_num()); 
    // cout << " LOW:" << low << " High: " << high << "\n [";
    // for (int k=low; k<high; k++) {
    //     cout << vec[k] << ",";
    //     } cout << vec[high] << "]" << endl;

    for (int j=low; j<high; j++)
    {   // Ensures the values <= Pivot are placed on the left side of the array.
        if (vec[j] <= pivot)
        {
            i++;
            swap(vec[i], vec[j]);
        }
    }
    // Final Swap to place the pivot value in its correct position.
    swap(vec[i + 1], vec[high]);
    // Returning the index of the pivot point.
    return (i+1);
}

// Recursive QuickSort
void quick_sort(int* vec, int low, int high)
{
    if (low < high)
    {
        // Reordering the vector & retrieving the pivot point
        int pivot = partition(vec, low, high);

        // Recursive call left of the pivot point
        quick_sort(vec, low, pivot-1);

        // Recursive call right of the pivot point.
        quick_sort(vec, pivot+1, high);
    }
}

// Recursive QuickSort (Main Quicksort Procedure).
void quick_sort_OMP(int* vec, int low, int high, int limit)
{
    if (low < high)
    {
        // Ensures that no additional tasks are allocated if vec is < limit.
            // Keeps task creation overhead reasonable. 
        if ((high-low) < limit)
        {   // Calls sequential Quick Sort.
            quick_sort(vec, low, high);
        }
        else
        {
            printf("Thread %d is entering quick_sort method.\n", omp_get_thread_num()); 
            // Reordering the vector & retrieving the pivot point
            int pivot = partition(vec, low, high);

            // Explicitly defines the task and the shared data.
            #pragma omp task shared(vec)
            // Recursive call left of the pivot point
            quick_sort_OMP(vec, low, pivot-1, limit);

            // Recursive call right of the pivot point.
            #pragma omp task shared(vec)
            quick_sort_OMP(vec, pivot+1, high, limit);
        }
    }
}

int main()
{
    // Allocating the memory for the array.
    int* v1 = (int*) malloc(sizeof(int) * SIZE);
    //unsigned int seed = 3; // Seed to produce same result for testing. 
    int limit = 1000; // Limits the size of Vec that OMP will create additional threads for.

    auto start = chrono::high_resolution_clock::now(); // Timer START.
    
    // OpenMP Parallised section - Using Auto Scheduling to fill Vector with random values.
    #pragma omp parallel shared(v1, limit) default(none)
    {
        int seed = omp_get_thread_num(); // Seeding with thread number.

        printf("Thread %d is entering rand_fill method.\n", omp_get_thread_num()); 
        rand_fill(v1, SIZE, seed); // Fill Vector with random values 0-999.

        #pragma omp barrier  // Wait for all threads to finish filling vector.  
    }
    
     // Writting the unordered vector to a txt file. 
    write_vec("OMP_Quicksort_OG_Vec", 5, v1);  

    #pragma omp parallel // OpenMP Parallised section 
    {
        printf("Thread %d is entering quick_sort method.\n", omp_get_thread_num()); 

        // OMP single ensures a single master task controls the process. 
            // Once inside Q-Sort, OMP Task generates children to work in parallel.  
        #pragma omp single
        quick_sort_OMP(v1, 0, SIZE-1, limit); // Quick sort start.
        // Waits for the child tasks to complete their work.
        #pragma omp taskwait
    }  

    auto stop = chrono::high_resolution_clock::now(); // Timer STOP.

    int duration = duration_cast<microseconds>(stop - start).count(); // Time Calc.

    write_vec("OMP_Quicksort_Sorted_Vec", duration, v1); // Writing the result to a txt file. 
    
    print_results(SIZE, duration); // Display results to the console. 

    free(v1); // Free the memory.
    v1 = NULL;

    return 0;

}