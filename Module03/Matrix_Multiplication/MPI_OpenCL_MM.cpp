// Compile: mpicxx MPI_OpenCL_MM.cpp -o ocl -lOpenCL
// Run Cluster: sudo mpirun -np 4 -hostfile ./cluster ./ocl

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <mpi.h>
#include <CL/cl.h>

using namespace std::chrono;
using namespace std;

#define SIZE 190 // For ease of adjusting matrix size.
const int TS = 4;
int **matrix_a, **matrix_b, **result_m; // Global Pointers

// OpenCL Variables
cl_mem buf_mA, buf_mB, buf_mR; // Declares a buffer memory object for each matrix.
cl_device_id device_id; // Active Device ID.
cl_context context; // Context where kernel is executed and memory managment is defined.
cl_program program; // Program consisting of kernels identified with the "kernel" qualifier.
cl_kernel kernel; // Kernal object to encapsulate the specific __kernel function.
cl_command_queue queue; // Command Queue to run Kernal functions.
cl_event event = NULL; // Event object used to track the status of a command.
int err; // A variable to hold error codes.

size_t local[2] = {TS, TS}; // Local Working group size (i.e. 2D matrices)
size_t global[2] = {(size_t)SIZE, (size_t)SIZE}; // Number of rows & columns: Or number of threads with indices i & J (row & column) fr results_m.

//////////////////////////////// OpenCL FUNCTIONS Sigs ////////////////////////////////
// Create a device and return the device ID.
cl_device_id create_device();

// Creates the device, defines the context, creates the command queue and kernel.
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname);

// Defines the program; sets context, device, and passing the file name. Returns program object.
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);

// Defines buffer object and writes buffer object to host memory (command queue).
void setup_kernel_memory(int partition); // Partition represents the size of data chunks to be distributed.

// Copies the kernel arguments.
void copy_kernel_args(int partition); // Partition represents the size of data chunks to be distributed.

// Free all allocated memory for buffers and all objects.
void free_memory();

//////////////////////////////// STANDARD FUNCTIONS Sigs ////////////////////////////////
// Initialise Matrix
void init_matrix(int **&matrix, bool fill);

// Prints the matrix passed to the screen
void print_matrix(int **&matrix);

// Prints the performance results
void print_results(int calc_time, int size);

// Writes the matrix to a text file.
void write_matrix(string f_name, int calc_time, int **&matrix);

// Takes three matrices, multipys a and b storing the result in the result_m matrix.
// void multiply_matrix(int **&matrix_a, int **&matrix_b, int **&result_m, int partition);

// Head Node process control function
void head_node(int num_tasks, int rank);

// Worker Node process control function
void worker_node(int num_tasks, int rank);

// Deallocates the memory for the matrices
void deallocate_memory();

// Initiates OpenCL when called
void run_openCL(int partition);


int main(int argc, char **argv)
{
    int num_pocesses, rank, name_len;
    char name[MPI_MAX_PROCESSOR_NAME];

    // Setting up and starting the parallel process.
    //MPI_Status status; // Required to receive routines.
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
    printf("\n%s has completed all work", name);

    if (rank == 0)
    {
        // Stop Timer
        auto stop = high_resolution_clock::now();

        // Calculate Time taken
        int calc_time = duration_cast<microseconds>(stop - start).count();

        // Write results to text file.
        write_matrix("MPI_OpenCL_Dist_Parallel.txt", calc_time, result_m);

        print_matrix(result_m); // TEST Print Function.

        // Print Summary of Calulation Time.
        print_results(calc_time, SIZE);

        // Deallocate Memory & set pointer to NULL.
        deallocate_memory();
    }

    MPI_Finalize(); // Finalise the MPI environment.
    free_memory();

    return 0;
}


