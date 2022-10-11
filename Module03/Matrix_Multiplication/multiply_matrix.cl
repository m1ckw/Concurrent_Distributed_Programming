// This function declares "multiply_matrix" as a kernal and makes it visable to the host 
// code so it can be enqueued and executed by an application on an OpenCL device. 
    // It can be executed on the OpenCL device only.
    // It can be called by the host
    // If another _kernel function calls it, it is treated like a regular function call. 

    // M: Rows of A assigned to each process
    // N: Columns of A and also the rows of B
    // K: Columns of B

__kernel void multiply_matrix( const int M, const int N, const int K, 
                                const __global int *A, const __global int *B, __global int *C)
{
    // Thread Identifiers 
    const int i = get_global_id(0); // Row ID of Matrix C (0...M)
    const int j = get_global_id(1); // Col ID of Matric C (0...N)

    // Compute a single element with each loop of K.
    float sum = 0;
    for (int k=0; k<N; k++)
    {
        sum += A[i*N + k] * B[k*N + j];
    }
    
    C[i*N + j] = sum; // Sore the results.

    /*
        NOTE: 
    */
}
