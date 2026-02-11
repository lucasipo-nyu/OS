// Shared memory example

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>

#define SIZE_ARRAY 5

void fill_array(int *array) {
    for (int i = 0; i < SIZE_ARRAY; i++) {
        array[i] = i + 1;
    }
}

void print_array(int *array) {
    for (int i = 0; i < SIZE_ARRAY; i++) {
        printf("%d ", *(array + i));
    }
    printf("\n");
}

int main(){

    int* shared_array = (int*) mmap(NULL, sizeof(int) * SIZE_ARRAY, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // check if memory allocation failed

    if (shared_array == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // fill the array

    fill_array(shared_array);
    printf("Before fork\n");

    print_array(shared_array);

    pid_t pid = fork();

    if (pid == 0){
        printf("Child process\n");
        shared_array[2] = 8; // modify the shared array
        print_array(shared_array);
        exit(0);
    } else if (pid > 0) {
        wait(NULL); // wait for the child process to finish
        printf("Child process has finished\n");
        printf("Parent process\n");
        printf("Shared array in parent process after child modification:\n");
        print_array(shared_array); // should reflect the change made by the child
    } else {
        printf("Error\n");
    }

    //deallocate shared memory
    munmap(shared_array, sizeof(int) * SIZE_ARRAY);
    return 0;
}
