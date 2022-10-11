// COMPILE COMMAND: g++ vector_ops2.cpp -o vec2 -lOpenCL

#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <cstdlib>
#include <chrono>

#define PRINT 1

using namespace std;
using namespace std::chrono;

int SZ = 150000000;
int *v1, *v2, *v3; // Creating addition Pointers & corrosponding buffers below.

// THE FOLLOWING DECLARATIONS ARE: (Data_type Variable_name) which are initialised later in the code.
//Comment: Declares a buffer memory object variable which is needed for standard OpenCl API calls.
cl_mem bufV1, bufV2, bufV3;
//Comment: Declares an ID variable for the current active device and stores it in "device_id".
cl_device_id device_id;
//Comment: Declares a context object which is the environment in which the kernel is executed and memory managment is defined.
cl_context context;
//Comment: Declares a program object that will consist of a set of kernels identified with the "kernel" qualifier.
cl_program program;
//Comment: Declares a kernal object which will encapsulate the specific __kernel function.
cl_kernel kernel;
//Comment: Declares a Command Queue object. Memory, program and kernel objects are created using a context. Operations
    // on these objects are performed using a command queue.
cl_command_queue queue;
//Comment: Declares an empty event object. Event objects can be used to track the status of a command.
cl_event event = NULL;
////////////////////////////////////////////////////////////////////////////////////////////////////

int err; // A variable to hold error codes.

// THE FOLLOWING DECLARATIONS ARE: Function signatures/prototypes defining parameters and return types.
//Comment: Function to create a device and return the device ID.
cl_device_id create_device();
//Comment: One of the main functions; Creates the device, defines the context, creates the command queue and kernel.
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname);
//Comment: Function to define the program setting the context, device, and passing the file name. Returns program object.
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);
//Comment: Function to define the buffer object and write the buffer object to the host memory (command queue).
void setup_kernel_memory();
//Comment: Function to set the argument values for a specific argument of multiple kernels.
void copy_kernel_args();
//Comment: Function to free all allocated memory for buffers and all objects.
void free_memory();

// More function signatures:
    // Initialise the vector & print the vector.
void init(int *&A, int size);
void print(int *A, int size);
int sum_vec(int *vec, int size);
////////////////////////////////////////////////////////////////////////////////////////////////////

// Main method
int main(int argc, char **argv)
{
    if (argc > 1)
        SZ = atoi(argv[1]);

    init(v1, SZ);
    init(v2, SZ);
    init(v3, SZ);

    //Comment: Defines the global work size i.e. allocates a region of global memory that remains constant
        // during the execution of a kernel and is accessible to all work itmes executing in a context.
    size_t global[1] = {(size_t)SZ};

    //initial vector
    //print(v1, SZ); //Printing of vectors for testing.
    //print(v2, SZ);

    auto start = high_resolution_clock::now();

    setup_openCL_device_context_queue_kernel((char *)"./vector_ops.cl", (char *)"add_vector");

    setup_kernel_memory();
    copy_kernel_args();

    //Comment: Enqueues a command to execute a kernel on a device, returns cl_int. Parameters;
        // Queue: The command queue itself, a valid host command-queue defined around line 200.
        // Kernel: The kernel object that holds the main kernal function.
        // 1: The number of dimensions used to specify the global work items in the work group.
        // NULL: The global work-offset, NULL indicates that the global IDs start at offset (0, 0, 0)
        // Global: The Global work size as defined above on line 65.
        // 0: The local work size.
        // NULL: The number of events in the wiat list. NULL indicates that the that this event will
            // not wait on any other event to complete.
        // Event: Returns an event object which can be used to identify a partiqular kernel instance.
            // Ass event is set to NULL above, no event will be created for this instance.
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);

    //Comment: Function command to read from, or write to, a buffer object from host memory. Parameters;
        // Queue: The command queue itself, a valid host command-queue defined around line 200.
        // Bufv: The buffer object delared above and defined in set_up_kernal_memory below.
        // CL_TRUE: Blocking_read, as it is True the read command is blocking, clEnqueueReadBuffer
            // does not return until the buffer data has been read and copied into memory pointed to by ptr
        // 0: Offset in bytes in the buffer object to read from and write to.
        // SZ * sizeof(int): Size of the data in bytes being read or written.
        // v[0]: Pointer to the buffer in host memory where data is to be read into and written from.
        // 0: Number of events in waitlist.
        // NULL: Event waitlist, as num of events is 0, thsi must be NULL.
        // NULL: Returns an event object, as it is NULL, it will not be possible for the application
            // to query the status of this command or queue.
    clEnqueueReadBuffer(queue, bufV3, CL_TRUE, 0, SZ * sizeof(int), &v3[0], 0, NULL, NULL);

    auto stop = high_resolution_clock::now();
    int duration = duration_cast<milliseconds>(stop - start).count();
    int sum = sum_vec(v3, SZ);

    //result vector
    // print(v3, SZ);
    printf("Total Number of Elements: %d. The Total Sum: %d\n", SZ, sum);
    printf(" Total processing time: %d milliseconds.\n\t    In seconds: %f\n",
            duration, duration/1000.0);

    //frees memory for device, kernel, queue, etc.
    //you will need to modify this to free your own buffers
    free_memory();
}
// END OF MAIN

