#include <stdio.h>      // For printf, fprintf, fopen, perror
#include <stdlib.h>     // For exit, EXIT_FAILURE
#include <pthread.h>    // For pthread_create, pthread_join, pthread_t
#include <stdbool.h>    // For bool, true, false
#include <unistd.h>     // For sleep()   

#define BUFFER_SIZE 10  

int buffer[BUFFER_SIZE];
int current_pos = 0;
bool buffer_full = false;
FILE *file;

// producer thread function
void *producer(void *arg) {
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = i + 1;     // Fill buffer with values 1 to 10
        current_pos++;          // Increment the shared counter
        printf("Producer: wrote buffer[%d] = %d\n", i, buffer[i]); // Log what was written
        sleep(1);               // Simulate slow production
    }

    while (!buffer_full) {  // keep checking boolean until someone else switches it to true
        sleep(1);   
    }

    for (int i = 0; i < BUFFER_SIZE; i++) {
        fprintf(file, "%d\n", buffer[i]);   // write buffer contents to file
    }

    fprintf(file, "Producer: finished writing to file. Exiting.\n");
    fflush(file); // ensure all output is written to the file

    return NULL;    //exit this threat
}

int main() {
    pthread_t producer_thread;  // Thread identifier for the producer thread

    file = fopen("output.txt", "w");
    if (file == NULL) {                    
        perror("Failed to open file");     
        exit(EXIT_FAILURE);               
    }

    if (pthread_create(&producer_thread, NULL, producer, NULL) != 0) { // create thread with function
        perror("Failed to create thread");  
        exit(EXIT_FAILURE);               
    }

    printf("Consumer: monitoring buffer...\n");
    while (current_pos < BUFFER_SIZE) {
        printf("Consumer: current_pos = %d (waiting for %d)\n", current_pos, BUFFER_SIZE);
        sleep(1);   // Check every second to avoid busy-spinning the CPU
    }

    buffer_full = true;   // Signal to producer that buffer is full

    pthread_join(producer_thread, NULL);  // Wait for producer thread to finish

    fclose(file);  // Close the file after producer is done writing
    return 0;      // Exit the program
}