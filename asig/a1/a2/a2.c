#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 512
#define MAX_FILES 64
#define MAX_USERS 64
#define MAX_PATH_LEN 256
#define MAX_NAME 64
#define NUM_CHILDREN 5

const char *event_types[] = {
    "PROCESS_CREATE",
    "MEMORY_ALLOC",
    "FILE_OPEN",
    "USER_LOGIN",
    "SYSTEM_BOOT"
};

// figure out which child should handle this line based on event type
int get_event_index(const char *line) {
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (strstr(line, event_types[i]) != NULL)
            return i;
    }
    return -1;
}

// Child 0 - just counts how many PROCESS_CREATE events there are
void handle_process_create(int read_fd, int write_fd) {
    FILE *fp = fdopen(read_fd, "r");
    if (!fp) {
        perror("fdopen");
        exit(1);
    }

    char buf[MAX_LINE];
    int count = 0;
    while (fgets(buf, sizeof(buf), fp))
        count++;
    fclose(fp);

    char result[MAX_LINE];
    snprintf(result, sizeof(result), "%d", count);
    write(write_fd, result, strlen(result) + 1);
    close(write_fd);
}

// Child 1 - sums up all the memory sizes from MEMORY_ALLOC events
void handle_memory_alloc(int read_fd, int write_fd) {
    FILE *fp = fdopen(read_fd, "r");
    if (!fp) {
        perror("fdopen");
        exit(1);
    }

    char buf[MAX_LINE];
    long long total = 0;
    while (fgets(buf, sizeof(buf), fp)) {
        char *size_ptr = strstr(buf, "SIZE:");
        if (size_ptr)
            total += atoll(size_ptr + 5);
    }
    fclose(fp);

    char result[MAX_LINE];
    snprintf(result, sizeof(result), "%lld", total);
    write(write_fd, result, strlen(result) + 1);
    close(write_fd);
}

// Child 2 - tracks which file was opened the most and how many times
void handle_file_open(int read_fd, int write_fd) {
    FILE *fp = fdopen(read_fd, "r");
    if (!fp) {
        perror("fdopen");
        exit(1);
    }

    char buf[MAX_LINE];
    char files[MAX_FILES][MAX_PATH_LEN];
    int counts[MAX_FILES];
    int num_files = 0;

    while (fgets(buf, sizeof(buf), fp)) {
        // path is between quotes after PATH:
        char *path_ptr = strstr(buf, "PATH:\"");
        if (!path_ptr)
            continue;
        path_ptr += 6;

        char *end = strchr(path_ptr, '"');
        if (!end)
            continue;

        char path[MAX_PATH_LEN];
        int len = end - path_ptr;
        strncpy(path, path_ptr, len);
        path[len] = '\0';

        // check if we already saw this file
        int found = 0;
        for (int i = 0; i < num_files; i++) {
            if (strcmp(files[i], path) == 0) {
                counts[i]++;
                found = 1;
                break;
            }
        }
        if (!found && num_files < MAX_FILES) {
            strcpy(files[num_files], path);
            counts[num_files] = 1;
            num_files++;
        }
    }
    fclose(fp);

    // find the file with the highest count
    int max_idx = 0;
    for (int i = 1; i < num_files; i++) {
        if (counts[i] > counts[max_idx])
            max_idx = i;
    }

    char result[MAX_LINE];
    if (num_files > 0)
        snprintf(result, sizeof(result), "%s|%d", files[max_idx], counts[max_idx]);
    else
        snprintf(result, sizeof(result), "none|0");

    write(write_fd, result, strlen(result) + 1);
    close(write_fd);
}

// Child 3 - counts how many different users logged in
void handle_user_login(int read_fd, int write_fd) {
    FILE *fp = fdopen(read_fd, "r");
    if (!fp) {
        perror("fdopen");
        exit(1);
    }

    char buf[MAX_LINE];
    char users[MAX_USERS][MAX_NAME];
    int num_users = 0;

    while (fgets(buf, sizeof(buf), fp)) {
        // username is between quotes after USER:
        char *user_ptr = strstr(buf, "USER:\"");
        if (!user_ptr)
            continue;
        user_ptr += 6;

        char *end = strchr(user_ptr, '"');
        if (!end)
            continue;

        char name[MAX_NAME];
        int len = end - user_ptr;
        strncpy(name, user_ptr, len);
        name[len] = '\0';

        // only add if we haven't seen this user before
        int found = 0;
        for (int i = 0; i < num_users; i++) {
            if (strcmp(users[i], name) == 0) {
                found = 1;
                break;
            }
        }
        if (!found && num_users < MAX_USERS) {
            strcpy(users[num_users], name);
            num_users++;
        }
    }
    fclose(fp);

    char result[MAX_LINE];
    snprintf(result, sizeof(result), "%d", num_users);
    write(write_fd, result, strlen(result) + 1);
    close(write_fd);
}

