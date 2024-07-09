#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>

#define MAX_PATH 1024

typedef struct {
    int buffer_size;
    int num_workers;
    char src_dir[MAX_PATH];
    char dest_dir[MAX_PATH];
} Config;

typedef struct {
    char src_path[MAX_PATH];
    char dest_path[MAX_PATH];
    int src_fd;
    int dest_fd;
} FilePair;

typedef struct {
    FilePair *buffer;
    int buffer_size;
    int in;
    int out;
    int count;
    int done;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} Buffer;

typedef struct {
    int files_copied;
    int bytes_copied;
    int errors;
    int dirs_copied;
} Statistics;

Config config;
Buffer buffer;
Statistics stats;
pthread_t manager_tid;
pthread_t *worker_tids;

void *manager_thread(void *arg);
void *worker_thread(void *arg);
void copy_file(FilePair *pair);
void parse_args(int argc, char *argv[]);
void init_buffer(int buffer_size);
void destroy_buffer();
void print_stats(double elapsed_time);
double get_time_diff(struct timeval start, struct timeval end);
void process_directory(const char *src_dir, const char *dest_dir);
void handle_sigint(int sig);

int main(int argc, char *argv[]) {
    // Register signal handler for SIGINT (CTRL+C)
    signal(SIGINT, handle_sigint);

    // Parse command-line arguments
    parse_args(argc, argv);

    // Initialize the buffer with the specified size
    init_buffer(config.buffer_size);

    worker_tids = malloc(config.num_workers * sizeof(pthread_t));
    if (worker_tids == NULL) {
        fprintf(stderr, "Failed to allocate memory for worker threads\n");
        destroy_buffer();
        exit(EXIT_FAILURE);
    }

    struct timeval start, end;

    gettimeofday(&start, NULL);

    // Create manager thread
    if (pthread_create(&manager_tid, NULL, manager_thread, NULL) != 0) {
        perror("pthread_create");
        free(worker_tids);
        destroy_buffer();
        exit(EXIT_FAILURE);
    }

    // Create worker threads
    for (int i = 0; i < config.num_workers; ++i) {
        if (pthread_create(&worker_tids[i], NULL, worker_thread, NULL) != 0) {
            perror("pthread_create");
            buffer.done = 1;
            pthread_cond_broadcast(&buffer.not_empty);
            for (int j = 0; j <= i; ++j) {
                pthread_join(worker_tids[j], NULL);
            }
            pthread_join(manager_tid, NULL);
            free(worker_tids);
            destroy_buffer();
            exit(EXIT_FAILURE);
        }
    }

    // Wait for manager thread to complete
    pthread_join(manager_tid, NULL);

    // Wait for worker threads to complete
    for (int i = 0; i < config.num_workers; ++i) {
        pthread_join(worker_tids[i], NULL);
    }

    gettimeofday(&end, NULL);

    // Calculate and print elapsed time
    double elapsed_time = get_time_diff(start, end);
    print_stats(elapsed_time);

    // Destroy the buffer
    destroy_buffer();
    free(worker_tids);

    return 0;
}

void parse_args(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <buffer_size> <num_workers> <src_dir> <dest_dir>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Initialize configuration with command-line arguments
    config.buffer_size = atoi(argv[1]);
    config.num_workers = atoi(argv[2]);
    strncpy(config.src_dir, argv[3], MAX_PATH);
    strncpy(config.dest_dir, argv[4], MAX_PATH);

    if (config.buffer_size <= 0 || config.num_workers <= 0) {
        fprintf(stderr, "Invalid buffer size or number of workers.\n");
        exit(EXIT_FAILURE);
    }
}

void init_buffer(int buffer_size) {
    // Allocate memory for the buffer
    buffer.buffer = malloc(buffer_size * sizeof(FilePair));
    if (buffer.buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for buffer\n");
        exit(EXIT_FAILURE);
    }
    buffer.buffer_size = buffer_size;
    buffer.in = 0;
    buffer.out = 0;
    buffer.count = 0;
    buffer.done = 0;
    pthread_mutex_init(&buffer.mutex, NULL);
    pthread_cond_init(&buffer.not_empty, NULL);
    pthread_cond_init(&buffer.not_full, NULL);

    stats.files_copied = 0;
    stats.bytes_copied = 0;
    stats.errors = 0;
    stats.dirs_copied = 0;
}

void destroy_buffer() {
    // Free allocated memory and destroy mutexes and condition variables
    free(buffer.buffer);
    pthread_mutex_destroy(&buffer.mutex);
    pthread_cond_destroy(&buffer.not_empty);
    pthread_cond_destroy(&buffer.not_full);
}

void *manager_thread(void *arg) {
    // Start processing the source directory
    process_directory(config.src_dir, config.dest_dir);

    pthread_mutex_lock(&buffer.mutex);
    buffer.done = 1;  // Mark processing as done
    pthread_cond_broadcast(&buffer.not_empty);
    pthread_mutex_unlock(&buffer.mutex);

    return NULL;
}

