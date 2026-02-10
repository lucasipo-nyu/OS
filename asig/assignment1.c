
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid_a, pid_b, pid_grandchild;

    // parent process
    printf("PARENT_START PID=%d PPID=%d\n", getpid(), getppid());
    fflush(stdout);

    // create child A
    pid_a = fork();

    if (pid_a==0) {
        // child A process
        printf("CHILD_A_START PID=%d PPID=%d\n", getpid(), getppid());
        fflush(stdout);

        // child A creates grandchild
        pid_grandchild = fork();

        if (pid_grandchild==0) {
            // grandchild process
            printf("GRANDCHILD_START PID=%d PPID=%d\n", getpid(), getppid());
            fflush(stdout);
            printf("GRANDCHILD_EXIT PID=%d\n", getpid());
            fflush(stdout);
            return 0;
        } else {
            // child A waits for grandchild
            wait(NULL);
            printf("CHILD_A_AFTER_WAIT PID=%d PPID=%d\n", getpid(), getppid());
            fflush(stdout);
            return 0;
        }
    } else 
    {
        // parent wait for child A
        wait(NULL);
        printf("PARENT_AFTER_CHILD_A PID=%d PPID=%d\n", getpid(), getppid());
        fflush(stdout);

        // create child B
        pid_b = fork();

        if (pid_b==0) {
            // child B process
            printf("CHILD_B_START PID=%d PPID=%d\n", getpid(), getppid());
            fflush(stdout);
            return 0;
        } else {
            // parent waits for child B
            wait(NULL);
            printf("PARENT_AFTER_CHILD_B PID=%d PPID=%d\n", getpid(), getppid());
            fflush(stdout);
        }
    }

    return 0;

}
    
