// Compile: mpicxx MPI_MM.cpp -o mpi
// Run Head: mpirun -np 4 ./mpi
// Run Cluster: sudo mpirun -np 4 -hostfile ./cluster ./mpi

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <mpi.h>

using namespace std::chrono;
using namespace std;

#define SIZE 1200 // For ease of adjusting matrix size.
int **matrix_a, **matrix_b, **result_m; // Global Pointers

// Initialise Matrix 
void init_matrix(int **&matrix, bool fill);

// Deallocates the memory for the matrices 
void deallocate_memory();

// Sets all values in the matrix passed to zero.
void init_zero(int **&matrix);

// Prints the matrix passed to the screen 
void print_matrix(int **&matrix);

// Prints the performance results
void print_results(int calc_time, int size);

// Writes the matrix to a text file.
void write_matrix(string f_name, int calc_time, int **&matrix);

// Takes three matrices, multipys a and b storing the result in the result_m matrix. 
void multiply_matrix(int **&matrix_a, int **&matrix_b, int **&result_m, int partition);

// Head Node process control function
void head_node(int num_tasks, int rank);

// Worker Node process control function
void worker_node(int num_tasks, int rank);


int main(int argc, char **argv) 
{
    int num_pocesses, rank, name_len;
    char name[MPI_MAX_PROCESSOR_NAME];

    // Setting up and starting the parallel process.
    MPI_Status status; // Required to receive routines.
    MPI_Init(&argc, &argv); // Initialise the MPI environment.
    MPI_Comm_size(MPI_COMM_WORLD , &num_pocesses); // Get the number of tasts/processes.
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get the rank i.e. process ID.
    MPI_Get_processor_name(name, &name_len); // Find the processors name. 

    auto start = high_resolution_clock::now();

    if (rank == 0)
    {
        printf("\n%s is entering the head_node function", name);
        head_node(num_pocesses, rank);
    }
    else
    {
        printf("\n%s is entering the worker_node function", name);
        worker_node(num_pocesses, rank);
    }
    int val = MPI_Barrier(MPI_COMM_WORLD);
    //printf("\n%s has completed all work", name);

    if (rank == 0)
    {
        // Stop Timer
        auto stop = high_resolution_clock::now();

        // Calculate Time taken
        int calc_time = duration_cast<microseconds>(stop - start).count();

        // Write results to text file. 
        write_matrix("MPI_Distributed.txt", calc_time, result_m);

        // print_matrix(result_m); // TEST Print Function. 

        // Print Summary of Calulation Time.
        print_results(calc_time, SIZE);

        // Deallocate Memory & set pointer to NULL. 
        deallocate_memory(); 
    }

    MPI_Finalize(); // Finalise the MPI environment.

    return 0;
}

// Initialise Matrix 
void init_matrix(int **&matrix, bool fill)
{
    // Allocate the memory to the arrays
    matrix = (int**) malloc(sizeof(int*) * SIZE * SIZE);
    int* temp = (int *) malloc(sizeof(int) * SIZE * SIZE);

    for (int i = 0; i < SIZE; ++i)
    {
        matrix[i] = &temp[i * SIZE];
    }

    // Populate with random values.
    if (fill) 
    {
        for (int i = 0; i < SIZE; ++i)
        {
            for (int j = 0; j < SIZE; ++j)
            {
                matrix[i][j] = rand() % 100;
            }
        }
    }
    
}

// Sets all values in the matrix passed to zero.
void init_zero(int **&matrix)
{
    for (int i=0; i<SIZE; i++)
    {
        for (int j=0; j<SIZE; j++)
        {
            matrix[i][j] = 0;
        }
    }
}

