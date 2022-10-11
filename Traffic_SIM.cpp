// On Mac COMPILE WITH: clang++ -pthread Traffic_SIM.cpp -o sim -std=c++11
// On Windows COMPILE WITH: g++ -pthread Traffic_SIM.cpp -o sim -std=c++11

#include <iostream>
#include <fstream>
#include <pthread.h>
#include <semaphore.h>

using namespace std;

// GLOBAL VARIABLES 
#define NUM_PRODUCERS 4     // Number of Consumers 
#define NUM_CONSUMERS 4     // Number of Producers 
#define BUFFER_SIZE 4       // Buffer Size
#define NUM_LIGHTS 10        // Number of Lights 
#define NUM_HOURS 5         // "Time Frame" of Data Collection - Not real time.

#define NUM_RESULTS 3       // WARNING: MUST BE 3 or LESS - Number of top most congested lights. 

sem_t *buff_avail_count;    // Semaphore to track available space.
sem_t *consume_flag;        // Semaphore to track available data.

// Naming Semaphores - Mac OS thing (ref below).
#define BUFFER_COUNT "/buffer_count"
#define CONSUMER_FLAG "/consumer_flag"
int insert;                 // Tracks buffer insertion position. 
int extract;                // Tracks buffer extraction position.
int **buffer;               // The buffer Matrix 
int **temp;                 // 1x4 Matrix to pass single rows of data.

pthread_mutex_t mutex_lock; // Mutual Exclusion Lock.
int **data_m;               // Main Data Matrix, data read from txt to matrix for ease of management.
int **result_m;             // Consumer populates this with Top X busiest signals from each hour.

// Thread data for producers. 
struct producer_data 
{
   int start;
   int stop;
   int id;
};

///// Functions/Procedures to generate fake traffic data file - START /////
string get_day(int value) // Switch to select Day
{
    if (value > 7) {value = value%7;}

    switch(value)
    {
        case 1: 
            return "Monday";
            break;
        case 2: 
            return "Tuesday";
            break;
        case 3: 
            return "Wednesday";
            break;
        case 4: 
            return "Thursday";
            break;
        case 5: 
            return "Friday";
            break;
        case 6: 
            return "Saturday";
            break;
        case 7: 
            return "Sunday";
            break;
        default:
            return "Lost in space and time";
    }

}

string increment_time(int hour, int min, int day) // In 5 min intervals
{
    string s_min, s_hour;
    int check_hour = hour;

    if (min == 0)
    {
        s_min = "05";
    }
    else if (min == 55)
    {
        s_min = "00";
        hour += 1;
    }
    else
    {
        s_min = to_string(min += 5);
    }

    if (hour < 10)
    {
        s_hour = "0" + to_string(hour);
    }
    else if (hour == 23 && check_hour == hour && s_min == "00")
    {
        s_hour = "00";
    }
    else if (hour == 24)
    {
        s_hour = "00";
    }
    else
    {
        s_hour = to_string(hour);
    }
    
    return  to_string(day) + "," + s_hour + s_min;
}

void create_input_data(bool generate) // Writes Data File
{
    ofstream out_file("data_file.txt"); // Creates the txt file. 
    int hour = 5, min = 55, day = 1, lights = 1010;
    string d_time;
    int sim_length = NUM_LIGHTS * NUM_HOURS * 12;

    if (generate)
    {
        for (int i=0; i<sim_length; i++) 
        {
            if (i % NUM_LIGHTS == 0)
            {
                d_time = increment_time(hour, min, day); 
                lights = 1010;

                if (min == 55)
                {
                    min = 0;
                    hour += 1;
                } 
                else 
                {
                    min += 5;
                }

                if (hour == 24 && min == 0)
                {
                    hour = 0;
                }

                if (hour == 23 && min == 55)
                {
                    day += 1;
                }
            } 

            out_file << d_time << "," << lights <<  "," << (rand()%150)/1 <<  ","  << endl;

            lights += 1;  
        }
    }
    out_file.close(); // Closes the file.
}
///// Functions/Procedures to generate fake traffic data file - FINISH /////

///// Functions/Procedures for house keeping & testing - START /////
void alloc_mem()  // Allocating the required memory.
{
    int rows = NUM_LIGHTS * NUM_HOURS * 12;
    // 1st allocation for pointers.
    data_m = (int**) malloc(sizeof(int*) * rows);
    result_m = (int**) malloc(sizeof(int*) * NUM_RESULTS * NUM_HOURS);
    buffer = (int**) malloc(sizeof(int*) * BUFFER_SIZE);
    temp = (int**) malloc(sizeof(int*) * 1);
    temp[0] = (int*) malloc(sizeof(int) * 60);

    // 2nd allocation for character space. 
    for (int i=0; i<rows; i++) 
    {
        data_m[i] = (int*) malloc(sizeof(int) * 60);
    }

    for (int i=0; i<(NUM_RESULTS * NUM_HOURS); i++) 
    {
        result_m[i] = (int*) malloc(sizeof(int) * 60);
    }

    for (int i=0; i<(BUFFER_SIZE); i++) 
    {
        buffer[i] = (int*) malloc(sizeof(int) * 60);
    }
    
}