void process_directory(const char *src_dir, const char *dest_dir) {
    DIR *dir = opendir(src_dir);
    if (dir == NULL) {
        perror("opendir");
        pthread_mutex_lock(&buffer.mutex);
        buffer.done = 1;  // Ensure worker threads can exit if there's an error
        pthread_cond_broadcast(&buffer.not_empty);
        pthread_mutex_unlock(&buffer.mutex);
        return;
    }

    if (mkdir(dest_dir, 0755) == -1 && errno != EEXIST) {
        perror("mkdir");
        closedir(dir);
        pthread_mutex_lock(&buffer.mutex);
        buffer.done = 1;
        pthread_cond_broadcast(&buffer.not_empty);
        pthread_mutex_unlock(&buffer.mutex);
        return;
    }

    pthread_mutex_lock(&buffer.mutex);
    stats.dirs_copied++;  // Increment directory count
    pthread_mutex_unlock(&buffer.mutex);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[MAX_PATH];
        char dest_path[MAX_PATH];
        snprintf(src_path, MAX_PATH, "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, MAX_PATH, "%s/%s", dest_dir, entry->d_name);

        if (entry->d_type == DT_DIR) {
            // Recursively process subdirectories
            process_directory(src_path, dest_path);
        } else if (entry->d_type == DT_REG) {
            // Handle regular files
            int src_fd = open(src_path, O_RDONLY);
            if (src_fd == -1) {
                perror("open src");
                pthread_mutex_lock(&buffer.mutex);
                stats.errors++;  // Increment error count
                pthread_mutex_unlock(&buffer.mutex);
                continue;
            }

            int dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (dest_fd == -1) {
                perror("open dest");
                close(src_fd);
                pthread_mutex_lock(&buffer.mutex);
                stats.errors++;
                pthread_mutex_unlock(&buffer.mutex);
                continue;
            }

            pthread_mutex_lock(&buffer.mutex);

            while (buffer.count == buffer.buffer_size) {
                // Wait if buffer is full
                pthread_cond_wait(&buffer.not_full, &buffer.mutex);
            }

            // Add file pair to buffer
            strncpy(buffer.buffer[buffer.in].src_path, src_path, MAX_PATH);
            strncpy(buffer.buffer[buffer.in].dest_path, dest_path, MAX_PATH);
            buffer.buffer[buffer.in].src_fd = src_fd;
            buffer.buffer[buffer.in].dest_fd = dest_fd;
            buffer.in = (buffer.in + 1) % buffer.buffer_size;
            buffer.count++;

            pthread_cond_signal(&buffer.not_empty);
            pthread_mutex_unlock(&buffer.mutex);
        }
    }

    closedir(dir);
}

void *worker_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&buffer.mutex);

        while (buffer.count == 0 && !buffer.done) {
            // Wait if buffer is empty
            pthread_cond_wait(&buffer.not_empty, &buffer.mutex);
        }

        if (buffer.count == 0 && buffer.done) {
            pthread_mutex_unlock(&buffer.mutex);
            break;
        }

        // Remove file pair from buffer
        FilePair pair = buffer.buffer[buffer.out];
        buffer.out = (buffer.out + 1) % buffer.buffer_size;
        buffer.count--;

        pthread_cond_signal(&buffer.not_full);
        pthread_mutex_unlock(&buffer.mutex);

        // Copy the file
        copy_file(&pair);

        close(pair.src_fd);
        close(pair.dest_fd);
    }

    return NULL;
}

void copy_file(FilePair *pair) {
    char buf[4096];
    ssize_t bytes_read, bytes_written;
    while ((bytes_read = read(pair->src_fd, buf, sizeof(buf))) > 0) {
        bytes_written = write(pair->dest_fd, buf, bytes_read);
        if (bytes_written != bytes_read) {
            perror("write");
            pthread_mutex_lock(&buffer.mutex);
            stats.errors++;
            pthread_mutex_unlock(&buffer.mutex);
            break;
        }

        pthread_mutex_lock(&buffer.mutex);
        stats.bytes_copied += bytes_written;  // Increment bytes copied count
        pthread_mutex_unlock(&buffer.mutex);
    }

    if (bytes_read == -1) {
        perror("read");
        pthread_mutex_lock(&buffer.mutex);
        stats.errors++;  // Increment error count
        pthread_mutex_unlock(&buffer.mutex);
    } else {
        pthread_mutex_lock(&buffer.mutex);
        stats.files_copied++;  // Increment files copied count
        pthread_mutex_unlock(&buffer.mutex);
    }
}

void print_stats(double elapsed_time) {
    long seconds = (long)elapsed_time;
    long milliseconds = (elapsed_time - seconds) * 1000;
    long minutes = seconds / 60;
    seconds = seconds % 60;

    // Print statistics
    printf("\n---------------STATISTICS--------------------\n");
    printf("Consumers: %d - Buffer Size: %d\n", config.num_workers, buffer.buffer_size);
    printf("Number of Regular Files: %d\n", stats.files_copied);
    printf("Number of FIFO Files: %d\n", 0);  // Assuming no FIFO files for simplicity
    printf("Number of Directories: %d\n", (stats.dirs_copied - 1));  // Subtract 1 to exclude the root directory
    printf("TOTAL BYTES COPIED: %d\n", stats.bytes_copied);
    printf("TOTAL TIME: %02ld:%02ld.%03ld (min:sec.mili)\n", minutes, seconds, milliseconds);
}

double get_time_diff(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
}

void handle_sigint(int sig) {
    // Handle CTRL+C (SIGINT) signal
    printf("\nSIGINT received. Cleaning up...\n");

    pthread_mutex_lock(&buffer.mutex);
    buffer.done = 1;
    pthread_cond_broadcast(&buffer.not_empty);
    pthread_mutex_unlock(&buffer.mutex);

    // Wait for manager thread to complete
    pthread_join(manager_tid, NULL);

    // Wait for worker threads to complete
    for (int i = 0; i < config.num_workers; ++i) {
        pthread_join(worker_tids[i], NULL);
    }

    // Print statistics
    struct timeval end;
    gettimeofday(&end, NULL);
    double elapsed_time = get_time_diff((struct timeval){0}, end);
    print_stats(elapsed_time);

    // Destroy the buffer
    destroy_buffer();
    free(worker_tids);

    exit(EXIT_SUCCESS);
}