// Head Node Tasks
void head_node(int num_pocesses, int rank)
{
    
    // Declare Matrices and allocate memory for each.
    // Populate matrices a and b with random values. 
    init_matrix(matrix_a, true);
    init_matrix(matrix_b, true);
    init_matrix(result_m, false);

    // print_matrix(matrix_a); // TEST Print Function.
    // print_matrix(matrix_b);

    int partition = SIZE/num_pocesses; // Number of rows per process
    int broadcast_size = (SIZE * SIZE); // Number of elements to be broadcast
    int scatter_size = (SIZE * SIZE)/num_pocesses; // Number of elements to be scattered

    // Scatter Matrix A to Nodes
    MPI_Scatter(&matrix_a[0][0], scatter_size, MPI_INT, &matrix_a, 0, MPI_INT, 0, MPI_COMM_WORLD);

    // Broadcast Entire Matrix B to Nodes
    MPI_Bcast(&matrix_b[0][0], broadcast_size, MPI_INT, 0, MPI_COMM_WORLD);

    // Perform the multiplication 
    multiply_matrix(matrix_a, matrix_b, result_m, partition);
    printf("\nWorker: %d has Completed Matrix Multiplication", rank);

    // Gather results from Nodes and write to resuts matrix
    MPI_Gather(MPI_IN_PLACE, scatter_size, MPI_INT, &result_m[0][0], scatter_size, MPI_INT, 0, MPI_COMM_WORLD);

}

// Worker Node tasks
void worker_node(int num_pocesses, int rank)
{
    // Declare and allocate memory to receive data sent. 
    init_matrix(matrix_a, false);
    init_matrix(matrix_b, false);
    init_matrix(result_m, false);

    int partition = SIZE/num_pocesses; // Number of rows per process
    int broadcast_size = (SIZE * SIZE); // Number of elements to be broadcast
    int scatter_size = (SIZE * SIZE)/num_pocesses; // Number of elements to be scattered

    // Receive the matrix data from the head node. 
    MPI_Scatter(NULL, scatter_size, MPI_INT, &matrix_a[0][0], scatter_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&matrix_b[0][0], broadcast_size, MPI_INT, 0, MPI_COMM_WORLD);
    

    // Perform the multiplication 
    multiply_matrix(matrix_a, matrix_b, result_m, partition);
    printf("\nWorker: %d has Completed Matrix Multiplication", rank);

    // Gather the results
    MPI_Gather(&result_m[0][0], scatter_size, MPI_INT, NULL, scatter_size, MPI_INT, 0, MPI_COMM_WORLD);

}

// Prints the matrix passed to the screen 
void print_matrix(int **&matrix)
{
    cout << "\n";
    for (int i=0; i<SIZE; i++)
    {
        cout << "|";
        for (int j=0; j<SIZE; j++)
        {
            cout << matrix[i][j] << "|";
        }
        cout << endl;
    }
}

// Print Summary of Calulation Time.
void print_results(int calc_time, int size)
{
    cout << "\n\nCalculation time: " << calc_time << " microseconds.\nMPI Distributed processing of a " 
        << size << " X " << size << " Matrix." << endl;
    cout << "                  " << calc_time/1000000.0 << " seconds." << endl;
    cout << endl;
}

// Writes the matrix to a text file.
void write_matrix(string f_name, int calc_time, int **&matrix)
{
    ofstream fstream(f_name); // Creates the txt file. 

    // Writes the calculation time. 
    fstream << "\nRESULTS FOR MPI Distributed PROCESSING - Time & Output Matrix." << endl;
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
void multiply_matrix(int **&matrix_a, int **&matrix_b, int **&result_m, int partition)
{
    for (int i=0; i<partition; i++)
    {
        for (int j=0; j<SIZE; j++)
        {
            for (int k=0; k<SIZE; k++)
            {
                result_m[i][j] += (matrix_a[i][k] * matrix_b[k][j]);
            }
        }
    }
}

void deallocate_memory()
{
    free(matrix_a);
    free(matrix_b);
    free(result_m); 

    matrix_a = NULL;
    matrix_b = NULL;
    result_m = NULL;
}