void dealloc_mem() // Deallocate Memory & set pointer to NULL. 
{
    int rows = NUM_LIGHTS * NUM_HOURS * 12;

    for (int i=0; i<rows; i++) 
    {
        free(data_m[i]); data_m[i] = NULL;
    }
    free(data_m); data_m = NULL;
    free(result_m); result_m = NULL;
    free(buffer); buffer = NULL;
    free(temp); temp = NULL;
}

void prep_result_m() // Prep results Matrix with Zeros.
{
    // Fill results matrix with Zeros
    for (int i=0; i<(NUM_RESULTS * NUM_HOURS); i++) 
    {
        for (int j=0; j<4; j++)
            {
                result_m[i][j] = 0;
            }
    }
    // Set Hours in the hour position.
    int time = 6;

    for (int i=0; i<(NUM_RESULTS * NUM_HOURS); i++) 
    {
        if (i < NUM_HOURS)
        {
            int idx = i*NUM_RESULTS;

            for (int k=0; k<(NUM_RESULTS); k++)
            {
                result_m[idx][1] = time;
                idx++;
            }
        time++;   
        }
    }
}

void fill_data_m(string line, int row) // Populate Data Matrix from txt file.
{
    // Variables for splitting the line string.
    string delaminator = ",";
    int start = 0, j = 0;
    int end = line.find(delaminator);

    while (end != -1) 
    {
        // Splitting string with substr function.
        string value = line.substr(start, end - start); 
        data_m[row][j] = stoi(value);
        start = end + delaminator.size();
        end = line.find(delaminator, start);
        j++;
    }
}

void read_file(string file_name) // Reads in txt File and calls fill_data_m
{
    ifstream in_file(file_name);
    string line;

    // Populates a matrix with the values from the txt file. 
    int row = 0;
    while(getline(in_file, line))
    {   
        fill_data_m(line, row);
        row ++;
    }
    in_file.close();
}

void print_matrix(int **matrix, int size) // Test Print Function.
{
    int rows = size;
    if (size != (NUM_RESULTS * NUM_HOURS))
    {
        rows = NUM_LIGHTS * NUM_HOURS * 12;
    }

    for (int i=0; i<rows; i++)
    {
        for (int j=0; j<4; j++)
        {
            cout << matrix[i][j] << " ";
        }
        cout << endl;
    }
}
///// Functions/Procedures for house keeping & testing - FINISH /////

///// CORE Functions/Procedures for tasks - START /////
// Prints final results to the console. 
void print_results()
{
    int rows = NUM_RESULTS * NUM_HOURS;

    cout << "\n~~ Top " << NUM_RESULTS << " Congested Lights p/Hour Over " << NUM_HOURS << " Hours ~~" << endl;

    for (int i=0; i<rows; i++)
    {
        if (i%3 == 0) {cout << "\n" << endl;}

        string day = get_day(result_m[i][0]);
        string time = to_string(result_m[i][1]);
        string lights = to_string(result_m[i][2]);
        string count = to_string(result_m[i][3]);

        cout << day << " " << time << " - Light ID: " << lights 
            << " - Total Traffic (5-min interval): " << count << " vehicles." << endl;
    }
}

// Inserts top results into results matrix
void insert_row(int **matrix, int m_i, int hour, int indx, int value) 
{
    int i = ((hour - 6) * 3) + 2; // Find the end of the relevant block.

    if (result_m[indx][3] == 0)
    {
        result_m[indx] = matrix[m_i];
    }
    else
    {
        if (value > result_m[i][3])
        {
            result_m[i-2] = result_m[i-1];
            result_m[i-1] = result_m[i];
            result_m[i] = matrix[m_i];
        }
        else if (value > result_m[i-1][3] && value != result_m[i][3])
        {
            result_m[i-2] = result_m[i-1];
            result_m[i-1] = matrix[m_i];
        }
        else if (value > result_m[i-2][3] && value != result_m[i-1][3])
        {
            result_m[i-2] = matrix[m_i];
        }
    }

}

// If conditions met, calls insert_row to record top congested lights.
void record_results(int **matrix, int indx)
{
    int value = matrix[indx][3];
    int hour = matrix[indx][1]/100;

    for (int i=0; i<(NUM_RESULTS * NUM_HOURS); i++)
    {
        if ((result_m[i][1]/100) == hour || result_m[i][1] == hour)
        {
            if (value > result_m[i][3])
            {
                insert_row(matrix, indx, hour, i, value);
            }
        }
    }
}

// PRODUCER PROCEDURE: 
// Pulls data from Matrix and places into buffer for consumers. 
void *producer(void *args)
{
    // unpacking the args object.
    producer_data *p_data;
    p_data = (producer_data*) args;

    for (int i=p_data->start; i<p_data->stop; i++) 
    {
        // Wait for an empty position in the buffer
        sem_wait(buff_avail_count);

        // CRITICAL SECTION - START //
        pthread_mutex_lock(&mutex_lock);
        // Insert traffic data row into buffer
        buffer[insert] = data_m[i];
        printf("Producer %d: Inserting Data -> Time: %d ID: %d at  %d\n", *(
            (int *)&p_data->id), data_m[i][1], data_m[i][2], insert); 
        // Update insertion counter
        insert = (insert+1)%BUFFER_SIZE;
        // CRITICAL SECTION - END //
        
        pthread_mutex_unlock(&mutex_lock);
        // Flag Data Available.
        sem_post(consume_flag);
    }
    pthread_exit(NULL);
}

