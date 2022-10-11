//ToDo: Add Comment (what is the purpose of this function? Where its going to get executed?)

// This function declares "square_magnitude" as a kernal and makes it visable to the host 
// code so it can be enqueued and executed by an application on an OpenCL device. 
    // It can be executed on the OpenCL device only.
    // It can be called by the host
    // If another _kernel function calls it, it is treated like a regular function call. 
__kernel void square_magnitude(const int size, __global int* v) {
    
    // Thread identifiers
    const int globalIndex = get_global_id(0);   
 
    //uncomment to see the index each PE works on
    printf("Kernel process index :(%d)\n ", globalIndex);

    v[globalIndex] = v[globalIndex] * v[globalIndex];
}

// Vector addition kernel function.
__kernel void add_vector(const int size, __global int* v1, __global int* v2, __global int* v3) {
    
    // Thread identifiers
    const int globalIndex = get_global_id(0);   

    v3[globalIndex] = v1[globalIndex] + v2[globalIndex];
}
