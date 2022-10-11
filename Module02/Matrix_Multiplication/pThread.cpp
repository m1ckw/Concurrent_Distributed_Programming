// On Mac COMPILE WITH: clang++ -pthread pThread.cpp -o pthread -std=c++11
// On Windows COMPILE WITH: g++ -pthread pThread.cpp -o pthread -std=c++11

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <pthread.h>

using namespace std::chrono;
using namespace std;

#define SIZE 1000 // Matrix size - MUST be >= THREADS.
#define THREADS 6 // Ideal number of threads = 6-12.

// Matrix data pointers
struct matrix_data
{
    int** matrix_a;
    int** matrix_b;
    int** result_m;
};

// Thread data for strat and stop positions.
struct thread_data
{
   int start;
   int stop;
   matrix_data *m_data;
};

// Sets all values in the matrix passed to zero.
void init_zero(int** matrix)
{
    for (int i=0; i<SIZE; i++)
    {
        for (int j=0; j<SIZE; j++)
        {
            matrix[i][j] = 0;
        }
    }
}

// Random generation of values to populate matrix.
void random_matrix(int** matrix, unsigned int seed)
{
    for (int i=0; i<SIZE; i++)
    {
        for (int j=0; j<SIZE; j++)
        {
            matrix[i][j] = rand_r(&seed) % 10;
        }
    }
}

// Prints the matrix passed to the screen (for testing only).
void print_matrix(int** matrix)
{
    cout << "\n";
    for (int i=0; i<SIZE; i++)
    {
        cout << "| ";
        for (int j=0; j<SIZE; j++)
        {
            cout << matrix[i][j] << " | ";
        }
        cout << endl;
    }
}

// Writes the matrix to a text file.
void write_matrix(string f_name, int calc_time, int** matrix)
{
    ofstream fstream(f_name); // Creates the txt file.

    // Writes the calculation time.
    fstream << "\nRESULTS FOR pTHREAD PARALLEL PROCESSING - Time & Output Matrix." << endl;
    fstream << "\nCalculation Time: " << calc_time << " microseconds." << endl;
    fstream << "                  " << calc_time/1000000.0 << " seconds." << endl;

    // Writes the matrix passed to the file.
    fstream << "\n";
    for (int i=0; i<SIZE; i++)
    {
        fstream << "|";
        for (int j=0; j<SIZE; j++)
        {
            fstream << matrix[i][j] << "|";
        }
        fstream << endl;
    }

    fstream.close(); // Closes the file.
}

// Takes three matrices, multipys a and b storing the result in the result_m matrix.
void *multiply_matrix(void *args)
{
    struct thread_data *t_data;
    t_data = (struct thread_data*) args;

    for (int i = t_data->start; i < t_data->stop; i++)
    {
        for (int j=0; j<SIZE; j++)
        {
            for (int k=0; k<SIZE; k++)
            {
                t_data->m_data->result_m[i][j] +=
                    (t_data->m_data->matrix_a[i][k] *
                        t_data->m_data->matrix_b[k][j]);
            }
        }
    }
    pthread_exit(NULL);
}


int main()
{
    // Allocating the required memory for each matrix.
    // Double pointers, 1st allocation for pointers.
    int** matrix_a = (int**) malloc(sizeof(int*) * SIZE);
    int** matrix_b = (int**) malloc(sizeof(int*) * SIZE);
    int** result_m = (int**) malloc(sizeof(int*) * SIZE);
    // 2nd allocation for integer space.
    for (int i=0; i<SIZE; i++)
    {
        matrix_a[i] = (int*) malloc(sizeof(int) * SIZE);
        matrix_b[i] = (int*) malloc(sizeof(int) * SIZE);
        result_m[i] = (int*) malloc(sizeof(int) * SIZE);
    }


    // Pointing the struct values to the matrices.
    matrix_data m_data;
    m_data.matrix_a = matrix_a;
    m_data.matrix_b = matrix_b;
    m_data.result_m = result_m;

    // Ensuring no garbage values exist
    init_zero(result_m);

    // Populate matrices a and b with random values.
    random_matrix(matrix_a, 5); random_matrix(matrix_b, 3);

    // print_matrix(matrix_a); // TEST Print Functions.
    // print_matrix(matrix_b);

    // Start Timer
    auto start = high_resolution_clock::now();

    // Setting the partition sizes and adjusting the number of threads.
    int partition = SIZE / THREADS;
    int remainder = SIZE % THREADS;
    int thread_num = THREADS;

    // Adds an aditional thread to handle the remainder.
    if (remainder != 0)
    {
        thread_num+= 1;
    }

    // Creating an array of threads and thread_data structs.
    pthread_t threads[thread_num];
    thread_data t_data[thread_num];

    // Allocating the partitions and putting the threads to work.
    for (int i=0; i<thread_num; i++)
    {
        t_data[i].start = partition * i;
        t_data[i].stop = t_data[i].start + partition;
        t_data[i].m_data = &m_data;

        // Dealing with the remainder partition.
        if (i == thread_num - 1)
        {
            t_data[i].stop = SIZE;
        }

        // Creates and runs the thread
        pthread_create(&threads[i], NULL, multiply_matrix, &t_data[i]);
    }

    // Joining threads
    for (int i = 0; i < thread_num; i++)
    {
        pthread_join(threads[i], NULL);
    }


    // Stop Timer
    auto stop = high_resolution_clock::now();

    // Calculate Time taken
    int calc_time = duration_cast<microseconds>(stop - start).count();

    // Write results to text file.
    write_matrix("pThread.txt", calc_time, result_m);

    // print_matrix(result_m); // TEST Print Function.

    // Print Summary of Calulation Time.
    cout << "Calculation time: " << calc_time << " microseconds for the pTHREAD processing of a "
        << SIZE << " X " << SIZE << " Matrix." << endl;
    cout << "                  " << calc_time/1000000.0 << " seconds. Running " << THREADS << " partitions." <<endl;
    cout << endl;

    // Deallocate Memory & set pointer to NULL.
    for (int i=0; i<SIZE; i++)
    {
        free(matrix_a[i]); matrix_a[i] = NULL;
        free(matrix_b[i]); matrix_b[i] = NULL;
        free(result_m[i]); result_m[i] = NULL;
    }
    free(matrix_a); matrix_a = NULL;
    free(matrix_b); matrix_b = NULL;
    free(result_m); result_m = NULL;

    return 0;
}