// CONSUMER PROCEDURE: 
// Consumes data from Buffer and adds data to the Results Matrix. 
void *consumer(void *args)
{
    // Dividing up the work for the number of consumers.
    int data_size = (NUM_LIGHTS * NUM_HOURS * 12) / NUM_CONSUMERS;

    for (int i=0; i<data_size; i++) 
    {
        // Wait for data to be placed in the buffer.
        sem_wait(consume_flag);

        // CRITICAL SECTION - START //
        pthread_mutex_lock(&mutex_lock);
        // Extract Traffic data from buffer.
        temp[0] = buffer[extract];
        // Pass the date to update max congestion.
        record_results(temp, 0);
        printf("Consumer %d: Removed Data -> Time: %d ID: %d from %d\n", *(
            (int *)args),temp[0][1], temp[0][2], extract);
        // Update extraction counter.
        extract = (extract+1)%BUFFER_SIZE;
        // CRITICAL SECTION - END //

        pthread_mutex_unlock(&mutex_lock);
        // Flag position available 
        sem_post(buff_avail_count);
    }
    pthread_exit(NULL);
}

// MAIN
int main()
{
    // Generates fake traffic data. 
    create_input_data(true); // Update to TRUE if Global Values Changed.
    alloc_mem();            // Allocates required memory 
    prep_result_m();        // Prepares results matrix 

    read_file("data_file.txt"); // Reads in the fake traffic data from file. 

     // Setting the partition sizes and adjusting the number of threads. 
    int partition = (NUM_LIGHTS * NUM_HOURS * 12) / NUM_PRODUCERS;
    int remainder = (NUM_LIGHTS * NUM_HOURS * 12) % NUM_PRODUCERS;
    int num_producers = NUM_PRODUCERS;
    // Adds an additional thread to handle the remainder.
    if (remainder != 0) 
    {
        num_producers+= 1;
    }

    // Creating an array of producers & consumers + producer_data structs.
    pthread_t produce[num_producers], consume[NUM_CONSUMERS];
    producer_data p_data[num_producers];

    // Initialising Mutex Lock
    pthread_mutex_init(&mutex_lock, NULL);

    // Initialising Buffer availability Semaphore for producers to fill.
    if ((buff_avail_count = sem_open(BUFFER_COUNT, O_CREAT, 0660, BUFFER_SIZE)) == SEM_FAILED) // 0644
    {
        perror ("sem_open"); // Catches error
        exit (1);
    }

    // Initialising data availability Semaphore for consumers to consume.
    if ((consume_flag = sem_open (CONSUMER_FLAG, O_CREAT, 0660, 0)) == SEM_FAILED)
    {
        perror ("sem_open"); // Catches error
        exit (1);
    }

    // Initialises Producer threads setting the partitions for each.
    for(int i = 0; i < num_producers; i++) 
    {
        p_data[i].start = partition * i;
        p_data[i].stop = p_data[i].start + partition;
        p_data[i].id = i + NUM_CONSUMERS + 1; // ID's count on from consumers.
        // Dealing with the remainder partition.
        if (i == num_producers - 1)
        {
            p_data[i].stop = (NUM_LIGHTS * NUM_HOURS * 12);
        }
        // Creates and runs the producers
        pthread_create(&produce[i], NULL, producer, (void *)&p_data[i]);
    }

    // Initialises Consumer threads setting the partitions for each.
    for(int i = 0; i < NUM_CONSUMERS; i++) 
    {
        int id = i+1; // Sets ID's starting at 1.
        // Creates and runs the consumers
        pthread_create(&consume[i], NULL, consumer, (void *)&id);
    }

    // Wait for other threads to finish their work. 
    for(int i = 0; i < num_producers; i++) 
    {
        pthread_join(produce[i], NULL);
    }
    for(int i = 0; i < NUM_CONSUMERS; i++) 
    {
        pthread_join(consume[i], NULL);
    }

    // Destroy Mutex lock once done. 
    pthread_mutex_destroy(&mutex_lock);

    // Unlink / Destroy Semaphores once finished. 
    if (sem_unlink("/buffer_count") == -1) 
    {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }
     if (sem_unlink("/consumer_flag") == -1) 
    {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }

    // Prints the results to the console. 
    print_results();
    cout << "\n" << endl;

    // Deallocates memory.
    dealloc_mem();
    return 0;
}

// REFERENCE (MAC Does not support Unnamed Semaphores): https://medium.com/helderco/semaphores-in-mac-os-x-fd7a7418e13b
// &: https://www.ibm.com/docs/en/i/7.1?topic=ssw_ibm_i_71/apis/ipcsemo.htm