// Initialise Matrix
void init_matrix(int **&matrix, bool fill)
{
    // Allocate the memory to the arrays
    matrix = (int**) malloc(sizeof(int*) * SIZE * SIZE);
    int* column = (int *) malloc(sizeof(int) * SIZE * SIZE);

    for (int i = 0; i < SIZE; ++i)
    {
        matrix[i] = &column[i * SIZE];
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

// Initialise Matrix
void init_matrix(int **&matrix, int rows)
{
    // Allocate the memory to the arrays
    matrix = (int**) malloc(sizeof(int*) * rows * SIZE);
    int* column = (int *) malloc(sizeof(int) * SIZE * SIZE);

    for (int i = 0; i < SIZE; ++i)
    {
        matrix[i] = &column[i * SIZE];
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
    // print_matrix(result_m);

    int partition = SIZE/num_pocesses; // Number of rows per process
    int broadcast_size = (SIZE * SIZE); // Number of elements to be broadcast
    int scatter_size = (SIZE * SIZE)/num_pocesses; // Number of elements to be scattered

    // Scatter Matrix A to Nodes
    MPI_Scatter(&matrix_a[0][0], scatter_size, MPI_INT, &matrix_a, 0, MPI_INT, 0, MPI_COMM_WORLD);
    // Broadcast Entire Matrix B to Nodes
    MPI_Bcast(&matrix_b[0][0], broadcast_size, MPI_INT, 0, MPI_COMM_WORLD);

    // Perform the multiplication
    //run_openCL(partition);
    local[0] = partition, local[1] = SIZE;
    global[0] = partition, global[1]= SIZE;
    // Setup the device, define context, create command queue and kernel.
    setup_openCL_device_context_queue_kernel((char *)"./multiply_matrix.cl", (char *)"multiply_matrix");
    setup_kernel_memory(partition);
    copy_kernel_args(partition);
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, local, 0, NULL, &event);
    clWaitForEvents(1, &event);
    clEnqueueReadBuffer(queue, buf_mR, CL_TRUE, 0, partition * SIZE * sizeof(int), &result_m[0][0], 0, NULL, NULL);
    //multiply_matrix(matrix_a, matrix_b, result_m, partition);
    printf("\nWorker: %d has Completed Matrix Multiplication", rank);

    // Gather results from Nodes and write to resuts matrix
    MPI_Gather(MPI_IN_PLACE, scatter_size, MPI_INT, &result_m[0][0], scatter_size, MPI_INT, 0, MPI_COMM_WORLD);
}

// Worker Node tasks
void worker_node(int num_pocesses, int rank)
{
    int partition = SIZE/num_pocesses; // Number of rows per process
    int broadcast_size = (SIZE * SIZE); // Number of elements to be broadcast
    int scatter_size = (SIZE * SIZE)/num_pocesses; // Number of elements to be scattered


    // Declare and allocate memory to receive data sent.
    init_matrix(matrix_a, partition);
    init_matrix(matrix_b, false);
    init_matrix(result_m, partition);

    // Receive the matrix data from the head node.
    MPI_Scatter(NULL, scatter_size, MPI_INT, &matrix_a[0][0], scatter_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&matrix_b[0][0], broadcast_size, MPI_INT, 0, MPI_COMM_WORLD);


    // Perform the multiplication
    //run_openCL(partition);
    local[0] = partition, local[1] = SIZE;
    global[0] = partition, global[1]= SIZE;
    // Setup the device, define context, create command queue and kernel.
    setup_openCL_device_context_queue_kernel((char *)"./multiply_matrix.cl", (char *)"multiply_matrix");
    setup_kernel_memory(partition);
    copy_kernel_args(partition);
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, local, 0, NULL, &event);
    clWaitForEvents(1, &event);
    clEnqueueReadBuffer(queue, buf_mR, CL_TRUE, 0, partition * SIZE * sizeof(int), &result_m[0][0], 0, NULL, NULL);

    // multiply_matrix(matrix_a, matrix_b, result_m, partition);
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
    fstream << "\nRESULTS FOR MPI OpenCL Distributed & Parallel PROCESSING - Time & Output Matrix." << endl;
    fstream << "Matrix Size: " << SIZE << " x " << SIZE << endl;
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
// void multiply_matrix(int **&matrix_a, int **&matrix_b, int **&result_m, int partition)
// {
//     for (int i=0; i<partition; i++)
//     {
//         for (int j=0; j<SIZE; j++)
//         {
//             for (int k=0; k<SIZE; k++)
//             {
//                 result_m[i][j] += (matrix_a[i][k] * matrix_b[k][j]);
//             }
//         }
//     }
// }

void deallocate_memory()
{
    free(matrix_a);
    free(matrix_b);
    free(result_m);

    matrix_a = NULL;
    matrix_b = NULL;
    result_m = NULL;
}

// Free memory.
void free_memory()
{
    //free the buffers
    clReleaseMemObject(buf_mA);
    clReleaseMemObject(buf_mB);
    clReleaseMemObject(buf_mR);

    //free opencl objects
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);
}

// Copy kernel Args.
void copy_kernel_args(int partition)
{
    //Comment: Sets the argument value for a specific argument of the kernel.
            // To execute a kernel, the kernel arguments must be set.
     // Parameters;
        // Kernel: The kernal object as defined in setup_openCL_device_context_queue_kernel.
        // 0: Arg index, Arguments to the kernel are referred by indices that go from 0 for the leftmost argument to n - 1
        // Sizeof(int): Argument size, If the argument is a memory object, the size is the size of the memory object.
        // (void *)&SZ: arg_value is a pointer to data that should be used as the argument value for argument specified by arg_index.
    const int SZ = SIZE;

    clSetKernelArg(kernel, 0, sizeof(int), (void *) &partition);
    clSetKernelArg(kernel, 1, sizeof(int), (void *) &SZ);
    clSetKernelArg(kernel, 2, sizeof(int), (void *) &SZ);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *) &buf_mA);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *) &buf_mB);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *) &buf_mR);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

