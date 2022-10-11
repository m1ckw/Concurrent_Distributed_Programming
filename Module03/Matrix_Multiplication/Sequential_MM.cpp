// On Mac RUN WITH: clang++ Sequential_MM.cpp -o seq -std=c++11
// On Windows RUN WITH: g++ Sequential_MM.cpp -o seq -std=c++11

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>

using namespace std::chrono;
using namespace std;

#define SIZE 1200 // For ease of adjusting matrix size.

// Initialise Matrix bay allocating memory space.
void init_matrix(int **&matrix, int size)
{
    // Array of pointers 
    matrix = (int**) malloc(sizeof(int*) * size);
    // 2nd allocation for integer space. 
    for (int i=0; i<size; i++) 
    {
        matrix[i] = (int*) malloc(sizeof(int) * size);
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

// Deallocates the memory space. 
void deallocate_matrix(int **&matrix, int size)
{
    for (int i=0; i<size; i++) 
    {
        free(matrix[i]); 
        matrix[i] = NULL;
    }
    free(matrix); 
    matrix = NULL;
}

// Random generation of values to populate matrix.
void random_matrix(int **&matrix, unsigned int seed)
{
    for (int i=0; i<SIZE; i++)
    {
        for (int j=0; j<SIZE; j++)
        {
            matrix[i][j] = rand_r(&seed) % 100;
        }
    } 
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

// Writes the matrix to a text file.
void write_matrix(string f_name, int calc_time, int **&matrix)
{
    ofstream fstream(f_name); // Creates the txt file. 

    // Writes the calculation time. 
    fstream << "\nRESULTS FOR SEQUENTIAL PROCESSING - Time & Output Matrix." << endl;
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
void multiply_matrix(int **&matrix_a, int **&matrix_b, int **&result_m)
{
    for (int i=0; i<SIZE; i++)
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

 // Print Summary of Calulation Time.
void print_results(int calc_time, int size)
{
    cout << "\nCalculation time: " << calc_time << " microseconds.\nSEQUENTIAL processing of a " 
        << SIZE << " X " << SIZE << " Matrix." << endl;
    cout << "                  " << calc_time/1000000.0 << " seconds." << endl;
    cout << endl;
}

int main() 
{
    int **matrix_a, **matrix_b, **result_m;
    
    // Allocateing the memory Space for each Matrix
    init_matrix(matrix_a, SIZE);
    init_matrix(matrix_b, SIZE);
    init_matrix(result_m, SIZE);

    // Ensuring no garbage values exist
    init_zero(result_m);

    // Populate matrices a and b with random values. 
    random_matrix(matrix_a, 5);
    random_matrix(matrix_b, 3);

    // print_matrix(matrix_a); // TEST Print Functions. 
    // print_matrix(matrix_b);
    // print_matrix(result_m);

    // Start Timer 
    auto start = high_resolution_clock::now();
    
    multiply_matrix(matrix_a, matrix_b, result_m);

    // Stop Timer
    auto stop = high_resolution_clock::now();

    // Calculate Time taken
    int calc_time = duration_cast<microseconds>(stop - start).count();

    // Write results to text file. 
    write_matrix("sequential.txt", calc_time, result_m);

    // print_matrix(result_m); // TEST Print Function. 

    // Print Summary of Calulation Time.
    print_results(calc_time, SIZE);

    // Deallocate Memory & set pointer to NULL.
    deallocate_matrix(matrix_a, SIZE); 
    deallocate_matrix(matrix_b, SIZE); 
    deallocate_matrix(result_m, SIZE); 

    return 0;
}