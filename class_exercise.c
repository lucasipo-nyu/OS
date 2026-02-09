#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("Parent: Only parent, child did not exist yet %ld    \n", (long)getpid());
    int a =5;
    printf("Parent: a in parent before creating a child %d\n", a); 

    pid_t pid;
    pid = fork(); // create child process
    if (pid<0) {
        fprintf(stderr, "Fork failed");
        return 1;
    }
    else if (pid == 0) { // child area
        int b = 5;
        printf("Child: b in child %d\n", b);
        printf("Child: a in child %d\n", a);
        printf("Child: I am the child process with PID %ld\n", (long)getpid());
    }
    else { // parent area
        // wait(NULL);    try with and without wait 
        printf("Parent: a in parent after creating a child %d\n", a);
        printf("Parent: I am the parent process with PID %ld\n", (long)getpid());
    }
    printf("Common area to parent and child: a = %d, PID = %ld\n", a, (long)getpid());

    return 0;
}