// Set up memory buffers.
void setup_kernel_memory(int partition)
{
    //Comment: Creates the buffer objects.
    buf_mA = clCreateBuffer(context, CL_MEM_READ_WRITE, partition * SIZE * sizeof(int), NULL, NULL);
    buf_mB = clCreateBuffer(context, CL_MEM_READ_WRITE, SIZE * SIZE * sizeof(int), NULL, NULL);
    buf_mR = clCreateBuffer(context, CL_MEM_READ_WRITE, partition * SIZE * sizeof(int), NULL, NULL);

    // Copy matrices to the GPU
    clEnqueueWriteBuffer(queue, buf_mA, CL_TRUE, 0, partition * SIZE * sizeof(int), &matrix_a[0][0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, buf_mB, CL_TRUE, 0, SIZE * SIZE * sizeof(int), &matrix_b[0][0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, buf_mR, CL_TRUE, 0, partition * SIZE * sizeof(int), &result_m[0][0], 0, NULL, NULL);
}

// Creates the device, defines the context, creates the command queue and kernel.
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname)
{
    device_id = create_device();
    cl_int err;

    // Defines the context, the environment within which kernels execute and in which synchronization.
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err < 0)
    {
        perror("Couldn't create a context");
        exit(1);
    }

    program = build_program(context, device_id, filename);

    // Creates a host or device command queue on a specific device.
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    if (err < 0)
    {
        perror("Couldn't create a command queue");
        exit(1);
    };

    // Creates the kernal object
    kernel = clCreateKernel(program, kernelname, &err);
    if (err < 0)
    {
        perror("Couldn't create a kernel");
        printf("error =%d", err);
        exit(1);
    };
}

// Builds and returns the program object.
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename)
{
    cl_program program;
    FILE *program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;

    /* Read program file and place content into buffer */
    program_handle = fopen(filename, "r");
    if (program_handle == NULL)
    {
        perror("Couldn't find the program file");
        exit(1);
    }

    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char *)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);

    //Comment: Creates a program object for a context, and loads the source code specified by the context (ctx)
    program = clCreateProgramWithSource(ctx, 1, (const char **)&program_buffer, &program_size, &err);
    if (err < 0)
    {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    /* Build program

   The fourth parameter accepts options that configure the compilation.
   These are similar to the flags used by gcc. For example, you can
   define a macro with the option -DMACRO=VALUE and turn off optimization
   with -cl-opt-disable.
   */
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err < 0)
    {

        /* Find size of log and print to std output */
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        program_log = (char *)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
    }

    return program;
}

// Create device object.
cl_device_id create_device() {

   cl_platform_id platform;
   cl_device_id dev;
   int err;

   /* Identify a platform */
   err = clGetPlatformIDs(1, &platform, NULL);
   if(err < 0) {
      perror("Couldn't identify a platform");
      exit(1);
   }

   // Access a device
   // GPU
   err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
   if(err == CL_DEVICE_NOT_FOUND) {
      // CPU
      printf("GPU not found\n");
      err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
   }
   if(err < 0) {
      perror("Couldn't access any devices");
      exit(1);
   }

   return dev;
}

// void run_openCL(int partition) // Move OpenCL Duplicate Code here.