// Child 4 - counts SYSTEM_BOOT events
void handle_system_boot(int read_fd, int write_fd) {
    FILE *fp = fdopen(read_fd, "r");
    if (!fp) {
        perror("fdopen");
        exit(1);
    }

    char buf[MAX_LINE];
    int count = 0;
    while (fgets(buf, sizeof(buf), fp))
        count++;
    fclose(fp);

    char result[MAX_LINE];
    snprintf(result, sizeof(result), "%d", count);
    write(write_fd, result, strlen(result) + 1);
    close(write_fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <events_file>\n", argv[0]);
        return 1;
    }

    // two pipes per child: one for sending events, one for getting results back
    int p2c[NUM_CHILDREN][2]; // parent -> child
    int c2p[NUM_CHILDREN][2]; // child -> parent
    pid_t pids[NUM_CHILDREN];

    // handler functions for each child
    void (*handlers[])(int, int) = {
        handle_process_create,
        handle_memory_alloc,
        handle_file_open,
        handle_user_login,
        handle_system_boot
    };

    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (pipe(p2c[i]) == -1 || pipe(c2p[i]) == -1) {
            perror("pipe");
            return 1;
        }

        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            return 1;
        }

        if (pids[i] == 0) {
            // child: close the ends we don't need
            close(p2c[i][1]);
            close(c2p[i][0]);

            // close the parent's ends for previous children's pipes
            // (parent already closed p2c[j][0] and c2p[j][1] after each fork)
            for (int j = 0; j < i; j++) {
                close(p2c[j][1]);
                close(c2p[j][0]);
            }

            handlers[i](p2c[i][0], c2p[i][1]);
            exit(0);
        }

        // parent: close the ends we don't need
        close(p2c[i][0]);
        close(c2p[i][1]);
    }

    // read the events file and send each line to the right child
    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    char line[MAX_LINE];
    int event_counts[NUM_CHILDREN] = {0};
    int total_events = 0;

    while (fgets(line, sizeof(line), fp)) {
        int idx = get_event_index(line);
        if (idx >= 0) {
            write(p2c[idx][1], line, strlen(line));
            event_counts[idx]++;
            total_events++;
        }
    }
    fclose(fp);

    // close write ends so children know there's no more data
    for (int i = 0; i < NUM_CHILDREN; i++)
        close(p2c[i][1]);

    // read results back from each child
    char results[NUM_CHILDREN][MAX_LINE];
    for (int i = 0; i < NUM_CHILDREN; i++) {
        int n = read(c2p[i][0], results[i], MAX_LINE - 1);
        if (n > 0)
            results[i][n] = '\0';
        else
            results[i][0] = '\0';
        close(c2p[i][0]);
    }

    // wait for all children to finish
    for (int i = 0; i < NUM_CHILDREN; i++)
        waitpid(pids[i], NULL, 0);

    // print the results
    printf("PROCESS_CREATE events: %s\n", results[0]);
    printf("MEMORY_ALLOC events: Total memory allocated: %s bytes\n", results[1]);

    // FILE_OPEN result comes as "path|count"
    char *sep = strchr(results[2], '|');
    if (sep) {
        *sep = '\0';
        printf("FILE_OPEN events: Most accessed file: \"%s\" (%s times)\n",
               results[2], sep + 1);
    }

    printf("USER_LOGIN events: Number of unique users: %s\n", results[3]);
    printf("SYSTEM_BOOT events: Number of system boots: %s\n", results[4]);

    // find which event type appeared the most
    int max_idx = 0;
    for (int i = 1; i < NUM_CHILDREN; i++) {
        if (event_counts[i] > event_counts[max_idx])
            max_idx = i;
    }

    printf("\nOverall Statistics:\n");
    printf("Total events processed: %d\n", total_events);
    printf("Most common event type: %s (%d occurrences)\n",
           event_types[max_idx], event_counts[max_idx]);

    return 0;
}
