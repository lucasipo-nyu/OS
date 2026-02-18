#include <stdio.h>      // For printf, FILE operations (fopen, fprintf, fclose)
#include <stdlib.h>     // For exit, EXIT_FAILURE
#include <pthread.h>    // For pthread_create, pthread_join, pthread_t
#include <stdbool.h>    // For bool, true, false
#include <unistd.h>     // For sleep()

#define BUFFER_SIZE 10  // Size of the shared buffer array

// Shared variables (accessed by both producer thread and consumer/main thread) 
int buffer[BUFFER_SIZE];    // shared buffer (middle ground between producer and consumer)
int current_pos = 0;        // shared counter
bool buffer_full = false;   // Boolean flag: set to true by consumer when buffer is full. Shared so producer can see
FILE *file;                 // Shared file pointer for writing the buffer contents

//  Producer function (runs in a separate thread)
void *producer(void *arg) {
    // Step 1: Fill the buffer array one element at a time
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = i + 1;     // Write value (1, 2, 3, ..., 10) into the buffer
        current_pos++;          // Increment the shared counter so the consumer can track progress
        printf("Producer: wrote buffer[%d] = %d\n", i, buffer[i]); // Log what was written
        sleep(1);               // Sleep 1 second to simulate slow production
    }

    // Step 2: Wait (sleep) until the consumer sets buffer_full to true
    while (!buffer_full) {
        sleep(1);   // Busy-wait with sleep: keep checking if consumer acknowledged the full buffer
    }

    // Step 3: Once buffer_full is true, write the entire array to the shared file
    printf("Producer: buffer_full acknowledged, writing to file...\n");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        fprintf(file, "%d\n", buffer[i]);   // Write each buffer element as a line in the file
    }

    printf("Producer: finished writing to file. Exiting.\n");
    return NULL;    // Exit the thread
}

int main() {
    pthread_t producer_thread;  // Thread identifier for the producer thread

    file = fopen("output.txt", "w");
    if (file == NULL) {                     // Check if fopen failed
        perror("Failed to open file");      // Print the system error message
        exit(EXIT_FAILURE);                 // Terminate the program with failure status
    }

    // Create the producer thread; it starts running the producer() function
    if (pthread_create(&producer_thread, NULL, producer, NULL) != 0) {
        perror("Failed to create thread");  // Print error if thread creation fails
        exit(EXIT_FAILURE);                 // Terminate the program
    }

    // ---- Consumer logic (runs in the main thread) ----
    // Monitor current_pos: wait until the producer has filled the entire buffer
    printf("Consumer: monitoring buffer...\n");
    while (current_pos < BUFFER_SIZE) {
        printf("Consumer: current_pos = %d (waiting for %d)\n", current_pos, BUFFER_SIZE);
        sleep(1);   // Check every second to avoid busy-spinning the CPU
    }

    // When current_pos reaches BUFFER_SIZE, the buffer is full
    printf("Consumer: buffer is full! Setting buffer_full = true\n");
    buffer_full = true;     // Signal the producer that the buffer is full

    // Wait for the producer thread to finish (write to file and exit)
    pthread_join(producer_thread, NULL);

    // Read back and display what the producer wrote to the file
    fclose(file);                       // Close the file first (flush writes)
    file = fopen("output.txt", "r");    // Reopen in read mode
    if (file == NULL) {
        perror("Failed to open file for reading");
        exit(EXIT_FAILURE);
    }

    printf("Consumer: reading from file:\n");
    int value;
    while (fscanf(file, "%d", &value) == 1) {  // Read integers one by one until EOF
        printf("  %d\n", value);                // Print each value read from the file
    }

    fclose(file);   // Close the file
    printf("Consumer: done. Exiting.\n");
    return 0;       // Program exits successfully
}