////////////////////////////////////////////////////////////////////////////////////////////////////
// The Actual Function Implementations from the above signatures.
void init(int *&A, int size)
{
    A = (int *)malloc(sizeof(int) * size);

    for (long i = 0; i < size; i++)
    {
        A[i] = rand() % 100; // any number less than 100
    }
}

void print(int *A, int size)
{
    if (PRINT == 0)
    {
        return;
    }

    if (PRINT == 1 && size > 15)
    {
        for (long i = 0; i < 5; i++)
        {                        //rows
            printf("%d ", A[i]); // print the cell value
        }
        printf(" ..... ");
        for (long i = size - 5; i < size; i++)
        {                        //rows
            printf("%d ", A[i]); // print the cell value
        }
    }
    else
    {
        for (long i = 0; i < size; i++)
        {                        //rows
            printf("%d ", A[i]); // print the cell value
        }
    }
    printf("\n----------------------------\n");
}

int sum_vec(int* vec, int size)
{
    int sum = 0;
    for (int i=0; i<size; i++)
    {
        sum += vec[i];
    }
    return sum;
}

void free_memory()
{
    //free the buffers
    clReleaseMemObject(bufV1);
    clReleaseMemObject(bufV2);
    clReleaseMemObject(bufV3);

    //free opencl objects
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);

    free(v1);
    free(v2);
    free(v3);
}


void copy_kernel_args()
{
    //Comment: Sets the argument value for a specific argument of a kernel.
            // To execute a kernel, the kernel arguments must be set.
     // Parameters;
        // Kernel: The kernal object as defined in setup_openCL_device_context_queue_kernel.
        // 0: Arg index, Arguments to the kernel are referred by indices that go from 0 for the leftmost argument to n - 1
        // Sizeof(int): Argument size, If the argument is a memory object, the size is the size of the memory object.
        // (void *)&SZ: arg_value is a pointer to data that should be used as the argument value for argument specified by arg_index.
    clSetKernelArg(kernel, 0, sizeof(int), (void *)&SZ);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bufV1);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&bufV2);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&bufV3);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void setup_kernel_memory()
{
    //Comment: Creates the buffer object. Parameters;
        // Context: A valid OpenCL context used to create the buffer object as defined on line 220.
        // CL_MEM_READ_WRITE: is a flag, a bit-field that is used to specify allocation and usage
            // information such as the memory that should be used to allocate the buffer object and how it will be used.
            // There seems to be 10 such flages, this one in partiqular specifies that the memory object will be read
            // and written by a kernel. This is the default.
        // SZ * sizeof(int): The size in bytes of the buffer memory object to be allocated.
        // NULL: host_ptr is a pointer to the buffer data that may already be allocated by the application.
        // NULL: Error code return, NULL means nothing will be returned.
    bufV1 = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * sizeof(int), NULL, NULL);
    bufV2 = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * sizeof(int), NULL, NULL);
    bufV3 = clCreateBuffer(context, CL_MEM_READ_WRITE, SZ * sizeof(int), NULL, NULL);

    // Copy vectors to the GPU
    clEnqueueWriteBuffer(queue, bufV1, CL_TRUE, 0, SZ * sizeof(int), &v1[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufV2, CL_TRUE, 0, SZ * sizeof(int), &v2[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufV3, CL_TRUE, 0, SZ * sizeof(int), &v3[0], 0, NULL, NULL);
}

void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname)
{
    device_id = create_device();
    cl_int err;

    //Comment: Defines the context, the environment within which kernels execute and in which synchronization
        // and memory management is defined. Parameters;
        // NULL: Properties, specifies a list of context property names and their corresponding values
        // 1: Number of devices.
        // device_id: devices, a pointer to a list of unique devices returned by clGetDeviceIds.
        // NULL: pfn_notify is a Callback function, used by the OpenCL implementation to report information
            // on errors during context creation.
        // NULL: user_data will be passed as the user_data argument when pfn_notify is called, both are NULL no not relevant here.
        // err:  Returns int ascociated with error, if err is set to NULL, no error code will be retunred.
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err < 0)
    {
        perror("Couldn't create a context");
        exit(1);
    }

    program = build_program(context, device_id, filename);

    //Comment: Creates a host or device command queue on a specific device.
        // Parameters:
            // Context: A valid OpenCL context used to create the buffer object as defined on line 220.
            // Device: Must be a device or subdevice associated with the context.
            // Properties (0): Specifies a list of properties for the command-queue and their corresponding values.
                // The list is terminated with 0
            // Error code: Returns int ascociated with error, if err is set to NULL, no error code will be retunred.
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    if (err < 0)
    {
        perror("Couldn't create a command queue");
        exit(1);
    };


    kernel = clCreateKernel(program, kernelname, &err);
    if (err < 0)
    {
        perror("Couldn't create a kernel");
        printf("error =%d", err);
        exit(1);
    };
}

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

    //Comment: Creates a program object for a context, and loads the source code specified by the text
        // strings in the strings array into the program object.
        // Parameters;
        // ctx: A valid OpenCL context used to create the buffer object as defined on line 220.
        // 1: cl_unit count.
        // (const char **)program_buffer: strings is an array of count pointers to optionally null-terminated character strings
            // that make up the source code.
        // program_size: lengths argument is an array with the number of chars in
            // each string (the string length).
        // err: Returns error code.
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
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);
        program_log = (char *)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
    }

    return program;
}

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
